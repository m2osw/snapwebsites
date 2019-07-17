// Snap Websites Server -- test to choose the container for the snap FIFO
// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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

//
// Test whether the == is faster than the endsWith() function of a QtString().
// Because we were using endsWith() a lot and to optimize, not using it
// wherever possible is a good idea (i.e. if you do it a lot.)
//


#include <algorithm>
#include <deque>
#include <memory>
#include <queue>
#include <vector>
#include <iostream>


template<class T>
class snap_fifo_vector
{
private:
    typedef std::vector<T>   items_t;

public:
    typedef T                               value_type;
    typedef snap_fifo_vector<value_type>    fifo_type;
    typedef std::shared_ptr<fifo_type>      pointer_t;

    bool push_back(T const & v)
    {
        if(f_done)
        {
            return false;
        }
        f_queue.push_back(v);
        return true;
    }

    bool pop_front(T & v, int64_t const /*usecs*/)
    {
        bool const result(!f_queue.empty());
        if(result)
        {
            v = f_queue.front();
            f_queue.erase(f_queue.begin());
        }
        if(f_done && !f_broadcast && f_queue.empty())
        {
            // make sure all the threads wake up on this new
            // "queue is empty" status
            //
            f_broadcast = true;
        }
        return result;
    }

    void clear()
    {
        items_t empty;
        f_queue.swap(empty);
    }

    bool empty() const
    {
        return f_queue.empty();
    }

    size_t size() const
    {
        return f_queue.size();
    }

    size_t byte_size() const
    {
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
        f_done = true;
        if(clear)
        {
            items_t empty;
            f_queue.swap(empty);
        }
        if(f_queue.empty())
        {
            f_broadcast = true;
        }
    }

    bool is_done() const
    {
        return f_done;
    }

private:
    items_t                 f_queue = items_t();
    bool                    f_done = false;
    bool                    f_broadcast = false;
};


template<class T>
class snap_fifo_deque
{
private:
    typedef std::deque<T>   items_t;

public:
    typedef T                               value_type;
    typedef snap_fifo_deque<value_type>           fifo_type;
    typedef std::shared_ptr<fifo_type>      pointer_t;

    bool push_back(T const & v)
    {
        if(f_done)
        {
            return false;
        }
        f_queue.push_back(v);
        return true;
    }

    bool pop_front(T & v, int64_t const /*usecs*/)
    {
        bool const result(!f_queue.empty());
        if(result)
        {
            v = f_queue.front();
            f_queue.pop_front();
        }
        if(f_done && !f_broadcast && f_queue.empty())
        {
            // make sure all the threads wake up on this new
            // "queue is empty" status
            //
            f_broadcast = true;
        }
        return result;
    }

    void clear()
    {
        items_t empty;
        f_queue.swap(empty);
    }

    bool empty() const
    {
        return f_queue.empty();
    }

    size_t size() const
    {
        return f_queue.size();
    }

    size_t byte_size() const
    {
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
        f_done = true;
        if(clear)
        {
            items_t empty;
            f_queue.swap(empty);
        }
        if(f_queue.empty())
        {
            f_broadcast = true;
        }
    }

    bool is_done() const
    {
        return f_done;
    }

private:
    items_t                 f_queue = items_t();
    bool                    f_done = false;
    bool                    f_broadcast = false;
};


template<class T>
class snap_fifo_queue
{
private:
    typedef std::queue<T>   items_t;

public:
    typedef T                               value_type;
    typedef snap_fifo_queue<value_type>     fifo_type;
    typedef std::shared_ptr<fifo_type>      pointer_t;

    bool push_back(T const & v)
    {
        if(f_done)
        {
            return false;
        }
        f_queue.push(v);
        return true;
    }

    bool pop_front(T & v, int64_t const /*usecs*/)
    {
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
            f_broadcast = true;
        }
        return result;
    }

    void clear()
    {
        items_t empty;
        f_queue.swap(empty);
    }

    bool empty() const
    {
        return f_queue.empty();
    }

    size_t size() const
    {
        return f_queue.size();
    }

    size_t byte_size() const
    {
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
        f_done = true;
        if(clear)
        {
            items_t empty;
            f_queue.swap(empty);
        }
        if(f_queue.empty())
        {
            f_broadcast = true;
        }
    }

    bool is_done() const
    {
        return f_done;
    }

private:
    items_t                 f_queue = items_t();
    bool                    f_done = false;
    bool                    f_broadcast = false;
};


int
main(int argc, char * argv[])
{
    bool use_vector(false), use_deque(false), use_queue(false);

    for(int i(1); i < argc; ++i)
    {
        if(std::string(argv[i]) == "--vector")
        {
            use_vector = true;
        }
        else if(std::string(argv[i]) == "--deque")
        {
            use_deque = true;
        }
        else if(std::string(argv[i]) == "--queue")
        {
            use_queue = true;
        }
        else
        {
            std::cerr << "error: unknown option " << argv[i] << std::endl;
            return 1;
        }
    }

    if((use_vector ? 1 : 0)
     + (use_deque  ? 1 : 0)
     + (use_queue  ? 1 : 0) != 1)
    {
        std::cerr << "error: choose exactly one of --vector or --deque or --queue" << std::endl;
        return 1;
    }

    if(use_vector)
    {
        snap_fifo_vector<uint32_t>  fifo;

std::cout << "vector\n";

        for(int j(0); j < 100000; ++j)
        {
            fifo.push_back(rand());
        }

        while(!fifo.empty())
        {
            uint32_t v;
            fifo.pop_front(v, 0);
        }
    }
    else if(use_deque)
    {
        snap_fifo_deque<uint32_t>   fifo;

std::cout << "deque\n";

        for(int j(0); j < 100000; ++j)
        {
            fifo.push_back(rand());
        }

        while(!fifo.empty())
        {
            uint32_t v;
            fifo.pop_front(v, 0);
        }
    }
    else if(use_queue)
    {
        snap_fifo_queue<uint32_t>   fifo;

std::cout << "queue\n";

        for(int j(0); j < 100000; ++j)
        {
            fifo.push_back(rand());
        }

        while(!fifo.empty())
        {
            uint32_t v;
            fifo.pop_front(v, 0);
        }
    }

    return 0;
}



// vim: ts=4 sw=4 et
