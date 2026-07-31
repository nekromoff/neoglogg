/*
 * Copyright (C) 2009, 2010 Nicolas Bonnefon and other contributors
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

#include <QFile>
#include <QRunnable>
#include <QSemaphore>

#include "log.h"
#include "persistentinfo.h"
#include "configuration.h"

#include "logfiltereddataworkerthread.h"
#include "logdata.h"

// Number of lines in each chunk to read
const int SearchOperation::nbLinesInChunk = 5000;

namespace {

// Below this many chunks there is nothing to gain from handing work to the
// pool: the dispatch and the barrier cost more than the search itself. This
// keeps follow mode (which re-searches a handful of new lines on every
// append) on the direct path.
const int minChunksToParallelise = 2;

// Search a single chunk of lines. Runs in a pool thread, and only touches
// the chunk it was given, the (read-only) log data and the regexp.
void searchChunk( const LogData* logData, const QRegularExpression& regexp,
        SearchChunk* chunk, const bool* interruptRequested )
{
    chunk->maxLength = 0;
    chunk->completed = false;

    if ( *interruptRequested )
        return;

    const QStringList lines = logData->getLines( chunk->firstLine, chunk->nbLines );

    for ( int j = 0; j < lines.size(); j++ ) {
        if ( regexp.match( lines[j] ).hasMatch() ) {
            // FIXME: increase perf by removing temporary
            const int length =
                logData->getExpandedLineString( chunk->firstLine + j ).length();
            if ( length > chunk->maxLength )
                chunk->maxLength = length;
            chunk->matches.push_back( MatchingLine( chunk->firstLine + j ) );
        }
    }

    chunk->completed = true;
}

// Adapter letting searchChunk() be handed to a QThreadPool.
class SearchChunkRunnable : public QRunnable
{
  public:
    SearchChunkRunnable( const LogData* logData, const QRegularExpression& regexp,
            SearchChunk* chunk, const bool* interruptRequested, QSemaphore* done )
        : logData_( logData ), regexp_( regexp ), chunk_( chunk ),
          interruptRequested_( interruptRequested ), done_( done )
    { setAutoDelete( false ); }

    void run() override
    {
        searchChunk( logData_, regexp_, chunk_, interruptRequested_ );
        done_->release();
    }

  private:
    const LogData* logData_;
    const QRegularExpression& regexp_;
    SearchChunk* chunk_;
    const bool* interruptRequested_;
    QSemaphore* done_;
};

} // namespace

void SearchData::getAll( int* length, SearchResultArray* matches,
        qint64* lines) const
{
    QMutexLocker locker( &dataMutex_ );

    *length  = maxLength_;
    *lines   = nbLinesProcessed_;

    // This is a copy (potentially slow)
    *matches = matches_;
}

void SearchData::setAll( int length,
        SearchResultArray&& matches )
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_  = length;
    matches_    = matches;
}

void SearchData::addAll( int length,
        const SearchResultArray& matches, qint64 lines )
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_        = qMax( maxLength_, length );
    nbLinesProcessed_ = lines;

    // This does a copy as we want the final array to be
    // linear.
    matches_.insert( std::end( matches_ ),
            std::begin( matches ), std::end( matches ) );
}

LineNumber SearchData::getNbMatches() const
{
    QMutexLocker locker( &dataMutex_ );

    return matches_.size();
}

// This function starts searching from the end since we use it
// to remove the final match.
void SearchData::deleteMatch( LineNumber line )
{
    QMutexLocker locker( &dataMutex_ );

    SearchResultArray::iterator i = matches_.end();
    while ( i != matches_.begin() ) {
        i--;
        const LineNumber this_line = i->lineNumber();
        if ( this_line == line ) {
            matches_.erase(i);
            break;
        }
        // Exit if we have passed the line number to look for.
        if ( this_line < line )
            break;
    }
}

void SearchData::clear()
{
    QMutexLocker locker( &dataMutex_ );

    maxLength_        = 0;
    nbLinesProcessed_ = 0;
    matches_.clear();
}

LogFilteredDataWorkerThread::LogFilteredDataWorkerThread(
        const LogData* sourceLogData )
    : QThread(), mutex_(), operationRequestedCond_(), nothingToDoCond_(),
    searchThreadPool_(), searchData_()
{
    terminate_          = false;
    interruptRequested_ = false;
    operationRequested_ = NULL;

    sourceLogData_ = sourceLogData;

    updateSearchThreadCount();
}

LogFilteredDataWorkerThread::~LogFilteredDataWorkerThread()
{
    {
        QMutexLocker locker( &mutex_ );
        terminate_ = true;
        operationRequestedCond_.wakeAll();
    }
    wait();

    // The runnables point at stack data owned by the search operation, so
    // none may outlive the thread that ran it. By this point run() has
    // returned, which means the last batch's barrier has been passed, but
    // wait for the pool explicitly rather than relying on that.
    searchThreadPool_.waitForDone();
}

void LogFilteredDataWorkerThread::updateSearchThreadCount()
{
    std::shared_ptr<Configuration> config =
        Persistent<Configuration>( "settings" );

    const unsigned threads = config ? config->searchThreadCount() : 1;

    LOG(logDEBUG) << "Search will use " << threads << " thread(s)";

    searchThreadPool_.setMaxThreadCount( static_cast<int>( threads ) );
}

void LogFilteredDataWorkerThread::search( const QRegularExpression& regExp,
        const SearchRange& range )
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "Search requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_ = false;
    operationRequested_ = new FullSearchOperation( sourceLogData_,
            regExp, &interruptRequested_, &searchThreadPool_, range );
    operationRequestedCond_.wakeAll();
}

void LogFilteredDataWorkerThread::updateSearch(const QRegularExpression &regExp,
        const SearchRange& range, qint64 position )
{
    QMutexLocker locker( &mutex_ );  // to protect operationRequested_

    LOG(logDEBUG) << "Search requested";

    // If an operation is ongoing, we will block
    while ( (operationRequested_ != NULL) )
        nothingToDoCond_.wait( &mutex_ );

    interruptRequested_ = false;
    operationRequested_ = new UpdateSearchOperation( sourceLogData_,
            regExp, &interruptRequested_, &searchThreadPool_, range, position );
    operationRequestedCond_.wakeAll();
}

void LogFilteredDataWorkerThread::interrupt()
{
    LOG(logDEBUG) << "Search interruption requested";

    // No mutex here, setting a bool is probably atomic!
    interruptRequested_ = true;

    // We wait for the interruption to be done
    {
        QMutexLocker locker( &mutex_ );
        while ( (operationRequested_ != NULL) )
            nothingToDoCond_.wait( &mutex_ );
    }
}

// This will do an atomic copy of the object
void LogFilteredDataWorkerThread::getSearchResult(
        int* maxLength, SearchResultArray* searchMatches, qint64* nbLinesProcessed )
{
    searchData_.getAll( maxLength, searchMatches, nbLinesProcessed );
}

// This is the thread's main loop
void LogFilteredDataWorkerThread::run()
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
            connect( operationRequested_, SIGNAL( searchProgressed( int, int, qint64 ) ),
                    this, SIGNAL( searchProgressed( int, int, qint64 ) ) );

            // Run the search operation
            operationRequested_->start( searchData_ );

            LOG(logDEBUG) << "... finished copy in workerThread.";

            emit searchFinished();
            delete operationRequested_;
            operationRequested_ = NULL;
            nothingToDoCond_.wakeAll();
        }
    }
}

//
// Operations implementation
//

SearchOperation::SearchOperation( const LogData* sourceLogData,
        const QRegularExpression& regExp, bool* interruptRequest,
        QThreadPool* threadPool, const SearchRange& range )
    : regexp_( regExp ), sourceLogData_( sourceLogData ), range_( range ),
      threadPool_( threadPool )
{
    interruptRequested_ = interruptRequest;
}

void SearchOperation::searchChunksInline( std::vector<SearchChunk>& chunks )
{
    for ( auto& chunk : chunks ) {
        searchChunk( sourceLogData_, regexp_, &chunk, interruptRequested_ );

        // No point starting the next chunk if we are being cancelled.
        if ( ! chunk.completed )
            break;
    }
}

void SearchOperation::searchChunksInParallel( std::vector<SearchChunk>& chunks )
{
    QSemaphore done;
    std::vector<SearchChunkRunnable> runnables;

    // Reserved so that the runnables never move: the pool holds pointers
    // to them for as long as they are queued.
    runnables.reserve( chunks.size() );

    for ( auto& chunk : chunks ) {
        runnables.emplace_back( sourceLogData_, regexp_, &chunk,
                interruptRequested_, &done );
    }

    for ( auto& runnable : runnables )
        threadPool_->start( &runnable );

    // Every runnable releases the semaphore exactly once, including the
    // ones that bail out early, so this always comes back.
    done.acquire( static_cast<int>( runnables.size() ) );
}

void SearchOperation::doSearch( SearchData& searchData, qint64 initialLine )
{
    // Never look outside the range the search was restricted to. The upper
    // bound is re-evaluated against the current line count, so a range that
    // reaches past the end of the file simply follows it as it grows.
    const qint64 nbSourceLines = range_.endLine( sourceLogData_->getNbLine() );

    initialLine = qMax( initialLine, range_.firstLine );

    int maxLength = 0;
    int nbMatches = searchData.getNbMatches();

    LOG(logDEBUG) << "Searching from line " << initialLine << " to " << nbSourceLines;

    if ( initialLine >= nbSourceLines ) {
        emit searchProgressed( nbMatches, 100, initialLine );
        return;
    }

    // Compile the pattern once, here, rather than letting the first worker
    // to call match() do it while the others queue up behind it.
    regexp_.optimize();

    const int nbWorkers = qMax( 1, threadPool_->maxThreadCount() );

    std::vector<SearchChunk> chunks;
    chunks.reserve( nbWorkers );

    SearchResultArray batchMatches;

    qint64 nextLine = initialLine;

    while ( nextLine < nbSourceLines ) {
        if ( *interruptRequested_ )
            break;

        // Carve out one batch: as many chunks as we have workers, so the
        // whole pool is busy between two barriers.
        chunks.clear();
        qint64 batchEnd = nextLine;
        for ( int w = 0; ( w < nbWorkers ) && ( batchEnd < nbSourceLines ); ++w ) {
            const int nbLines = static_cast<int>(
                    qMin<qint64>( nbLinesInChunk, nbSourceLines - batchEnd ) );

            chunks.push_back( SearchChunk { batchEnd, nbLines,
                    SearchResultArray(), 0, false } );
            batchEnd += nbLines;
        }

        const int percentage =
            ( nextLine - initialLine ) * 100 / ( nbSourceLines - initialLine );
        emit searchProgressed( nbMatches, percentage, initialLine );

        LOG(logDEBUG) << "Batch of " << chunks.size() << " chunks starting at "
            << nextLine;

        if ( chunks.size() >= static_cast<size_t>( minChunksToParallelise ) )
            searchChunksInParallel( chunks );
        else
            searchChunksInline( chunks );

        // Merge in chunk order so the result array stays sorted. A chunk
        // that was interrupted ends the batch: committing past it would
        // record lines as searched that never were, and an update search
        // would then resume beyond them and miss their matches.
        batchMatches.clear();
        qint64 processedUpTo = nextLine;

        for ( const auto& chunk : chunks ) {
            if ( ! chunk.completed )
                break;

            maxLength = qMax( maxLength, chunk.maxLength );
            batchMatches.insert( std::end( batchMatches ),
                    std::begin( chunk.matches ), std::end( chunk.matches ) );
            processedUpTo = chunk.firstLine + chunk.nbLines;
        }

        nbMatches += static_cast<int>( batchMatches.size() );

        if ( processedUpTo > nextLine ) {
            searchData.addAll( maxLength, batchMatches, processedUpTo );
            nextLine = processedUpTo;
        }
        else {
            // Nothing survived the batch, which only happens when we were
            // interrupted before any chunk finished.
            break;
        }
    }

    emit searchProgressed( nbMatches, 100, initialLine );
}

// Called in the worker thread's context
void FullSearchOperation::start( SearchData& searchData )
{
    // Clear the shared data
    searchData.clear();

    doSearch( searchData, range_.firstLine );
}

// Called in the worker thread's context
void UpdateSearchOperation::start( SearchData& searchData )
{
    qint64 initial_line = initialPosition_;

    if ( initial_line >= 1 ) {
        // We need to re-search the last line because it might have
        // been updated (if it was not LF-terminated)
        --initial_line;
        // In case the last line matched, we don't want it to match twice.
        searchData.deleteMatch( initial_line );
    }

    doSearch( searchData, initial_line );
}
