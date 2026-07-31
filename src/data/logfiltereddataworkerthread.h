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

#ifndef LOGFILTEREDDATAWORKERTHREAD_H
#define LOGFILTEREDDATAWORKERTHREAD_H

#include <QObject>
#include <QThread>
#include <QThreadPool>
#include <QMutex>
#include <QWaitCondition>
#include <QRegularExpression>
#include <QList>

#include <vector>
#include <memory>

class LogData;

// Line number are unsigned 32 bits for now.
typedef uint32_t LineNumber;

// Class encapsulating a single matching line
// Contains the line number the line was found in and its content.
class MatchingLine {
  public:
    MatchingLine( LineNumber line ) { lineNumber_ = line; };

    // Accessors
    LineNumber lineNumber() const { return lineNumber_; }

    bool operator <( const MatchingLine& other) const
    { return lineNumber_ < other.lineNumber_; }

  private:
    LineNumber lineNumber_;
};

// This is an array of matching lines.
// It shall be implemented for random lookup speed, so
// a fixed "in-place" array (vector) is probably fine.
typedef std::vector<MatchingLine> SearchResultArray;

// This class is a mutex protected set of search result data.
// It is thread safe.
class SearchData
{
  public:
    SearchData() : dataMutex_(), matches_(), maxLength_(0) { }

    // Atomically get all the search data
    void getAll( int* length, SearchResultArray* matches,
            qint64* nbLinesProcessed ) const;
    // Atomically set all the search data
    // (overwriting the existing)
    // (the matches are always moved)
    void setAll( int length, SearchResultArray&& matches );
    // Atomically add to all the existing search data.
    // The matches must be sorted and start after the last match already
    // stored, so that the result array stays sorted (it is looked up by
    // bisection). nbLinesProcessed is the number of lines searched so far,
    // counted from the start of the file, with no gaps.
    void addAll( int length, const SearchResultArray& matches, qint64 nbLinesProcessed );
    // Get the number of matches
    LineNumber getNbMatches() const;
    // Delete the match for the passed line (if it exist)
    void deleteMatch( LineNumber line );
    // Atomically clear the data.
    void clear();

  private:
    mutable QMutex dataMutex_;

    SearchResultArray matches_;
    int maxLength_;
    qint64 nbLinesProcessed_;
};

// One contiguous range of lines, searched by a single worker.
// The worker fills in the results; the ranges are merged back in order
// so that the final match list stays sorted.
struct SearchChunk
{
    qint64 firstLine;
    int    nbLines;

    SearchResultArray matches;
    int  maxLength;
    // False if the worker gave up because the search was interrupted, in
    // which case this chunk and everything after it must not be committed.
    bool completed;
};

// The range of lines a search is restricted to. Both bounds are inclusive
// and zero-based. A default-constructed range covers the whole file, and
// keeps covering it as the file grows.
struct SearchRange
{
    SearchRange() : firstLine( 0 ), lastLine( -1 ) {}
    SearchRange( qint64 first, qint64 last )
        : firstLine( first ), lastLine( last ) {}

    bool hasUpperBound() const { return lastLine >= 0; }

    // Number of lines of the file this range actually covers, given how
    // many lines the file has right now.
    qint64 endLine( qint64 nbFileLines ) const
    {
        return hasUpperBound() ? qMin( lastLine + 1, nbFileLines ) : nbFileLines;
    }

    qint64 firstLine;
    // Negative means "no upper bound".
    qint64 lastLine;
};

class SearchOperation : public QObject
{
  Q_OBJECT
  public:
    SearchOperation(const LogData* sourceLogData,
            const QRegularExpression &regExp, bool* interruptRequest,
            QThreadPool* threadPool, const SearchRange& range );

    virtual ~SearchOperation() { }

    // Start the search operation, returns true if it has been done
    // and false if it has been cancelled (results not copied)
    virtual void start( SearchData& result ) = 0;

  signals:
    void searchProgressed( int percent, int nbMatches, qint64 started );

  protected:
    static const int nbLinesInChunk;

    // Implement the common part of the search, passing
    // the shared results and the line to begin the search from.
    void doSearch( SearchData& result, qint64 initialLine );

    bool* interruptRequested_;
    const QRegularExpression regexp_;
    const LogData* sourceLogData_;
    const SearchRange range_;

  private:
    // Search one batch of chunks, spreading them over the thread pool and
    // blocking until they are all done.
    void searchChunksInParallel( std::vector<SearchChunk>& chunks );
    // Same, done inline in the calling thread. Used for batches too small
    // to be worth dispatching.
    void searchChunksInline( std::vector<SearchChunk>& chunks );

    QThreadPool* threadPool_;
};

class FullSearchOperation : public SearchOperation
{
  public:
    FullSearchOperation( const LogData* sourceLogData, const QRegularExpression& regExp,
            bool* interruptRequest, QThreadPool* threadPool,
            const SearchRange& range )
        : SearchOperation( sourceLogData, regExp, interruptRequest, threadPool,
                range ) {}
    virtual void start( SearchData& result );
};

class UpdateSearchOperation : public SearchOperation
{
  public:
    UpdateSearchOperation( const LogData* sourceLogData, const QRegularExpression& regExp,
            bool* interruptRequest, QThreadPool* threadPool,
            const SearchRange& range, qint64 position )
        : SearchOperation( sourceLogData, regExp, interruptRequest, threadPool,
                range ),
        initialPosition_( position ) {}
    virtual void start( SearchData& result );

  private:
    qint64 initialPosition_;
};

// Create and manage the thread doing loading/indexing for
// the creating LogData. One LogDataWorkerThread is used
// per LogData instance.
// Note everything except the run() function is in the LogData's
// thread.
class LogFilteredDataWorkerThread : public QThread
{
  Q_OBJECT

  public:
    LogFilteredDataWorkerThread( const LogData* sourceLogData );
    ~LogFilteredDataWorkerThread();

    // Start the search with the passed regexp, restricted to the passed
    // range of lines
    void search(const QRegularExpression &regExp, const SearchRange& range );
    // Number of workers the searches will be spread over. Read from the
    // configuration; must be called from the thread owning this object.
    void updateSearchThreadCount();
    // Continue the previous search starting at the passed position
    // in the source file (line number)
    void updateSearch( const QRegularExpression& regExp,
            const SearchRange& range, qint64 position );
    // Interrupts the search if one is in progress
    void interrupt();

    // Returns a copy of the current indexing data
    void getSearchResult( int* maxLength, SearchResultArray* searchMatches,
           qint64* nbLinesProcessed );

  signals:
    // Sent during the indexing process to signal progress
    // percent being the percentage of completion.
    void searchProgressed( int percent, int nbMatches, qint64 initial_position );
    // Sent when indexing is finished, signals the client
    // to copy the new data back.
    void searchFinished();

  protected:
    void run();

  private:
    const LogData* sourceLogData_;

    // Mutex to protect operationRequested_ and friends
    QMutex mutex_;
    QWaitCondition operationRequestedCond_;
    QWaitCondition nothingToDoCond_;

    // Set when the thread must die
    bool terminate_;
    bool interruptRequested_;
    SearchOperation* operationRequested_;

    // Workers the search chunks are spread over. Owned here rather than by
    // the operation so that the threads survive between searches, which
    // matters in follow mode where a small update search runs on every
    // append.
    QThreadPool searchThreadPool_;

    // Shared indexing data
    SearchData searchData_;
};

#endif
