// Snap Websites Server -- advanced handling of Unix thread
// Copyright (C) 2013-2017  Made to Order Software Corp.
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
#pragma once

// our lib
//
#include "snapwebsites/not_reached.h"
#include "snapwebsites/snap_exception.h"

// C++ lib
//
#include <memory>
#include <numeric>
#include <queue>

// C lib
//
#include <sys/time.h>


namespace snap
{

class snap_thread_exception : public snap_exception
{
public:
    explicit snap_thread_exception(char const *        whatmsg) : snap_exception("snap_thread", whatmsg) {}
    explicit snap_thread_exception(std::string const & whatmsg) : snap_exception("snap_thread", whatmsg) {}
    explicit snap_thread_exception(QString const &     whatmsg) : snap_exception("snap_thread", whatmsg) {}
};

class snap_thread_exception_not_started : public snap_thread_exception
{
public:
    explicit snap_thread_exception_not_started(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_not_started(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_not_started(QString const &     whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_in_use_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_in_use_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_in_use_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_in_use_error(QString const &     whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_not_locked_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_not_locked_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_not_locked_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_not_locked_error(QString const &     whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_not_locked_once_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_not_locked_once_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_not_locked_once_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_not_locked_once_error(QString const &     whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_mutex_failed_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_mutex_failed_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_mutex_failed_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_mutex_failed_error(QString const &     whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_invalid_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_invalid_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_invalid_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_invalid_error(QString const &     whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_system_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_system_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_system_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_system_error(QString const &     whatmsg) : snap_thread_exception(whatmsg) {}
};


class snap_thread
{
public:
    typedef std::shared_ptr<snap_thread>    pointer_t;

    // a mutex to ensure single threaded work
    class snap_mutex
    {
    public:
        typedef std::shared_ptr<snap_mutex>     pointer_t;

                            snap_mutex();
                            ~snap_mutex();

        void                lock();
        bool                try_lock();
        void                unlock();
        void                wait();
        bool                timed_wait(uint64_t const usec);
        bool                dated_wait(uint64_t const usec);
        void                signal();
        void                broadcast();

    private:
        uint32_t            f_reference_count = 0;
        pthread_mutex_t     f_mutex;
        pthread_cond_t      f_condition;
    };

    class snap_lock
    {
    public:
                            snap_lock(snap_mutex & mutex);
                            ~snap_lock();

        void                unlock();

    private:
        snap_mutex *        f_mutex = nullptr;
    };

    // this is the actual thread because we cannot use the main thread
    // object destructor to properly kill a thread in a C++ environment
    class snap_runner
    {
    public:
                            snap_runner(QString const & name);
        virtual             ~snap_runner();

        virtual bool        is_ready() const;
        virtual bool        continue_running() const;
        virtual void        run() = 0;
        snap_thread *       get_thread() const;

    protected:
        mutable snap_mutex  f_mutex;

    private:
        friend class snap_thread;

        snap_thread *       f_thread = nullptr;
        QString const       f_name;
    };


    /** \brief Create a thread safe FIFO.
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
     * \param T  the type of data that the FIFO will handle.
     */
    template<class T>
    class snap_fifo : public snap_mutex
    {
    public:
        /** \brief Push data on this FIFO.
         *
         * This function appends data on the FIFO queue. The function
         * has the side effect to wake up another thread if such is
         * currently waiting for data on the same FIFO.
         *
         * \param[in] v  The value to be pushed on the FIFO queue.
         */
        void push_back(T const & v)
        {
            snap_lock lock_mutex(*this);
            f_stack.push(v);
            signal();
        }


        /** \brief Retrieve one value from the FIFO.
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
        bool pop_front(T & v, int64_t const usecs)
        {
            snap_lock lock_mutex(*this);
            if(f_stack.empty())
            {
                // when empty wait a bit if possible and try again
                if(usecs == -1)
                {
                    // wait forever
                    wait();
                }
                else if(usecs <= 0) // not including -1!
                {
                    // do not wait at all
                    return false;
                }
                else //if(usecs > 0)
                {
                    if(!timed_wait(usecs))
                    {
                        // we timed out
                        //return false; -- we may have missed the signal
                        //                 so still check the stack
                    }
                }
                if(f_stack.empty())
                {
                    // stack is still empty...
                    return false;
                }
            }
            v = f_stack.front();
            f_stack.pop();
            return true;
        }


        /** \brief Test whether the FIFO is empty.
         *
         * This function checks whether the FIFO is empty and if so
         * returns true, otherwise it returns false.
         *
         * The function does not check the semaphore. Instead it
         * checks the size of the FIFO itself.
         *
         * \return true if the FIFO is empty.
         */
        bool empty() const
        {
            snap_lock lock_mutex(const_cast<snap_fifo &>(*this));
            return f_stack.empty();
        }


        /** \brief Return the number of items in the stack.
         *
         * This function returns the number of items currently added to
         * the stack. This can be used by the caller to avoid flooding
         * the stack, if at all possible.
         *
         * \return the number of items.
         */
        size_t size() const
        {
            snap_lock lock_mutex(const_cast<snap_fifo &>(*this));
            return f_stack.size();
        }


        /** \brief Return the total size of the stack.
         *
         * This function returns the sum of each element size() function.
         *
         * \note
         * Note that this does not include the amount of bytes used by
         * the stack itself. It only includes the size of the elements,
         * which in most cases is what you want anyway.
         *
         * \return the byte size of the stack.
         */
        size_t byte_size() const
        {
            snap_lock lock_mutex(const_cast<snap_fifo &>(*this));
            return std::accumulate(
                        f_stack.begin(),
                        f_stack.end(),
                        0,
                        [](size_t accum, T const & obj)
                        {
                            return accum + obj.size();
                        });
        }


    private:
        /** \brief The type of the FIFO.
         *
         * This typedef declaration defines the type of items in this
         * FIFO object.
         */
        typedef std::queue<T>   items_t;


        /** \brief The actual FIFO.
         *
         * This variable member holds the actual data in this FIFO
         * object.
         */
        items_t                 f_stack;
    };

    class snap_thread_life
    {
    public:
        /** \brief Initialize a "thread life" object.
         *
         * This type of objects are used to record a thread and make sure
         * that it gets destroyed once done with it.
         *
         * The constructor makes sure that the specified thread is not
         * a null pointer and it starts the thread. If the thread is
         * already running, then the constructor will throw.
         *
         * Once such an object was created, it is not possible to prevent
         * the thread life desrtuctor from call the stop() function and
         * waiting for the thread to be done.
         *
         * \param[in] thread  The thread which life is to be controlled.
         */
        snap_thread_life( snap_thread * const thread )
            : f_thread(thread)
        {
            if(f_thread == nullptr)
            {
                throw snap_logic_exception("snap_thread_life pointer is nullptr");
            }
            if(!f_thread->start())
            {
                // we cannot really just generate an error if the thread
                // does not start because we do not offer a way for the
                // user to know so we have to throw for now
                throw snap_thread_exception_not_started("somehow the thread was not started, an error should have been logged");
            }
        }

        /** \brief Make sure the thread stops.
         *
         * This function requests that the attach thread stop. It will block
         * until such happens. You are responsible to make sure that the
         * stop happens early on if your own object needs to access the
         * thread while stopping.
         */
        ~snap_thread_life()
        {
            //SNAP_LOG_TRACE() << "stopping snap_thread_life...";
            f_thread->stop();
            //SNAP_LOG_TRACE() << "snap_thread_life stopped!";
        }

    private:
        /** \brief The pointer to the thread being managed.
         *
         * This pointer is a pointer to the thread. Once a thread life
         * object is initialized, the pointer is never nullptr (we
         * throw before in the constructor if that is the case.)
         *
         * The user of the snap_thread_life class must view the
         * thread pointer as owned by the snap_thread_life object
         * (similar to a smart pointer.)
         */
        snap_thread *           f_thread = nullptr;
    };

                                snap_thread(QString const & name, snap_runner * runner);
                                ~snap_thread();
                                snap_thread(snap_thread const & ) = delete;
                                snap_thread & operator = (snap_thread const & ) = delete;

    QString const &             get_name() const;
    bool                        is_running() const;
    bool                        is_stopping() const;
    bool                        start();
    void                        stop();
    bool                        kill(int sig);

private:
    // internal function to start the runner
    friend void *               func_internal_start(void * thread);
    void                        internal_run();

    QString const               f_name;
    snap_runner *               f_runner;
    mutable snap_mutex          f_mutex;
    bool                        f_running = false;
    bool                        f_started = false;
    bool                        f_stopping = false;
    pthread_t                   f_thread_id = -1;
    pthread_attr_t              f_thread_attr;
    std::exception_ptr          f_exception;
};

} // namespace snap
// vim: ts=4 sw=4 et
