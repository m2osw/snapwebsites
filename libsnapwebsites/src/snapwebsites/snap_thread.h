// Snap Websites Server -- advanced handling of Unix thread
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
#pragma once

/** \file
 * \brief Thread Runner and Managers.
 *
 * This file includes the declaration and implementation (For templates)
 * of classes used to manage threads the easy way. Especially, our
 * implementation is aware of object destructors so a thread manager
 * (snap_thread) can be destroyed. It will automatically and properly
 * wait for its runner (the actual system pthread) to exit before
 * finishing up its and its runner clean up.
 */

// our lib
//
#include "snapwebsites/snap_exception.h"

// snapdev lib
//
#include <snapdev/not_reached.h>

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
};

class snap_thread_exception_not_started : public snap_thread_exception
{
public:
    explicit snap_thread_exception_not_started(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_not_started(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_in_use_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_in_use_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_in_use_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_not_locked_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_not_locked_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_not_locked_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_not_locked_once_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_not_locked_once_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_not_locked_once_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_mutex_failed_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_mutex_failed_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_mutex_failed_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_invalid_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_invalid_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_invalid_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
};

class snap_thread_exception_system_error : public snap_thread_exception
{
public:
    explicit snap_thread_exception_system_error(char const *        whatmsg) : snap_thread_exception(whatmsg) {}
    explicit snap_thread_exception_system_error(std::string const & whatmsg) : snap_thread_exception(whatmsg) {}
};




class snap_thread
{
public:
    typedef std::shared_ptr<snap_thread>    pointer_t;
    typedef std::vector<pointer_t>          vector_t;


    // a mutex to ensure single threaded work
    //
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
        pthread_mutex_t     f_mutex = pthread_mutex_t();
        pthread_cond_t      f_condition = pthread_cond_t();
    };

    class snap_lock
    {
    public:
                            snap_lock(snap_mutex & mutex);
                            snap_lock(snap_lock const & rhs) = delete;
                            ~snap_lock();

        snap_lock &         operator = (snap_lock const & rhs) = delete;

        void                unlock();

    private:
        snap_mutex *        f_mutex = nullptr;
    };


    // this is the actual thread because we cannot use the main thread
    // object destructor to properly kill a thread in a C++ environment
    //
    class snap_runner
    {
    public:
        typedef std::shared_ptr<snap_runner>    pointer_t;
        typedef std::vector<pointer_t>          vector_t;

                            snap_runner(std::string const & name);
                            snap_runner(snap_runner const & rhs) = delete;
        virtual             ~snap_runner();

        snap_runner &       operator = (snap_runner const & rhs) = delete;

        std::string const & get_name() const;
        virtual bool        is_ready() const;
        virtual bool        continue_running() const;
        virtual void        run() = 0;
        snap_thread *       get_thread() const;
        pid_t               gettid() const;

    protected:
        mutable snap_mutex  f_mutex = snap_mutex();

    private:
        friend class snap_thread;

        snap_thread *       f_thread = nullptr;
        std::string const   f_name = std::string();
    };


    template<class T>
    class snap_fifo : public snap_mutex
    {
    private:
        typedef std::queue<T>   items_t;

    public:
        typedef T                               value_type;
        typedef snap_fifo<value_type>           fifo_type;
        typedef std::shared_ptr<fifo_type>      pointer_t;

        bool push_back(T const & v)
        {
            snap_lock lock_mutex(*this);
            if(f_done)
            {
                return false;
            }
            f_queue.push(v);
            signal();
            return true;
        }

        bool pop_front(T & v, int64_t const usecs)
        {
            snap_lock lock_mutex(*this);
            if(!f_done && f_queue.empty())
            {
                // when empty wait a bit if possible and try again
                //
                if(usecs == -1)
                {
                    // wait until signal() wakes us up
                    //
                    wait();
                }
                else if(usecs > 0)
                {
                    timed_wait(usecs);
                }
            }
            bool const result(!f_queue.empty());
            if(result)
            {
                v = f_queue.front();
                f_queue.pop();
            }
            if(f_done && !f_broadcast && f_queue.empty())
            {
                // make sure all the threads wake up on this new
                // "queue is empty" status
                //
                broadcast();
                f_broadcast = true;
            }
            return result;
        }

        void clear()
        {
            snap_lock lock_mutex(*this);
            items_t empty;
            f_queue.swap(empty);
        }

        bool empty() const
        {
            snap_lock lock_mutex(const_cast<snap_fifo &>(*this));
            return f_queue.empty();
        }

        size_t size() const
        {
            snap_lock lock_mutex(const_cast<snap_fifo &>(*this));
            return f_queue.size();
        }

        size_t byte_size() const
        {
            snap_lock lock_mutex(const_cast<snap_fifo &>(*this));
            return std::accumulate(
                        f_queue.begin(),
                        f_queue.end(),
                        0,
                        [](size_t accum, T const & obj)
                        {
                            return accum + obj.size();
                        });
        }

        void done(bool clear)
        {
            snap_lock lock_mutex(*this);
            f_done = true;
            if(clear)
            {
                items_t empty;
                f_queue.swap(empty);
            }
            if(f_queue.empty())
            {
                broadcast();
                f_broadcast = true;
            }
        }

        bool is_done() const
        {
            snap_lock lock_mutex(const_cast<snap_fifo &>(*this));
            return f_done;
        }

    private:
        items_t                 f_queue = items_t();
        bool                    f_done = false;
        bool                    f_broadcast = false;
    };


    /** \brief A runner augmentation allowing for worker threads.
     *
     * This class allows you to create a pool of worker threads. This is
     * useful to add/remove work in a snap_fifo object and have any one
     * worker thread pick up the next load as soon as it becomes
     * available. This is pretty much the fastest way to get work done
     * using threads, however, it really only works if you can easily
     * break down the work by chunk.
     *
     * One pool of worker threads is expected to share one pair of snap_fifo
     * objects. Also. the input and output snap_fifo objects must be of the
     * same type.
     *
     * To use your pool of threads, all you have to do is add data to the
     * input snap_fifo and grab results from the output snap_fifo. Note that
     * the output snap_fifo of one pool of threads can be the input snap_fifo
     * of another pool of threads.
     */
    template<class T>
    class snap_worker : public snap_runner
    {
    public:
        typedef T       work_load_type;

        /** \brief Initialize a worker thread.
         *
         * This function initializes a worker thread. The name should be
         * different for each worker, although there is no test to verify
         * that is the case.
         *
         * The \p in and \p out parameters are snap_fifo objects used to
         * receive work and return work that was done.
         *
         * Keep in mind that data added and removed from the snap_fifo
         * is being copied. It is suggested that you make use of a
         * shared_ptr<> to an object and not directly an object. This
         * will make the copies much more effective.
         *
         * \param[in] name  The name of this new worker thread.
         * \param[in] position  The worker thread position.
         * \param[in] in  The input FIFO.
         * \param[in] out  The output FIFO.
         */
        snap_worker<T>(std::string const & name
                     , size_t position
                     , typename snap_fifo<T>::pointer_t in
                     , typename snap_fifo<T>::pointer_t out)
            : snap_runner(name)
            , f_in(in)
            , f_out(out)
            , f_position(position)
        {
        }

        snap_worker<T>(snap_worker const & rhs) = delete;
        snap_worker<T> & operator = (snap_worker<T> const & rhs) = delete;


        /** \brief Get the worker thread position.
         *
         * Whenever the snap_thread_pool class creates a worker thread,
         * it assigns a position to it. The position is not used, but
         * it may help you when you try to debug the system.
         *
         * \return This worker thread position in the snap_thread_pool vector.
         */
        size_t position() const
        {
            return f_position;
        }


        /** \brief Check whether this specific worker thread is busy.
         *
         * This function let you know whether this specific worker thread
         * picked a work load and is currently processing it. The processing
         * includes copying the data to the output FIFO. However, there is
         * a small period of time between the time another work load object
         * is being picked up and the time it gets used that the thread is
         * not marked as working yet. So in other words, this function may
         * be lying at that point.
         *
         * \return Whether the thread is still working.
         */
        bool is_working() const
        {
            snap_lock lock(f_mutex);
            return f_working;
        }


        /** \brief Number of time this worker got used.
         *
         * This function returns the number of time this worker ended up
         * running against a work load.
         *
         * The function may return 0 if the worker never ran. If you create
         * a large pool of threads but do not have much work, this is not
         * unlikely to happen.
         *
         * \return Number of times this worker ran your do_work() function.
         */
        size_t runs() const
        {
            snap_lock lock(f_mutex);
            return f_runs;
        }


        /** \brief Implement the worker loop.
         *
         * This function is the overload of the snap_runner run() function.
         * It takes care of waiting for more data and run your process
         * by calling the do_work() function.
         *
         * You may reimplement this function if you need to do some
         * initialization or clean up as follow:
         *
         * \code
         *      virtual void run()
         *      {
         *          // initialize my variable
         *          m_my_var = allocate_object();
         *
         *          snap_worker::run();
         *
         *          // make sure to delete that resource
         *          delete_object(m_my_var);
         *      }
         * \endcode
         */
        virtual void run()
        {
            // on a re-run, f_working could be true
            {
                snap_lock lock(f_mutex);
                f_working = false;
            }

            while(continue_running())
            {
                if(f_in->pop_front(f_workload, -1))
                {
                    if(continue_running())
                    {
                        {
                            snap_lock lock(f_mutex);
                            f_working = true;
                            ++f_runs;
                        }

                        // note: if do_work() throws, then f_working remains
                        //       set to 'true' which should not matter
                        //
                        if(do_work())
                        {
                            f_out->push_back(f_workload);
                        }

                        {
                            snap_lock lock(f_mutex);
                            f_working = false;
                        }
                    }
                }
                else
                {
                    // if the FIFO is empty and it is marked as done, we
                    // want to exit immediately
                    //
                    if(f_in->is_done())
                    {
                        break;
                    }
                }
            }
        }

        /** \brief Worker Function.
         *
         * This function is your worker function which will perform work
         * against the work load automatically retrieved in the run()
         * function.
         *
         * Your load is available in the f_workload variable member.
         * You are free to modify it. The snap_worker object ignores
         * its content. It retrieved it from the input fifo (f_in)
         * and will save it in the output fifo once done (f_out).
         */
        virtual bool do_work() = 0;

    protected:
        T                       f_workload = T();
        typename snap_fifo<T>::pointer_t f_in;
        typename snap_fifo<T>::pointer_t f_out;

    private:
        size_t const            f_position;
        bool                    f_working = false;
        size_t                  f_runs = 0;
    };

    /** \brief Manage a pool of worker threads.
     *
     * This function manages a pool of worker threads. It allocates the
     * threads, accepts incoming data, and returns outgoing data. Everything
     * else is managed for you.
     *
     * To make use of this template, you need to overload the snap_worker
     * template and implement your own snap_worker::do_work() function.
     *
     * Something like this:
     *
     * \code
     *      struct data
     *      {
     *          std::string f_func = "counter";
     *          int         f_counter = 0;
     *      };
     *
     *      class foo
     *          : public snap_worker<data>
     *      {
     *          void do_work()
     *          {
     *              if(f_workload.f_func == "counter")
     *              {
     *                  ++f_workload.f_counter;
     *              }
     *              else if(f_workload.f_func == "odd-even")
     *              {
     *                  f_workload.f_counter ^= 1;
     *              }
     *              // ...etc...
     *          }
     *      };
     *
     *      snap_thread_pool<foo> pool("my pool", 10);
     *
     *      // generate input
     *      data data_in;
     *      ...fill data_in...
     *      pool.push_back(data_in);
     *
     *      // retrieve output
     *      data data_out;
     *      if(pool.pop_front(data_out))
     *      {
     *          ...use data_out...
     *      }
     * \endcode
     *
     * You can do all the push_data() you need before doing any pop_front().
     * You may also want to consider looping with both interleaved or even
     * have two threads: one feeder which does the push_data() and one
     * consumer which does the pop_front().
     *
     * Note that the thread pool does not guarantee order of processing.
     * You must make sure that each individual chunk of data you pass in
     * the push_front() function can be processed in any order.
     */
    template<class W, class ...A>
    class snap_thread_pool
    {
    public:
        typedef std::shared_ptr<snap_thread_pool<W, A...>>  pointer_t;
        typedef typename W::work_load_type                  work_load_type;
        typedef snap_fifo<work_load_type>                   worker_fifo_t;

    private:
        /** \brief Class used to manage the worker and worker thread.
         *
         * This class creates a worker thread, it adds it to a thread,
         * and it starts the thread. It is here so we have a single list
         * of _worker threads_.
         *
         * \note
         * I had to allocate a snap_thread object because at this point
         * the snap_thread is not yet fully defined so it would not take
         * it otherwise. It certainly would be possible to move this
         * declaration and those depending on it to avoid this problem,
         * though.
         */
        class worker_thread_t
        {
        public:
            typedef std::shared_ptr<worker_thread_t>    pointer_t;
            typedef std::vector<pointer_t>              vector_t;

            worker_thread_t(std::string const & name
                          , size_t i
                          , typename worker_fifo_t::pointer_t in
                          , typename worker_fifo_t::pointer_t out
                          , A... args)
                : f_worker(name + " (worker #" + std::to_string(i) + ")"
                         , i
                         , in
                         , out
                         , args...)
                , f_thread(std::make_shared<snap_thread>(name, &f_worker))
            {
                f_thread->start();
            }

            W & get_worker()
            {
                return f_worker;
            }

            W const & get_worker() const
            {
                return f_worker;
            }

        private:
            W                       f_worker;   // runner before thread; this is safe
            snap_thread::pointer_t  f_thread;
        };


    public:
        /** \brief Initializes a pool of worker threads.
         *
         * This constructor is used to initialize the specified number
         * of worker threads in this pool.
         *
         * At this time, all the worker threads get allocated upfront.
         *
         * \todo
         * We may want to add more threads as the work load increases
         * which could allow for much less unused threads created. To
         * do so we want to (1) time the do_work() function to get
         * an idea of how long a thread takes to perform its work;
         * and (2) have a threshold so we know when the client wants
         * to create new worker threads if the load increases instead
         * of assuming it should happen as soon as data gets added
         * to the input FIFO. The \p pool_size parameter would then
         * become a \p max_pool_size.
         *
         * \param[in] name  The name of the pool.
         * \param[in] pool_size  The number of threads to create.
         * \param[in] in  The input FIFO (where workers receive workload.)
         * \param[in] out The output FIFO (where finished work is sent.)
         * \param[in] args  Extra arguments to initialize the workers.
         */
        snap_thread_pool<W, A...>(std::string const & name
                             , size_t pool_size
                             , typename worker_fifo_t::pointer_t in
                             , typename worker_fifo_t::pointer_t out
                             , A... args)
            : f_name(name)
            , f_in(in)
            , f_out(out)
        {
            if(pool_size == 0)
            {
                throw snap_thread_exception_invalid_error("the pool size must be a positive number (1 or more)");
            }
            if(pool_size > 1000)
            {
                throw snap_thread_exception_invalid_error("pool size too large (we accept up to 1000 at this time, which is already very very large!)");
            }
            for(size_t i(0); i < pool_size; ++i)
            {
                f_workers.push_back(std::make_shared<worker_thread_t>(
                              f_name
                            , i
                            , f_in
                            , f_out
                            , args...));
            }
        }


        /** \brief Make sure that the thread pool is cleaned up.
         *
         * The destructor of the snap thread pool stops all the threads
         * and then waits on them. Assuming that your code doesn't loop
         * forever, the result is that all the threads are stopped, joined,
         * and all the incoming and outgoing data was cleared.
         *
         * If you need to get the output data, then make sure to call
         * stop(), wait(), and pop_front() until it returns false.
         * Only then can you call the destructor.
         *
         * You are safe to run that stop process in your own destructor.
         */
        ~snap_thread_pool<W, A...>()
        {
            stop(false);
            wait();
        }


        /** \brief Retrieve the number of workers.
         *
         * This function returns the number of workers this thread pool
         * is handling. The number is at least 1 as a thread pool can't
         * currently be empty.
         *
         * \return The number of workers in this thread pool.
         */
        size_t size() const
        {
            return f_workers.size();
        }


        /** \brief Get worker at index `i`.
         *
         * This function returns a reference to the worker at index `i`.
         *
         * \exception std::range_error
         * If the index is out of range (negative or larger or equal to
         * the number of workers) then this exception is raised.
         *
         * \param[in] i  The index of the worker to retrieve.
         *
         * \return A reference to worker `i`.
         */
        W & get_worker(int i)
        {
            if(static_cast<size_t>(i) >= f_workers.size())
            {
                throw std::range_error("snap::snap_thread::snap_thread_pool::get_worker() called with an index out of bounds.");
            }
            return f_workers[i]->get_worker();
        }


        /** \brief Get worker at index `i` (constant version).
         *
         * This function returns a reference to the worker at index `i`.
         *
         * \exception std::range_error
         * If the index is out of range (negative or larger or equal to
         * the number of workers) then this exception is raised.
         *
         * \param[in] i  The index of the worker to retrieve.
         *
         * \return A reference to worker `i`.
         */
        W const & get_worker(int i) const
        {
            if(static_cast<size_t>(i) >= f_workers.size())
            {
                throw std::range_error("snap::snap_thread::snap_thread_pool::get_worker() called with an index out of bounds.");
            }
            return f_workers[i]->get_worker();
        }


        /** \brief Push one work load of data.
         *
         * This function adds a work load of data to the input. One of the
         * worker threads will automatically pick up that work and have
         * its snap_worker::do_work() function called.
         *
         * \param[in] v  The work load of data to add to this pool.
         *
         * \sa pop_front()
         */
        void push_back(work_load_type const & v)
        {
            f_in.push_back(v);
        }


        /** \brief Retrieve one work load of processed data.
         *
         * This function retrieves one T object from the output FIFO.
         *
         * The \p usecs parameter can be set to -1 to wait until output
         * is received. Set it to 0 to avoid waiting (check for data,
         * if nothing return immediately) and a positive number to wait
         * up to that many microseconds.
         *
         * Make sure to check the function return value. If false, the
         * \p v parameter is unmodified.
         *
         * Once you called the stop() function, the pop_front() function
         * can still be called until the out queue is emptied. The proper
         * sequence is to 
         *
         * \param[in] v  A reference where the object gets copied.
         * \param[in] usecs  The number of microseconds to wait.
         *
         * \return true if an object was retrieved.
         *
         * \sa snap_mutex::pop_front()
         */
        bool pop_front(work_load_type & v, int64_t usecs)
        {
            if(f_in.is_done())
            {
                usecs = 0;
            }
            return f_out.pop_front(v, usecs);
        }


        /** \brief Stop the threads.
         *
         * This function is called to ask all the threads to stop.
         *
         * When the \p immediate parameter is set to true, whatever is
         * left in the queue gets removed. This means you are likely to
         * get invalid or at least incomplete results in the output.
         * It should only be used if the plan is to cancel the current
         * work process and trash everything away.
         *
         * After a call to the stop() function, you may want to retrieve
         * the last bits of data with the pop_front() function until
         * it returns false and then call the join() function to wait
         * on all the threads and call pop_front() again to make sure
         * that you got all the output.
         *
         * You may also call the join() function right after stop()
         * and then the pop_front().
         *
         * \note
         * It is not necessary to call pop_front() if you are cancelling
         * the processing anyway. You can just ignore that data and
         * it will be deleted as soon as the thread pool gets deleted.
         *
         * \note
         * The function can be called any number of times. It really only
         * has an effect on the first call, though.
         *
         * \param[in] immediate  Whether the clear the remaining items on
         *                       the queue.
         */
        void stop(bool immediate)
        {
            if(!f_in->is_done())
            {
                f_in->done(immediate);
            }
        }


        /** \brief Wait on the threads to be done.
         *
         * This function waits on all the worker threads until they all
         * exited. This ensures that all the output (see the pop_front()
         * function) was generated and you can therefore end your
         * processing.
         *
         * The order of processing is as follow:
         *
         * \code
         *     snap_thread_pool my_pool;
         *
         *     ...initialization...
         *
         *     data_t in;
         *     data_t out;
         *
         *     while(load_data(in))
         *     {
         *         pool.push_back(in);
         *
         *         while(pool.pop_front(out))
         *         {
         *             // handle out, maybe:
         *             save_data(out);
         *         }
         *     }
         *
         *     // no more input
         *     pool.stop();
         *
         *     // optionally, empty the output pipe before the wait()
         *     while(pool.pop_front(out))
         *     {
         *         // handle out...
         *     }
         *
         *     // make sure the workers are done
         *     pool.wait();
         *
         *     // make sure the output is all managed
         *     while(pool.pop_front(out))
         *     {
         *         // handle out...
         *     }
         *
         *     // now we're done
         * \endcode
         *
         * Emptying the output queue between the stop() and wait() is not
         * required. It may always be empty or you will anyway get only
         * a few small chunks that can wait in the buffer.
         *
         * \note
         * The function can be called any number of times. After the first
         * time, though, the vector of worker threads is empty so really
         * nothing happens.
         *
         * \attention
         * This function can't be called from one of the workers.
         */
        void wait()
        {
            f_workers.clear();
        }


    private:
        typedef typename worker_thread_t::vector_t  workers_t;

        std::string const                   f_name;
        typename worker_fifo_t::pointer_t   f_in;
        typename worker_fifo_t::pointer_t   f_out;
        workers_t                           f_workers = workers_t();
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
         * the thread life desrtuctor from calling the stop() function and
         * waiting for the thread to be done.
         *
         * \note
         * If possible, you should consider using the snap_thread::pointer_t
         * instead of the snap_thread_life which expects a bare pointer.
         * There are situations, though, where this class is practical
         * because you have a thread as a variable member in a class.
         *
         * \param[in] thread  The thread which life is to be controlled.
         */
        snap_thread_life(snap_thread * const thread)
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

        snap_thread_life(snap_thread_life const & rhs) = delete;

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

        snap_thread_life & operator = (snap_thread_life const & rhs) = delete;

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

                                snap_thread(std::string const & name, snap_runner * runner);
                                ~snap_thread();
                                snap_thread(snap_thread const & rhs) = delete;
                                snap_thread & operator = (snap_thread const & rhs) = delete;

    std::string const &         get_name() const;
    snap_runner *               get_runner() const;
    bool                        is_running() const;
    bool                        is_stopping() const;
    bool                        start();
    void                        stop();
    bool                        kill(int sig);
    pid_t                       get_thread_tid() const;
    snap_mutex &                get_thread_mutex() const;

    static int                  get_total_number_of_processors();
    static int                  get_number_of_available_processors();
    static pid_t                gettid();

private:
    // internal function to start the runner
    friend void *               func_internal_start(void * thread);
    void                        internal_run();

    std::string const           f_name = std::string();
    snap_runner *               f_runner = nullptr;
    mutable snap_mutex          f_mutex = snap_mutex();
    bool                        f_running = false;
    bool                        f_started = false;
    bool                        f_stopping = false;
    pid_t                       f_tid = -1;
    pthread_t                   f_thread_id = -1;
    pthread_attr_t              f_thread_attr = pthread_attr_t();
    std::exception_ptr          f_exception = std::exception_ptr();
};

} // namespace snap
// vim: ts=4 sw=4 et
