// Snap Websites Server -- C++ object to handle threads
// Copyright (c) 2013-2019  Made to Order Software Corp.  All Rights Reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Implementation of the Thread Runner and Managers.
 *
 * This file includes the implementation used by the snap_thread environment.
 */


// self
//
#include "snapwebsites/snap_thread.h"


// snapwebsites lib
//
#include "snapwebsites/log.h"


// C lib
//
#include <signal.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>


// last include
//
#include <snapdev/poison.h>




namespace snap
{


/** \class snap_thread_exception
 * \brief To catch any thread exception, catch this base thread exception.
 *
 * This is the base thread exception for all the thread exceptions.
 * You may catch this exception to catch any of the thread exceptions.
 */

/** \class snap_thread_exception_not_started
 * \brief Tried to start a thread and it failed.
 *
 * When using the thread_safe object which is created on the FIFO, one
 * guarantee is that the thread actually starts. If the threads cannot
 * be started, this exception is raised.
 */

/** \class snap_thread_exception_in_use_error
 * \brief One thread runner can be attached to one thread.
 *
 * This exception is raised if a thread notices that a runner being
 * attached to it is already attached to another thread. This is just
 * not possible. One thread runner can only be running in one thread
 * not two or three (it has to stop and be removed from another thread
 * first otherwise.)
 */

/** \class snap_thread_exception_not_locked_error
 * \brief A mutex cannot be unlocked if not locked.
 *
 * Each time we lock a mutex, we increase a counter. Each time we nulock a
 * mutex we decrease a counter. If you try to unlock when the counter is
 * zero, you have a lock/unlock discrepancy. This exception is raised
 * when such is discovered.
 */

/** \class snap_thread_exception_not_locked_once_error
 * \brief When calling wait() the mutex should be locked once.
 *
 * When calling the wait() instruction, the mutex has to be locked once.
 *
 * At this time this is commented out as it caused problems. We probably
 * need to test that it is at least locked once and not exactly once.
 */

/** \class snap_thread_exception_mutex_failed_error
 * \brief A mutex failed.
 *
 * In most cases, a mutex will fail if the input buffer is not considered
 * valid. (i.e. it was not initialized and it does not look like a mutex.)
 */

/** \class snap_thread_exception_invalid_error
 * \brief An invalid parameter or value was detected.
 *
 * This exception is raised when a parameter or a variable member or some
 * other value is out of range or generally not valid for its purpose.
 */

/** \class snap_thread_exception_system_error
 * \brief We called a system function and it failed.
 *
 * This exception is raised if a system function call fails.
 */






/** \class snap_thread::snap_thread_life
 * \brief An RAII class managing the lifetime of a thread.
 *
 * This class is used to manage the life of a thread: the time it runs.
 * The constructor calls the snap_thread::start() function and
 * the destructor makes sure to call the snap_thread::stop() function.
 *
 * If you have a specific block or another class that should run a
 * thread for the lifetime of the block or class object, then this
 * is well adapted.
 *
 * \note
 * This class is not responsible for deleting the thread at the
 * end. It only manages the time while the thread runs.
 */





/** \class snap_thread::snap_fifo
 * \brief Create a thread safe FIFO.
 *
 * This template defines a thread safe FIFO which is also a mutex.
 * You should use this snap_fifo object to lock your thread and
 * send messages/data across various threads. The FIFO itself is
 * a mutex so you can use it to lock the threads as with a normal
 * mutex:
 *
 * \code
 *  {
 *      snap_thread::snap_lock lock(f_messages);
 *      ...
 *  }
 * \endcode
 *
 * \note
 * It is recommanded that you use a smart pointer to your data as
 * the type T. This way you do not have to deal with copies in these
 * FIFOs. However, if your data is very small (a few integers, a
 * small string or two) then you may also use a type T which will
 * be shared by copy. A smart pointer, thou
 *
 * \tparam T  the type of data that the FIFO will handle.
 */

/** \typedef snap_thread::snap_fifo::items_t
 * \brief The container type of our items.
 *
 * All of the FIFO items are pushed and popped from this type of
 * container.
 */

/** \typedef snap_thread::snap_fifo::value_type
 * \brief The type of value to push and pop from the FIFO.
 *
 * This typedef returns the type T of the template.
 */

/** \typedef snap_thread::snap_fifo::fifo_type
 * \brief The type of the FIFO as a typedef.
 *
 * This is a declaration of the FIFO type from the template type T.
 * It can be useful in meta programming.
 */

/** \typedef snap_thread::snap_fifo::pointer_t;
 * \brief A smart pointer to the FIFO.
 *
 * You may want to create FIFOs on the heap in which case we strongly
 * advice that you use this shared pointer type to old those FIFOs.
 *
 * It is otherwise possible to have the FIFO as a variable member of
 * your thread. One thing to consider, though, if that if thread A
 * owns a FIFO and shares it with thread B, then you must make sure
 * that B is done before destroying A.
 */

/** \fn snap_thread::snap_fifo::push_back(T const & v)
 * \brief Push data on this FIFO.
 *
 * This function appends data on the FIFO queue. The function
 * has the side effect to wake up another thread if such is
 * currently waiting for data on the same FIFO.
 *
 * \note
 * You can also wake up the other thread by calling the signal()
 * function directly. This is especially useful after you marked
 * the FIFO as done to make sure that all the worker threads
 * wake up and exit cleanly.
 *
 * \attention
 * Remember that if a thread is not currently waiting on the
 * signal, calling signal is not likely to do anything except
 * for the one next thread that waits on that signal.
 *
 * \exception snap_thread_exception_invalid_error
 * Do not call this function after calling done(), it will raise
 * this exception if you do so.
 *
 * \param[in] v  The value to be pushed on the FIFO queue.
 *
 * \return true if the value was pushed, false otherwise.
 *
 * \sa done()
 */

/** \fn snap_thread::snap_fifo::pop_front(T & v, int64_t const usecs)
 * \brief Retrieve one value from the FIFO.
 *
 * This function retrieves one value from the thread FIFO.
 * If necessary, the function can wait for a value to be
 * received. The wait works as defined in the semaphore
 * wait() function:
 *
 * \li -1 -- wait forever (use with caution as this prevents
 *           the STOP event from working.)
 * \li 0 -- do not wait if there is no data, return immediately
 * \li +1 and more -- wait that many microseconds
 *
 * If the function works (returns true,) then \p v is set
 * to the value being popped. Otherwise v is not modified
 * and the function returns false.
 *
 * \note
 * Because of the way the pthread conditions are implemented
 * it is possible that the condition was already raised
 * when you call this function. This means the wait, even if
 * you used a value of -1 or 1 or more, will not happen.
 *
 * \note
 * If the function returns false, \p v is not set to anything
 * so it still has the value it had when calling the function.
 *
 * \param[out] v  The value read.
 * \param[in] usecs  The number of microseconds to wait.
 *
 * \return true if a value was popped, false otherwise.
 */

/** \fn snap_thread::snap_fifo::clear()
 * \brief Clear the current FIFO.
 *
 * This function can be used to clear the FIFO. Right after this
 * call, the FIFO will be empty. All the objects that were pushed
 * in the FIFO will be removed. It is your responsibility to ensure
 * they get cleaned up appropriately.
 *
 * \note
 * This function is often used along the done() function to quickly
 * terminate threads.
 *
 * \sa done()
 */

/** \fn snap_thread::snap_fifo::empty() const
 * \brief Test whether the FIFO is empty.
 *
 * This function checks whether the FIFO is empty and if so
 * returns true, otherwise it returns false.
 *
 * The function does not check the semaphore. Instead it
 * checks the size of the FIFO itself.
 *
 * \return true if the FIFO is empty.
 */

/** \fn snap_thread::snap_fifo::size() const
 * \brief Return the number of items in the FIFO.
 *
 * This function returns the number of items currently added to
 * the FIFO. This can be used by the caller to avoid flooding
 * the FIFO, if at all possible.
 *
 * The complexity of this function is O(1).
 *
 * \return the number of items in the FIFO.
 */

/** \fn snap_thread::snap_fifo::byte_size() const
 * \brief Return the total size of the FIFO uses in memory.
 *
 * This function returns the sum of each element size() function.
 *
 * \note
 * This calculation does not include the amount of bytes used by
 * the FIFO itself. It only includes the size of the elements,
 * which in most cases is what you want anyway.
 *
 * The complexity of this function is O(n).
 *
 * \return the byte size of the FIFO.
 */

/** \fn snap_thread::snap_fifo::done(bool clear)
 * \brief Mark the FIFO as done.
 *
 * By default the FIFO is not done. Once you are finished with it
 * and will never push any more data to it, call this function.
 * This flag is used by worker threads to know whether they should
 * wait for more data or just exit.
 *
 * This is rarely used with regular threads. It is more of a feature
 * for worker threads.
 *
 * \note
 * If the FIFO is empty, this function also broadcasts a signal
 * to all the worker threads so that way they can exit.
 *
 * \param[in] clear  Whether the function should also call clear()
 *
 * \sa clear()
 */

/** \fn snap_thread::snap_fifo::is_done() const
 * \brief Check whether the FIFO was marked as done.
 *
 * When a child process calls pop_front() and the function returns
 * false, it means the FIFO is empty. On return, the thread may
 * then check whether is_done() is true. If so, then the thread
 * is expected to exit (no more data will even be added to the
 * FIFO so you might as well leave.)
 *
 * \return true if the thread is expected to exit, false while still
 *         running.
 */

/** \var snap_thread::snap_fifo::f_queue
 * \brief The actual FIFO.
 *
 * This variable member holds the actual data in this FIFO
 * object.
 */

/** \var snap_thread::snap_fifo::f_done
 * \brief Whether the FIFO is done.
 *
 * This flag tells us whether the FIFO is done or not.
 */

/** \var snap_thread::snap_fifo::f_broadcast
 * \brief Whether the done() function called broadcast().
 *
 * This variable is set to true once the done() function called the
 * broadcast() function of the mutex. This way we avoid calling it
 * more than once even if you call the done() function multiple
 * times.
 */


/** \class snap_thread::snap_mutex
 * \brief A mutex object to ensures atomicity.
 *
 * This class is used by threads when some data accessed by more than
 * one thread is about to be accessed. In most cases it is used with the
 * snap_lock class so it is safe even in the event an exception is raised.
 *
 * The snap_mutex also includes a condition variable which can be signaled
 * using the signal() function. This wakes threads that are currently
 * waiting on the condition with one of the wait() functions.
 *
 * \note
 * We use a recursive mutex so you may lock the mutex any number of times.
 * It has to be unlocked that many times, of course.
 */

/** \var snap_thread::snap_mutex::f_mutex
 * \brief The pthread mutex.
 *
 * This variable member holds the pthread mutex. The snap_mutex
 * implementation manages this field as required.
 */


/** \brief An inter-thread mutex to ensure unicity of execution.
 *
 * The mutex object is used to lock part of the code that needs to be run
 * by only one thread at a time. This is also called a critical section
 * and a memory barrier.
 *
 * In most cases one uses the snap_lock object to temporarily lock
 * the mutex using the FIFO to help ensure the mutex gets unlocked as
 * required in the event an exception occurs.
 *
 * \code
 * {
 *    snap_thread::snap_lock lock(&my_mutex)
 *    ... // protected code
 * }
 * \endcode
 *
 * The lock can be tried to see whether another thread already has the
 * lock and fail if so. See the try_lock() function.
 *
 * The class also includes a condition in order to send signals and wait
 * on signals. There are two ways to send signals and three ways to wait.
 * Note that to call any one of the wait funtions you must first have the
 * mutex locked, what otherwise happens is undefined.
 *
 * \code
 * {
 *      // wake one waiting thread
 *      my_mutex.signal();
 *
 *      // wake all the waiting thread
 *      my_mutex.broadcast();
 *
 *      // wait on the signal forever
 *      {
 *          snap_thread::snap_lock lock(&my_mutex);
 *          my_mutex.wait();
 *      }
 *
 *      // wait on the signal for the specified amount of time
 *      {
 *          snap_thread::snap_lock lock(&my_mutex);
 *          my_mutex.timed_wait(1000000UL); // wait up to 1 second
 *      }
 *
 *      // wait on the signal for until date or later or the signal
 *      {
 *          snap_thread::snap_lock lock(&my_mutex);
 *          my_mutex.dated_wait(date); // wait on signal or until date
 *      }
 * }
 * \endcode
 *
 * If you need a FIFO of messages between your threads, look at the
 * snap_fifo template.
 *
 * \note
 * Care must be used to always initialized a mutex before it
 * is possibly accessed by more than one thread. This is usually
 * the case in the constructor of your objects.
 *
 * \exception snap_thread_exception_invalid_error
 * If any one of the initialization functions fails, this exception is
 * raised. The function also logs the error.
 */
snap_thread::snap_mutex::snap_mutex()
{
    // initialize the mutex
    pthread_mutexattr_t mattr;
    int err(pthread_mutexattr_init(&mattr));
    if(err != 0)
    {
        SNAP_LOG_FATAL("a mutex attribute structure could not be initialized, error #")(err);
        throw snap_thread_exception_invalid_error("pthread_muteattr_init() failed");
    }
    err = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
    if(err != 0)
    {
        SNAP_LOG_FATAL("a mutex attribute structure type could not be setup, error #")(err);
        pthread_mutexattr_destroy(&mattr);
        throw snap_thread_exception_invalid_error("pthread_muteattr_settype() failed");
    }
    err = pthread_mutex_init(&f_mutex, &mattr);
    if(err != 0)
    {
        SNAP_LOG_FATAL("a mutex structure could not be initialized, error #")(err);
        pthread_mutexattr_destroy(&mattr);
        throw snap_thread_exception_invalid_error("pthread_mutex_init() failed");
    }
    err = pthread_mutexattr_destroy(&mattr);
    if(err != 0)
    {
        SNAP_LOG_FATAL("a mutex attribute structure could not be destroyed, error #")(err);
        pthread_mutex_destroy(&f_mutex);
        throw snap_thread_exception_invalid_error("pthread_mutexattr_destroy() failed");
    }

    // initialize the condition
    pthread_condattr_t cattr;
    err = pthread_condattr_init(&cattr);
    if(err != 0)
    {
        SNAP_LOG_FATAL("a mutex condition attribute structure could not be initialized, error #")(err);
        pthread_mutex_destroy(&f_mutex);
        throw snap_thread_exception_invalid_error("pthread_condattr_init() failed");
    }
    err = pthread_cond_init(&f_condition, &cattr);
    if(err != 0)
    {
        SNAP_LOG_FATAL("a mutex condition structure could not be initialized, error #")(err);
        pthread_condattr_destroy(&cattr);
        pthread_mutex_destroy(&f_mutex);
        throw snap_thread_exception_invalid_error("pthread_cond_init() failed");
    }
    err = pthread_condattr_destroy(&cattr);
    if(err != 0)
    {
        SNAP_LOG_FATAL("a mutex condition attribute structure could not be destroyed, error #")(err);
        pthread_mutex_destroy(&f_mutex);
        throw snap_thread_exception_invalid_error("pthread_condattr_destroy() failed");
    }
}


/** \brief Clean up a mutex object.
 *
 * This function ensures that the mutex object is cleaned up, which means
 * the mutex and conditions get destroyed.
 *
 * This destructor verifies that the mutex is not currently locked. A
 * locked mutex can't be destroyed. If still locked, then an error is
 * sent to the logger and the function calls exit(1).
 */
snap_thread::snap_mutex::~snap_mutex()
{
    // Note that the following reference count test only ensure that
    // you don't delete a mutex which is still locked; however, if
    // you still have multiple threads running, we can't really know
    // if another thread is not just about to use this thread...
    //
    if(f_reference_count != 0UL)
    {
        // we cannot legally throw in a destructor so we instead generate a fatal error
        //
        SNAP_LOG_FATAL("a mutex is being destroyed when its reference count is ")(f_reference_count)(" instead of zero.");
        exit(1);
    }
    int err(pthread_cond_destroy(&f_condition));
    if(err != 0)
    {
        SNAP_LOG_ERROR("a mutex condition destruction generated error #")(err);
    }
    err = pthread_mutex_destroy(&f_mutex);
    if(err != 0)
    {
        SNAP_LOG_ERROR("a mutex destruction generated error #")(err);
    }
}


/** \brief Lock a mutex
 *
 * This function locks the mutex. The function waits until the mutex is
 * available if it is not currently available. To avoid waiting one may
 * want to use the try_lock() function instead.
 *
 * Although the function cannot fail, the call can lock up a process if
 * two or more mutexes are used and another thread is already waiting
 * on this process.
 *
 * \exception snap_thread_exception_invalid_error
 * If the lock fails, this exception is raised.
 */
void snap_thread::snap_mutex::lock()
{
    int const err(pthread_mutex_lock(&f_mutex));
    if(err != 0)
    {
        SNAP_LOG_ERROR("a mutex lock generated error #")(err)(" -- ")(strerror(err));
        throw snap_thread_exception_invalid_error("pthread_mutex_lock() failed");
    }

    // note: we do not need an atomic call since we
    //       already know we are running alone here...
    ++f_reference_count;
}


/** \brief Try locking the mutex.
 *
 * This function tries locking the mutex. If the mutex cannot be locked
 * because another process already locked it, then the function returns
 * immediately with false.
 *
 * \exception snap_thread_exception_invalid_error
 * If the lock fails, this exception is raised.
 *
 * \return true if the lock succeeded, false otherwise.
 */
bool snap_thread::snap_mutex::try_lock()
{
    int const err(pthread_mutex_trylock(&f_mutex));
    if(err == 0)
    {
        // note: we do not need an atomic call since we
        //       already know we are running alone here...
        ++f_reference_count;
        return true;
    }

    // failed because another thread has the lock?
    if(err == EBUSY)
    {
        return false;
    }

    // another type of failure
    SNAP_LOG_ERROR("a mutex try lock generated error #")(err)(" -- ")(strerror(err));
    throw snap_thread_exception_invalid_error("pthread_mutex_trylock() failed");
}


/** \brief Unlock a mutex.
 *
 * This function unlock the specified mutex. The function must be called
 * exactly once per call to the lock() function, or successful call to
 * the try_lock() function.
 *
 * The unlock never waits.
 *
 * \exception snap_thread_exception_invalid_error
 * If the unlock fails, this exception is raised.
 *
 * \exception snap_thread_exception_not_locked_error
 * If the function is called too many times, then the lock count is going
 * to be zero and this exception will be raised.
 */
void snap_thread::snap_mutex::unlock()
{
    // We can't unlock if it wasn't locked before!
    if(f_reference_count <= 0UL)
    {
        SNAP_LOG_FATAL("attempting to unlock a mutex when it is still locked ")(f_reference_count)(" times");
        throw snap_thread_exception_not_locked_error("unlock was called too many times");
    }

    // NOTE: we do not need an atomic call since we
    //       already know we are running alone here...
    --f_reference_count;

    int const err(pthread_mutex_unlock(&f_mutex));
    if(err != 0)
    {
        SNAP_LOG_ERROR("a mutex unlock generated error #")(err)(" -- ")(strerror(err));
        throw snap_thread_exception_invalid_error("pthread_mutex_unlock() failed");
    }
}


/** \brief Wait on a mutex condition.
 *
 * At times it is useful to wait on a mutex to become available without
 * polling the mutex (which uselessly wastes precious processing time.)
 * This function can be used to wait on a mutex condition.
 *
 * This version of the wait() blocks until a signal is received.
 *
 * \warning
 * This function cannot be called if the mutex is not locked or the
 * wait will fail in unpredicatable ways.
 *
 * \exception snap_thread_exception_not_locked_once_error
 * This exception is raised if the reference count is not exactly 1.
 * In other words, the mutex must be locked by the caller but only
 * one time.
 *
 * \exception snap_thread_exception_mutex_failed_error
 * This exception is raised in the event the conditional wait fails.
 */
void snap_thread::snap_mutex::wait()
{
    // For any mutex wait to work, we MUST have the
    // mutex locked already and just one time.
    //
    // note: the 1 time is just for assurance that it will
    //       work in most cases; it should work even when locked
    //       multiple times, but it is less likely. For sure, it
    //       has to be at least once.
    //if(f_reference_count != 1UL)
    //{
    //    SNAP_LOG_FATAL("attempting to wait on a mutex when it is not locked exactly once, current count is ")(f_reference_count);
    //    throw snap_thread_exception_not_locked_once_error();
    //}
    int const err(pthread_cond_wait(&f_condition, &f_mutex));
    if(err != 0)
    {
        // an error occurred!
        SNAP_LOG_ERROR("a mutex conditional wait generated error #")(err)(" -- ")(strerror(err));
        throw snap_thread_exception_mutex_failed_error("pthread_cond_wait() failed");
    }
}


/** \brief Wait on a mutex condition with a time limit.
 *
 * At times it is useful to wait on a mutex to become available without
 * polling the mutex, but only for some time. This function waits for
 * the number of specified micro seconds. The function returns early if
 * the condition was triggered. Otherwise it waits until the specified
 * number of micro seconds elapsed and then returns.
 *
 * \warning
 * This function cannot be called if the mutex is not locked or the
 * wait will fail in unpredicatable ways.
 *
 * \exception snap_thread_exception_system_error
 * This exception is raised if a function returns an unexpected error.
 *
 * \exception snap_thread_exception_mutex_failed_error
 * This exception is raised when the mutex wait function fails.
 *
 * \param[in] usecs  The maximum number of micro seconds to wait until you
 *                   receive the signal.
 *
 * \return true if the condition was raised, false if the wait timed out.
 */
bool snap_thread::snap_mutex::timed_wait(uint64_t const usecs)
{
    // For any mutex wait to work, we MUST have the
    // mutex locked already and just one time.
    //
    // note: the 1 time is just for assurance that it will
    //       work in most cases; it should work even when locked
    //       multiple times, but it is less likely. For sure, it
    //       has to be at least once.
    //if(f_reference_count != 1UL)
    //{
    //    SNAP_LOG_FATAL("attempting to timed wait ")(usec)(" usec on a mutex when it is not locked exactly once, current count is ")(f_reference_count);
    //    throw snap_thread_exception_not_locked_once_error();
    //}

    int err(0);

    // get time now
    struct timeval vtime;
    if(gettimeofday(&vtime, nullptr) != 0)
    {
        err = errno;
        SNAP_LOG_FATAL("gettimeofday() failed with errno: ")(err)(" -- ")(strerror(err));
        throw snap_thread_exception_system_error("gettimeofday() failed");
    }

    // now + user specified usec
    struct timespec timeout;
    timeout.tv_sec = vtime.tv_sec + usecs / 1000000ULL;
    uint64_t micros(vtime.tv_usec + usecs % 1000000ULL);
    if(micros > 1000000ULL)
    {
        timeout.tv_sec++;
        micros -= 1000000ULL;
    }
    timeout.tv_nsec = static_cast<long>(micros * 1000ULL);

    err = pthread_cond_timedwait(&f_condition, &f_mutex, &timeout);
    if(err != 0)
    {
        if(err == ETIMEDOUT)
        {
            return false;
        }

        // an error occurred!
        SNAP_LOG_ERROR("a mutex conditional timed wait generated error #")(err)(" -- ")(strerror(err));
        throw snap_thread_exception_mutex_failed_error("pthread_cond_timedwait() failed");
    }

    return true;
}


/** \brief Wait on a mutex until the specified date.
 *
 * This function waits on the mutex condition to be signaled up until the
 * specified date is passed.
 *
 * \warning
 * This function cannot be called if the mutex is not locked or the
 * wait will fail in unpredicatable ways.
 *
 * \exception snap_thread_exception_mutex_failed_error
 * This exception is raised whenever the thread wait functionf fails.
 *
 * \param[in] usec  The date when the mutex times out in microseconds.
 *
 * \return true if the condition occurs before the function times out,
 *         false if the function times out.
 */
bool snap_thread::snap_mutex::dated_wait(uint64_t usec)
{
    // For any mutex wait to work, we MUST have the
    // mutex locked already and just one time.
    //
    // note: the 1 time is just for assurance that it will
    //       work in most cases; it should work even when locked
    //       multiple times, but it is less likely. For sure, it
    //       has to be at least once.
    //if(f_reference_count != 1UL)
    //{
    //    SNAP_LOG_FATAL("attempting to dated wait until ")(msec)(" msec on a mutex when it is not locked exactly once, current count is ")(f_reference_count);
    //    throw snap_thread_exception_not_locked_once_error();
    //}

    // setup the timeout date
    struct timespec timeout;
    timeout.tv_sec = static_cast<long>(usec / 1000000ULL);
    timeout.tv_nsec = static_cast<long>((usec % 1000000ULL) * 1000ULL);

    int const err(pthread_cond_timedwait(&f_condition, &f_mutex, &timeout));
    if(err != 0)
    {
        if(err == ETIMEDOUT)
        {
            return false;
        }

        // an error occurred!
        SNAP_LOG_ERROR("a mutex conditional dated wait generated error #")(err)(" -- ")(strerror(err));
        throw snap_thread_exception_mutex_failed_error("pthread_cond_timedwait() failed");
    }

    return true;
}


/** \brief Signal a mutex.
 *
 * Our mutexes include a condition that get signaled by calling this
 * function. This function wakes up one listening thread.
 *
 * The function ensures that the mutex is locked before broadcasting
 * the signal so you do not have to lock the mutex yourself.
 *
 * \exception snap_thread_exception_invalid_error
 * If one of the pthread system functions return an error, the function
 * raises this exception.
 */
void snap_thread::snap_mutex::signal()
{
    snap_lock lock_mutex(*this);

    int const err(pthread_cond_signal(&f_condition));
    if(err != 0)
    {
        SNAP_LOG_ERROR("a mutex condition signal generated error #")(err);
        throw snap_thread_exception_invalid_error("pthread_cond_signal() failed");
    }
}


/** \brief Broadcast a mutex signal.
 *
 * Our mutexes include a condition that get signaled by calling this
 * function. This function actually signals all the threads that are
 * currently listening to the mutex signal. The order in which the
 * threads get awaken is unspecified.
 *
 * The function ensures that the mutex is locked before broadcasting
 * the signal so you do not have to lock the mutex yourself.
 *
 * \exception snap_thread_exception_invalid_error
 * If one of the pthread system functions return an error, the function
 * raises this exception.
 */
void snap_thread::snap_mutex::broadcast()
{
    snap_lock lock_mutex(*this);

    int const err(pthread_cond_broadcast(&f_condition));
    if(err != 0)
    {
        SNAP_LOG_ERROR("a mutex signal broadcast generated error #")(err);
        throw snap_thread_exception_invalid_error("pthread_cond_broadcast() failed");
    }
}





/** \class snap_thread::snap_lock
 * \brief Lock a mutex in an RAII manner.
 *
 * This class is used to lock mutexes in a safe manner in regard to
 * exceptions. It is extremely important to lock all mutexes before
 * a thread quits otherwise the application will lock up.
 *
 * \code
 *    {
 *        snap_lock lock(my_mutex);
 *        ... // atomic work
 *    }
 * \endcode
 */


/** \var snap_thread::snap_lock::f_mutex
 * \brief The mutex used by the lock class.
 *
 * Whenever you want to lock a part of your code so only one thread
 * runs it at any given time, you want to use a lock. This lock
 * makes use of a mutex that you pass to it on construction.
 *
 * The snap_lock object keeps a reference to your mutex and uses
 * it to lock on construction and unlock on destruction. This
 * generates a perfect safe guard around your code. Safe guard
 * which is exception safe since it will still get unlocked when
 * an exception occurs.
 *
 * \warning
 * Note that it is not safe if you get a Unix signal. The lock
 * will very likely still be in place if such a signal happens
 * while within the lock.
 */





/** \brief Lock a mutex.
 *
 * This function locks the specified mutex and keep track of the lock
 * until the destructor is called.
 *
 * The mutex parameter cannot be a reference to a nullptr pointer.
 *
 * \param[in] mutex  The Snap! mutex to lock.
 */
snap_thread::snap_lock::snap_lock(snap_mutex & mutex)
    : f_mutex(&mutex)
{
    if(!f_mutex)
    {
        // mutex is mandatory
        //
        throw snap_thread_exception_invalid_error("mutex missing in snap_lock() constructor");
    }
    f_mutex->lock();
}


/** \brief Ensure that the mutex was unlocked.
 *
 * The destructor ensures that the mutex gets unlocked. Note that it is
 * written to avoid exceptions, however, if an exception occurs it ends
 * up calling exit(1).
 *
 * \note
 * If a function throws it logs information using the Snap! logger.
 */
snap_thread::snap_lock::~snap_lock()
{
    try
    {
        unlock();
    }
    catch(std::exception const & e)
    {
        // a log was already printed, we do not absolutely need another one
        exit(1);
    }
}


/** \brief Unlock this mutex.
 *
 * This function can be called any number of times. The first time it is
 * called, the mutex gets unlocked. Further calls (in most cases one more
 * when the destructor is called) have no effects.
 *
 * This function may throw an exception if the mutex unlock call
 * fails.
 */
void snap_thread::snap_lock::unlock()
{
    if(f_mutex != nullptr)
    {
        f_mutex->unlock();
        f_mutex = nullptr;
    }
}




/** \class snap_thread::snap_runner
 * \brief The runner is the class that wraps the actual system thread.
 *
 * This class defines the actuall thread wrapper. This is very important
 * because when the main snap_thread object gets destroyed and if it
 * were a system thread, the virtual tables would be destroyed and thus
 * invalid before you reached the ~snap_thread() destructor. This means
 * any of the virtual functions could not get called.
 *
 * For this reason we have a two level thread objects implementation:
 * the snap_thread which acts as a controller and the snap_runner which
 * is the object that is the actual system thread and thus which has the
 * run() virtual function: the function that gets called when the thread
 * starts running.
 */


/** \typedef snap_thread::snap_runner::pointer_t
 * \brief The shared pointer of a thread runner.
 *
 * This type is used to hold a smart pointer to a thread runner.
 *
 * Be very careful. Using a smart pointer does NOT mean that you can just
 * delete a snap_runner without first stopping the thread. Make sure to
 * have a snap_thread object to manage your snap_running pointers (i.e you
 * can delete a snap_thread, which will stop your snap_runner and then
 * delete the snap_runner.)
 */


/** \typedef snap_thread::snap_runner::vector_t
 * \brief A vector of threads.
 *
 * This type defines a vector of thread runners as used by the
 * snap_thread::snap_thread_pool template.
 *
 * Be careful as vectors are usually copyable and this one is because it
 * holds smart pointers to thread runners, not the actual thread. You
 * still only have one thread, just multiple instances of its pointer.
 * However, keep in mind that you can't just destroy a runner. The
 * thread it is runner must be stopped first. Please make sure to
 * have a snap_thread or a snap_thread::snap_thread_pool to manage
 * your thread runners.
 */


/** \var snap_thread::snap_runner::f_mutex
 * \brief The mutex of this thread.
 *
 * Each thread is given its own mutex so it can handle its data safely.
 *
 * This mutex is expected to mainly be used by the thread and its parent.
 *
 * If you want to share data and mutexes between multiple threads,
 * you may want to consider using another mutex. For example, the
 * snap_thread::snap_fifo is itself derived from the snap_mutex
 * class. So when you use a FIFO between multiple threads, the
 * lock/unlock mechanism is not using the mutex of your thread.
 */


/** \var snap_thread::snap_runner::f_thread
 * \brief A pointer back to the owner ("parent") of this runner
 *
 * When a snap_runner is created, it gets created by a specific \em parent
 * object. This pointer holds that parent.
 *
 * The runner uses this pointer to know whether it is still running
 * and to retrieve its identifier that the parent holds.
 */


/** \var snap_thread::snap_runner::f_name
 * \brief The name of this thread.
 *
 * Each thread is given a name. This can help greatly when debugging a
 * threaded environment with a large number of threads. That way you
 * can easily identify which thread did what and work you way to a
 * perfect software.
 *
 * On some systems it may be possible to give this name to the OS
 * which then can be displayed in tools listing processes and threads.
 */


/** \brief Initializes the runner.
 *
 * The constructor expects a name. The name is mainly used in case a
 * problem occur and we want to log a message. That way you will know
 * which thread runner caused a problem.
 *
 * \param[in] name  The name of this thread runner.
 */
snap_thread::snap_runner::snap_runner(std::string const & name)
    : f_name(name)
{
}


/** \brief The destructor checks that the thread was stopped.
 *
 * This function verifies that the thread was stopped before the
 * object gets destroyed (and is likely to break something along
 * the way.)
 */
snap_thread::snap_runner::~snap_runner()
{
    // the thread should never be set when the runner gets deleted
    if(f_thread)
    {
        // this is a bug; it could be that the object that derived from
        // the snap_runner calls gets destroyed under the thread controller's
        // nose and that could break a lot of things.
        SNAP_LOG_FATAL("The Snap! thread runner named \"")(f_name)("\" is still marked as running when its object is being destroyed.");
        exit(1);
    }
}


/** \brief Retrieve the name of the runner.
 *
 * This function returns the name of the runner as specified in the
 * constructor.
 *
 * Since the name is read-only, it will always match one to one what
 * you passed on.
 *
 * \return The name of this thread runner.
 */
std::string const & snap_thread::snap_runner::get_name() const
{
    return f_name;
}


/** \brief Check whether this thread runner is ready.
 *
 * By default a thread runner is considered ready. If you reimplement this
 * function it is possible to tell the thread controller that you are not
 * ready. This means the start() function will fail and return false.
 *
 * \return true by default, can return false to prevent a start() command.
 */
bool snap_thread::snap_runner::is_ready() const
{
    return true;
}


/** \brief Whether the thread should continue running.
 *
 * This function checks whether the user who handles the controller asked
 * the thread to quit. If so, then the function returns false. If not
 * the function returns true.
 *
 * The function can be reimplemented in your runner. In that case, the
 * runner implementation should probably call this function too in order
 * to make sure that the stop() function works.
 *
 * It is expected that your run() function implements a loop that checks
 * this flag on each iteration with iterations that take as little time
 * as possible.
 *
 * \code
 * void my_runner::run()
 * {
 *    while(continue_running())
 *    {
 *       // do some work
 *       ...
 *    }
 * }
 * \endcode
 *
 * \return true if the thread is expected to continue running.
 */
bool snap_thread::snap_runner::continue_running() const
{
    snap_lock lock(f_mutex);
    if(f_thread == nullptr)
    {
        return true;
    }
    return !f_thread->is_stopping();
}


/** \fn snap_thread::snap_runner::run();
 * \brief This virtual function represents the code run by the thread.
 *
 * The run() function is the one you have to implement in order to have
 * something to execute when the thread is started.
 *
 * To exit the thread, simply return from the run() function.
 */


/** \brief Retrieve the thread controller linked to this runner.
 *
 * Each runner is assigned a thread controller whenever the snap_thread
 * is created (they get attached, in effect.) Once the snap_thread is
 * destroyed, the pointer goes back to nullptr.
 *
 * \return A snap_thread pointer or nullptr.
 */
snap_thread * snap_thread::snap_runner::get_thread() const
{
    return f_thread;
}


/** \brief Get this runner thread identifier.
 *
 * This function returns the thread identifier of the thread running
 * this runner run() function.
 *
 * This function can be called from any thread and the correct value
 * will be returned.
 *
 * \return The thread identifier.
 */
pid_t snap_thread::snap_runner::gettid() const
{
    return f_thread->get_thread_tid();
}











/** \class snap_thread
 * \brief A thread object that ensures proper usage of system threads.
 *
 * This class is used to handle threads. It should NEVER be used, however,
 * there are some very specific cases where a thread is necessary to make
 * sure that main process doesn't get stuck. For example, the process
 * environment using pipes requires threads to read and write pipes
 * because otherwise the processes could lock up.
 */


/** \typedef snap_thread::pointer_t
 * \brief The shared pointer for a thread object.
 *
 * This type is used to hold a smart pointer to a thread.
 *
 * This smart pointer is safe. It can be used to hold a thread object and
 * when it goes out of scope, it properly ends the corresponding thread
 * runner (the snap_thread::snap_runner) and returns.
 *
 * Be cautious because the smart pointer of a snap_runner is not actually
 * safe to delete without first stopping the thread. Make sure to manage
 * all your threads in with two objects, making sure that the thread goes
 * out of scope first so it can stop your thread before your thread object
 * gets destroyed.
 */


/** \typedef snap_thread::vector_t
 * \brief A vector of threads.
 *
 * This type defines a vector of threads. Since each entry in the vector
 * is a smart pointer, it is safe to use this type.
 */


/** \brief Initialize the thread object.
 *
 * This function saves the name of the thread. The name is generally a
 * static string and it is used to distinguish between threads when
 * managing several at once. The function makes a copy of the name.
 *
 * The runner pointer is an object which has a run() function that will
 * be called from another thread. That object becomes the "child" of
 * this snap_thread controller. However, if it is already assigned a
 * thread controller, then the initialization of the snap_thread fails.
 * You may test whether a runner is already assigned a thread controller
 * by calling its get_thread() function and see that it is not nullptr.
 *
 * The pointer to the runner object cannot be nullptr.
 *
 * \param[in] name  The name of the process.
 * \param[in] runner  The runner (the actual thread) to handle.
 */
snap_thread::snap_thread(std::string const & name, snap_runner * runner)
    : f_name(name)
    , f_runner(runner)
{
    if(f_runner == nullptr)
    {
        throw snap_thread_exception_invalid_error("runner missing in snap_thread() constructor");
    }
    if(f_runner->f_thread != nullptr)
    {
        throw snap_thread_exception_in_use_error("this runner (" + name + ") is already is use");
    }

    int err(pthread_attr_init(&f_thread_attr));
    if(err != 0)
    {
        SNAP_LOG_ERROR("the thread attributes could not be initialized, error #")(err);
        throw snap_thread_exception_invalid_error("pthread_attr_init() failed");
    }
    err = pthread_attr_setdetachstate(&f_thread_attr, PTHREAD_CREATE_JOINABLE);
    if(err != 0)
    {
        SNAP_LOG_ERROR("the thread detach state could not be initialized, error #")(err);
        pthread_attr_destroy(&f_thread_attr);
        throw snap_thread_exception_invalid_error("pthread_attr_setdetachstate() failed");
    }

    f_runner->f_thread = this;
}


/** \brief Delete a thread object.
 *
 * The destructor of a Snap! C++ thread object ensures that the thread stops
 * running before actually deleting the runner object.
 *
 * Then it destroyes the thread attributes and returns.
 *
 * The destructor also removes the thread from the runner so the runner
 * can create another thread controller and run again.
 */
snap_thread::~snap_thread()
{
    try
    {
        stop();
    }
    catch(snap_thread_exception_mutex_failed_error const &)
    {
    }
    catch(snap_thread_exception_invalid_error const &)
    {
    }
    f_runner->f_thread = nullptr;

    int const err(pthread_attr_destroy(&f_thread_attr));
    if(err != 0)
    {
        SNAP_LOG_ERROR("the thread attributes could not be destroyed, error #")(err);
    }
}


/** \brief Retrieve the name of this process object.
 *
 * This process object is given a name on creation. In most cases this is
 * a static name that is used to determine which process is which.
 *
 * \return The name of the process.
 */
std::string const & snap_thread::get_name() const
{
    return f_name;
}


/** \brief Get a pointer to this thread runner.
 *
 * This function returns the pointer to the thread runner. There are cases
 * where it is quite handy to be able to use this function rather than
 * having to hold on the information in your own way.
 *
 * You will probably have to dynamic_cast<>() the result to your own
 * object type.
 *
 * \note
 * The snap_thread constructor ensures that this pointer is never nullptr.
 * Therefore this function never returns a null pointer. However, the
 * dynamic_cast<>() function may return a nullptr.
 *
 * \return The snap_runner object attached to this snap_thread.
 */
snap_thread::snap_runner * snap_thread::get_runner() const
{
    return f_runner;
}


/** \brief Check whether the thread is considered to be running.
 *
 * This flag is used to know whether the thread is running.
 *
 * \todo
 * We need to save the pid of the process that creates threads.
 * This tells us whether the thread is running in this process.
 * i.e. if a fork() happens, then the child process does not
 * have any threads so is_running() has to return false. Also,
 * any other function that checks whether a thread is running
 * needs to also check the pid. We have to save the pid at
 * the time we start the thread and then when we check whether
 * the thread is running.
 *
 * \return true if the thread is still considered to be running.
 */
bool snap_thread::is_running() const
{
    snap_lock lock(f_mutex);
    return f_running;
}


/** \brief Check whether the thread was asked to stop.
 *
 * The thread is using three status flags. One of them is f_stopping which
 * is set to false (which is also the default status) when start() is called
 * and to true when stop() is called. This function is used to read that
 * flag status from the continue_running() function.
 *
 * \return true if the stop() function was called, false otherwise.
 */
bool snap_thread::is_stopping() const
{
    snap_lock lock(f_mutex);
    return f_stopping;
}


/** \brief Start the actual thread.
 *
 * This function is called when starting the thread. This is a static
 * function since pthread can only accept such a function pointer.
 *
 * The function then calls the internal_run().
 *
 * \note
 * The function parameter is a void * instead of snap_thread because that
 * way the function signature matches the signature the pthread_create()
 * function expects.
 *
 * \param[in] thread  The thread pointer.
 *
 * \return We return a null pointer, which we do not use because we do
 *         not call the pthread_join() function.
 */
void * func_internal_start(void * thread)
{
    snap_thread * t(reinterpret_cast<snap_thread *>(thread));
    t->internal_run();
    return nullptr; // == pthread_exit(nullptr);
}


/** \brief Run the thread process.
 *
 * This function is called by the func_internal_start() so we run from
 * within the snap_thread class. (i.e. the func_internal_start() function
 * itself is static.)
 *
 * The function marks the thread as started which allows the parent start()
 * function to return.
 *
 * \note
 * The function catches standard exceptions thrown by any of the functions
 * called by the thread. When that happens, the thread returns early after
 * making a copy of the exception. The stop() function will then rethrow
 * that exception and it can be managed as if it had happened in the main
 * process (if a thread creates another thread, then it can be propagated
 * multiple times between all the threads up to the main process.)
 */
void snap_thread::internal_run()
{
    {
        snap_lock lock(f_mutex);
        f_tid = gettid();
    }

    try
    {
        {
            snap_lock lock(f_mutex);
            f_started = true;
            f_mutex.signal();
        }

        f_runner->run();

        // if useful (necessary) it would probably be better to call this
        // function from here; see function and read the "note" section
        // for additional info
        //
        //tcp_client_server::cleanup_on_thread_exit();
    }
    catch(std::exception const &)
    {
        // keep a copy of the exception
        //
        f_exception = std::current_exception();
    }
    catch(...)
    {
        // ... any other exception terminates the whole process ...
        //
        SNAP_LOG_FATAL("thread got an unknown exception (a.k.a. non-std::exception), exiting process");

        // rethrow, our goal is not to ignore the exception, only to
        // have a log about it
        //
        throw;
    }

    // marked we are done (outside of the try/catch because if this one
    // fails, we have a big problem... (i.e. invalid mutex or more unlock
    // than locks)
    {
        snap_lock lock(f_mutex);
        f_running = false;
        f_tid = -1;
        f_mutex.signal();
    }
}


/** \brief Attempt to start the thread.
 *
 * This function is used to start running the thread code. If the
 * thread is already running, then the function returns false.
 *
 * The function makes use of a condition to wait until the thread
 * is indeed started. The function will not return until the thread
 * is started or something failed.
 *
 * \return true if the thread successfully started, false otherwise.
 */
bool snap_thread::start()
{
    snap_lock lock(f_mutex);

    if(f_running || f_started)
    {
        SNAP_LOG_WARNING("the thread is already running");
        return false;
    }

    if(!f_runner->is_ready())
    {
        SNAP_LOG_WARNING("the thread runner is not ready");
        return false;
    }

    f_running = true;
    f_started = false;
    f_stopping = false; // make sure it is reset
    f_exception = std::exception_ptr();

    int const err(pthread_create(&f_thread_id, &f_thread_attr, &func_internal_start, this));
    if(err != 0)
    {
        f_running = false;

        SNAP_LOG_ERROR("the thread could not be created, error #")(err);
        return false;
    }

    while(!f_started)
    {
        f_mutex.wait();
    }

    return true;
}


/** \brief Stop the thread.
 *
 * This function requests the thread to stop. Note that the function does
 * not actually forcebly stop the thread. It only turns on a flag (namely
 * it makes the is_stopping() function return true) meaning that the
 * thread should stop as soon as possible. This gives the thread the
 * time necessary to do all necessary cleanup before quitting.
 *
 * The stop function blocks until the thread is done.
 *
 * \warning
 * This function throws the thread exceptions that weren't caught in your
 * run() function. This happens after the thread has completed.
 */
void snap_thread::stop()
{
    {
        snap_lock lock(f_mutex);

        if(!f_running && !f_started)
        {
            // we return immediately in this case because
            // there is nothing to join when the thread never
            // started...
            //
            return;
        }

        // request the child to stop
        //
        f_stopping = true;
    }

    // wait for the child to be stopped
    //
    // we cannot pass any results through the pthread interface so we
    // pass a nullptr for the result; instead, the user is expected to
    // add fields to his class and fill in whatever results he wants
    // there; it is going to work much better that way
    //
    pthread_join(f_thread_id, nullptr);

    // at this point the thread has fully exited

    // we are done now
    //
    // these flags are likely already the correct value except for
    // f_stopping which the stop() function manages here
    //
    f_running = false;
    f_started = false;
    f_stopping = false;

    // if the child died because of a standard exception, rethrow it now
    //
    if(f_exception != std::exception_ptr())
    {
        std::rethrow_exception(f_exception);
    }
}


/** \brief Retrieve the thread identifier of this thread.
 *
 * Under Linux, threads are tasks like any others. Each task is given a
 * `pid_t` value. This function returns that `pid_t` for this thread.
 *
 * When the thread is not running this function returns -1. Note, however,
 * that the value is set a little after the thread started and cleared a
 * little before the thread exists. This is **not** a good way to know
 * whether the thread is running. Use the is_running() function instead.
 *
 * \return The thread identifier (tid) or -1 if the thread is not running.
 */
pid_t snap_thread::get_thread_tid() const
{
    snap_lock lock(f_mutex);
    return f_tid;
}


/** \brief Retrieve a reference to the thread mutex.
 *
 * This function returns a reference to this thread mutex. Note that
 * the `snap_runner` has its own mutex as well.
 *
 * \return This thread's mutex.
 */
snap_thread::snap_mutex & snap_thread::get_thread_mutex() const
{
    return f_mutex;
}


/** \brief Send a signal to this thread.
 *
 * This function sends a signal to a specific thread.
 *
 * You have to be particularly careful with Unix signal and threads as they
 * do not always work as expected. This is yet particularly useful if you
 * want to send a signal such as SIGUSR1 and SIGUSR2 to a thread so it
 * reacts one way or another (i.e. you are using poll() over a socket and
 * need to be stopped without using a possibly long time out, you can use
 * the signalfd() function to transform SIGUSR1 into a pollable signal.)
 *
 * \note
 * Obviously, if the thread is not running, nothing happens.
 *
 * \param[in] sig  The signal to send to this thread.
 *
 * \return true if the signal was sent, false if the signal could not
 *         be sent (i.e. the thread was already terminated...)
 */
bool snap_thread::kill(int sig)
{
    snap_lock lock(f_mutex);
    if(f_running)
    {
        // pthread_kill() returns zero on success, otherwise it returns
        // an error code which at this point we lose
        //
        return pthread_kill(f_thread_id, sig) == 0;
    }

    return false;
}


/** \brief Retrieve the number of processors available on this system.
 *
 * This function returns the number of processors available on this system.
 *
 * \attention
 * Note that the OS may not be using all of the available processors. This
 * function returns the total number, including processors that are not
 * currently usable by your application. Most often, you probably want to
 * call get_number_of_available_processors() instead.
 *
 * \return The total number of processors on this system.
 *
 * \sa get_number_of_available_processors()
 */
int snap_thread::get_total_number_of_processors()
{
    return get_nprocs_conf();
}


/** \brief Retrieve the number of processors currently usable.
 *
 * This function returns the number of processors that are currently
 * running on this system. This is usually what you want to know about
 * to determine how many threads to run in parallel.
 *
 * This function is going to be equal or less than what the
 * get_total_number_of_processors() returns. For example, on some systems
 * a processors can be made offline. This is useful to save energy when
 * the total load decreases under a given threshold.
 *
 * \note
 * What is the right number of threads needed in a thread pool? If you
 * have worker threads that do a fairly small amount of work, then
 * having a number of threads equal to two times the number of
 * available thread is still sensible:
 *
 * \code
 *     pool_size = get_number_of_available_processors() * 2;
 * \endcode
 *
 * \par
 * This is particularly true when threads perform I/O with high latency
 * (i.e. read/write from a hard drive or a socket.)
 *
 * \par
 * If your thread does really intesive work for a while (i.e. one thread
 * working on one 4Mb image,) then the pool size should be limited to
 * one worker per CPU:
 *
 * \code
 *     pool_size = get_number_of_available_processors();
 * \endcode
 *
 * \par
 * Also, if you know that you will never get more than `n` objects at
 * a time in your input, then the maximum number of threads needed is
 * going to be `n`. In most cases `n` is going to be larger than the
 * number of processors although now that we can have machines with
 * over 100 CPUs, make sure to clamp your `pool_size` parameter to
 * `n` if known ahead of time.
 *
 * \return The number of processors currently available for your threads.
 *
 * \sa get_total_number_of_processors()
 */
int snap_thread::get_number_of_available_processors()
{
    return get_nprocs();
}


/** \brief Get the thread identifier of the current thread.
 *
 * This function retrieves the thread identifier of the current thread.
 * In most cases, this is only useful to print out messages to a log
 * including the thread identifier. This identifier is equivalent
 * to the `pid_t` returned by `getpid()` but specific to the running
 * thread.
 *
 * \return The thread identifier.
 */
pid_t snap_thread::gettid()
{
    return static_cast<pid_t>(syscall(SYS_gettid));
}

} // namespace snap
// vim: ts=4 sw=4 et
