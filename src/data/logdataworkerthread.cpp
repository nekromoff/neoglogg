/*
 * Copyright (c) 2026+ Daniel Duris, dusoft@staznosti.sk
 * Copyright (C) 2009, 2010, 2014, 2015 Nicolas Bonnefon and other contributors
 *
 * This file is part of neoglogg.
 *
 * neoglogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * neoglogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with neoglogg.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstring>

#include <QFile>

#include "log.h"

#include "logdata.h"
#include "logdataworkerthread.h"

// Size of the chunk to read (5 MiB)
const int IndexOperation::sizeChunk = 5*1024*1024;

qint64 IndexingData::getSize() const
{
    QMutexLocker locker( &dataMutex_ );

    return indexedSize_;
}

int IndexingData::getMaxLength() const
{
    QMutexLocker locker( &dataMutex_ );

    return maxLength_;
}

LineNumber IndexingData::getNbLines() const
{
    QMutexLocker locker( &dataMutex_ );

    return linePosition_.size();
}

qint64 IndexingData::getPosForLine( LineNumber line ) const
{
    QMutexLocker locker( &dataMutex_ );

    return linePosition_.at( line );
}

std::vector<qint64> IndexingData::getPosForLines( LineNumber first_line,
        int number ) const
{
    QMutexLocker locker( &dataMutex_ );

    std::vector<qint64> positions;
    positions.reserve( number );

    // Sequential reads hit the storage's read cache, so this walk is
    // linear in 'number', not quadratic.
    for ( int i = 0; i < number; ++i )
        positions.push_back( linePosition_.at( first_line + i ) );

    return positions;
}

EncodingSpeculator::Encoding IndexingData::getEncodingGuess() const
{
    QMutexLocker locker( &dataMutex_ );

    return encoding_;
}

void IndexingData::addAll( qint64 size, int length,
        const FastLinePositionArray& linePosition,
        EncodingSpeculator::Encoding encoding )

{
    QMutexLocker locker( &dataMutex_ );

    indexedSize_  += size;
    maxLength_     = qMax( maxLength_, length );
    linePosition_.append_list( linePosition );

    encoding_      = encoding;
}

void IndexingData::clear()
{
    // Called from the worker thread at the start of a full (re)index while
    // the GUI thread can be reading, so it must lock like every other
    // accessor: the old storage is freed here, an unprotected concurrent
    // read would be a use-after-free.
    QMutexLocker locker( &dataMutex_ );

    maxLength_   = 0;
    indexedSize_ = 0;
    linePosition_ = LinePositionArray();
    encoding_    = EncodingSpeculator::Encoding::ASCII7;
}

LogDataWorkerThread::LogDataWorkerThread( IndexingData* indexing_data )
    : QThread(), mutex_(), operationRequestedCond_(),
    nothingToDoCond_(), fileName_(), indexing_data_( indexing_data )
{
    terminate_          = false;
    interruptRequested_ = false;
    operationRequested_ = NULL;
}

LogDataWorkerThread::~LogDataWorkerThread()
{
    {
        QMutexLocker locker( &mutex_ );
        terminate_ = true;
        operationRequestedCond_.wakeAll();
    }
    wait();
}

void LogDataWorkerThread::attachFile( const QString& fileName )
{
    QMutexLocker locker( &mutex_ );  // to protect fileName_

    fileName_ = fileName;
}

void LogDataWorkerThread::indexAll()
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "FullIndex requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_ = false;
    operationRequested_ = new FullIndexOperation( fileName_,
            indexing_data_, &interruptRequested_, &encodingSpeculator_ );
    operationRequestedCond_.wakeAll();
}

void LogDataWorkerThread::indexAdditionalLines()
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "AddLines requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_ = false;
    operationRequested_ = new PartialIndexOperation( fileName_,
            indexing_data_, &interruptRequested_, &encodingSpeculator_ );
    operationRequestedCond_.wakeAll();
}

void LogDataWorkerThread::interrupt()
{
    LOG(logDEBUG) << "Load interrupt requested";

    // No mutex needed, the flag is atomic
    interruptRequested_ = true;
}

// This is the thread's main loop
void LogDataWorkerThread::run()
{
    QMutexLocker locker( &mutex_ );

    forever {
        while ( (terminate_ == false) && (operationRequested_ == NULL) )
            operationRequestedCond_.wait( &mutex_ );
        LOG(logDEBUG) << "Worker thread signaled";

        // Look at what needs to be done
        if ( terminate_ )
            return;      // We must die

        if ( operationRequested_ ) {
            connect( operationRequested_, SIGNAL( indexingProgressed( int ) ),
                    this, SIGNAL( indexingProgressed( int ) ) );

            // Run the operation
            try {
                if ( operationRequested_->start() ) {
                    LOG(logDEBUG) << "... finished copy in workerThread.";
                    emit indexingFinished( LoadingStatus::Successful );
                }
                else {
                    emit indexingFinished( LoadingStatus::Interrupted );
                }
            }
            catch ( std::bad_alloc& ba ) {
                LOG(logERROR) << "Out of memory whilst indexing!";
                emit indexingFinished( LoadingStatus::NoMemory );
            }

            delete operationRequested_;
            operationRequested_ = NULL;
            nothingToDoCond_.wakeAll();
        }
    }
}

//
// Operations implementation
//

IndexOperation::IndexOperation( const QString& fileName,
        IndexingData* indexingData, std::atomic_bool* interruptRequest,
        EncodingSpeculator* encodingSpeculator )
    : fileName_( fileName )
{
    interruptRequest_ = interruptRequest;
    indexing_data_ = indexingData;
    encoding_speculator_ = encodingSpeculator;
}

void IndexOperation::doIndex( IndexingData* indexing_data,
        EncodingSpeculator* encoding_speculator, qint64 initialPosition )
{
    qint64 pos = initialPosition; // Absolute position of the start of current line
    qint64 end = 0;               // Absolute position of the end of current line
    int additional_spaces = 0;    // Additional spaces due to tabs

    QFile file( fileName_ );
    if ( file.open( QIODevice::ReadOnly ) ) {
        // Count the number of lines and max length
        // (read big chunks to speed up reading from disk)
        file.seek( pos );
        while ( !file.atEnd() ) {
            FastLinePositionArray line_positions;
            // A guess at the number of lines in the chunk (assuming lines
            // of 64 bytes) to avoid repeated re-allocations while scanning.
            line_positions.reserve( sizeChunk / 64 );
            int max_length = 0;

            if ( *interruptRequest_ )
                break;

            // Read a chunk of 5MB
            const qint64 block_beginning = file.pos();
            const QByteArray block = file.read( sizeChunk );
            const char* const data = block.constData();
            const int block_size = static_cast<int>( block.length() );

            // Feed the whole chunk to the speculator in one call; the
            // line scan below then only has to look for \n and \t, which
            // memchr does far faster than a byte-at-a-time loop.
            encoding_speculator->inject_block( data, block_size );

            int scan_pos = static_cast<int>( qMax( pos - block_beginning, 0LL ) );
            while ( scan_pos < block_size ) {
                const char* nl = static_cast<const char*>(
                        memchr( data + scan_pos, '\n', block_size - scan_pos ) );
                const int line_end = nl ?
                        static_cast<int>( nl - data ) : block_size;

                // Expand the tabs of the current line (or the part of it
                // that lies in this chunk)
                const char* tab = data + scan_pos;
                const char* const seg_end = data + line_end;
                while ( ( tab = static_cast<const char*>(
                        memchr( tab, '\t', seg_end - tab ) ) ) ) {
                    // Column of the tab once the line is expanded so far
                    const qint64 column = ( block_beginning + ( tab - data ) )
                            - pos + additional_spaces;
                    additional_spaces += AbstractLogData::tabStop -
                            static_cast<int>( column % AbstractLogData::tabStop ) - 1;
                    ++tab;
                }

                if ( ! nl )
                    break;      // Line continues in the next chunk

                end = block_beginning + line_end;
                const int length = end-pos + additional_spaces;
                if ( length > max_length )
                    max_length = length;
                pos = end + 1;
                additional_spaces = 0;
                line_positions.append( pos );

                scan_pos = line_end + 1;
            }

            // Update the shared data
            indexing_data->addAll( block.length(), max_length, line_positions,
                   encoding_speculator->guess() );

            // Update the caller for progress indication
            int progress = ( file.size() > 0 ) ? pos*100 / file.size() : 100;
            emit indexingProgressed( progress );
        }

        // Check if there is a non LF terminated line at the end of the file
        qint64 file_size = file.size();
        if ( !*interruptRequest_ && file_size > pos ) {
            LOG( logWARNING ) <<
                "Non LF terminated file, adding a fake end of line";

            FastLinePositionArray line_position;
            line_position.append( file_size + 1 );
            line_position.setFakeFinalLF();

            indexing_data->addAll( 0, 0, line_position, encoding_speculator->guess() );
        }
    }
    else {
        // TODO: Check that the file is seekable?
        // If the file cannot be open, we do as if it was empty
        LOG(logWARNING) << "Cannot open file " << fileName_.toStdString();

        emit indexingProgressed( 100 );
    }
}

// Called in the worker thread's context
bool FullIndexOperation::start()
{
    LOG(logDEBUG) << "FullIndexOperation::start(), file "
        << fileName_.toStdString();

    LOG(logDEBUG) << "FullIndexOperation: Starting the count...";

    emit indexingProgressed( 0 );

    // First empty the index
    indexing_data_->clear();

    doIndex( indexing_data_, encoding_speculator_, 0 );

    LOG(logDEBUG) << "FullIndexOperation: ... finished counting."
        "interrupt = " << *interruptRequest_;

    return ( *interruptRequest_ ? false : true );
}

bool PartialIndexOperation::start()
{
    LOG(logDEBUG) << "PartialIndexOperation::start(), file "
        << fileName_.toStdString();

    qint64 initial_position = indexing_data_->getSize();

    LOG(logDEBUG) << "PartialIndexOperation: Starting the count at "
        << initial_position << " ...";

    emit indexingProgressed( 0 );

    doIndex( indexing_data_, encoding_speculator_, initial_position );

    LOG(logDEBUG) << "PartialIndexOperation: ... finished counting.";

    return ( *interruptRequest_ ? false : true );
}
