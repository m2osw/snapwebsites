/*
 * Text:
 *      snapwebsites/snaplistd/tests/snap_listd.cpp
 *
 * Description:
 *      Test the snaplistd daemon to make sure that the system works
 *      as expected.
 *
 * Documentation:
 *      The test is not currently implemented.
 *
 * License:
 *      Copyright (c) 2013-2017 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// snapwebsites lib
//
#include <snapwebsites/snap_listd.h>
#include <snapwebsites/snapwebsites.h>

// C++ lib
//
#include <iostream>

// C lib
//
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    std::cout << "+ snap version" << snap::server::version() << std::endl;

    int process_count(3);
    int repeat(3);
    int obtention_timeout(snap::snap_lock::SNAP_LOCK_DEFAULT_TIMEOUT);
    int duration_timeout(snap::snap_lock::SNAP_LOCK_DEFAULT_TIMEOUT);
    const char * mysql_host("127.0.0.1:3306"); // address and port to a Cassandra node
    const char * communicator_host("127.0.0.1:4040"); // address and port to the snapcommunicator TCP connection
    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "-h") == 0
        || strcmp(argv[i], "--help") == 0)
        {
            std::cout << "Usage: " << argv[0] << " [--help|-h] [--mysql-host <IP:port>] [--communicator <IP:port>] [-i <count>] [-n <repeat>] [-t <timeout>]" << std::endl;
            std::cout << "  where:" << std::endl;
            std::cout << "    --help | -h    print out this help screen" << std::endl;
            std::cout << "    --mysql-host   indicates the MySQL server IP address, you may also include the port (127.0.0.1:3306 by default)" << std::endl;
            std::cout << "    --communicator indicates the snapcommunicator IP address, you may also include the port (127.0.0.1:4040 by default)" << std::endl;
            std::cout << "    -i             indicates the number of process to spawn total (parallel execution on a single computer)" << std::endl;
            std::cout << "    -n             indicates the number of time each process will increment the counter" << std::endl;
            std::cout << "    -o             change the obtention timeout from the default (" << snap::snap_lock::SNAP_LOCK_DEFAULT_TIMEOUT << ") to this value" << std::endl;
            std::cout << "    -t             change the duration timeout from the default (" << snap::snap_lock::SNAP_LOCK_DEFAULT_TIMEOUT << ") to this value" << std::endl;
            std::cout << "To run the test you need to run snapinit and make sure snapcommunicator" << std::endl;
            std::cout << "and snaplock are both running. Then you can run this test:" << std::endl;
            std::cout << "  tests/test_snap_lock -i 4 -n 60" << std::endl;
            exit(1);
        }
        if(strcmp(argv[i], "--mysql-host") == 0)
        {
            ++i;
            if(i >= argc)
            {
                std::cerr << "error: --mysql-host must be followed by an address and optionally a port (127.0.0.1:9042)." << std::endl;
                exit(1);
            }
            mysql_host = argv[i];
        }
        else if(strcmp(argv[i], "--communicator") == 0)
        {
            ++i;
            if(i >= argc)
            {
                std::cerr << "error: --communicator must be followed by an address and optionally a port (127.0.0.1:4040)." << std::endl;
                exit(1);
            }
            communicator_host = argv[i];
        }
        else if(strcmp(argv[i], "-i") == 0)
        {
            ++i;
            if(i >= argc)
            {
                std::cerr << "error: -i must be followed by the number of processes." << std::endl;
                exit(1);
            }
            process_count = atol(argv[i]);
        }
        else if(strcmp(argv[i], "-n") == 0)
        {
            ++i;
            if(i >= argc)
            {
                std::cerr << "error: -n must be followed by the number of time each process repeats the procedure." << std::endl;
                exit(1);
            }
            repeat = atol(argv[i]);
        }
        else if(strcmp(argv[i], "-o") == 0)
        {
            ++i;
            if(i >= argc)
            {
                std::cerr << "error: -o must be followed by the number of seconds before the obtention of a lock times out." << std::endl;
                exit(1);
            }
            obtention_timeout = atol(argv[i]);
        }
        else if(strcmp(argv[i], "-t") == 0)
        {
            ++i;
            if(i >= argc)
            {
                std::cerr << "error: -t must be followed by the number of seconds before a lock times out." << std::endl;
                exit(1);
            }
            duration_timeout = atol(argv[i]);
        }
    }

    if(process_count < 1)
    {
        std::cerr << "error: -i must be specified and followed by a valid decimal number larger than 0" << std::endl;
        exit(1);
    }
    if(process_count > 100)
    {
        std::cerr << "error: -i must be followed by a valid decimal number up to 100" << std::endl;
        exit(1);
    }

    if(repeat < 1)
    {
        std::cerr << "error: -n must be specified and followed by a valid decimal number larger than 0" << std::endl;
        exit(1);
    }
    if(repeat > 1000)
    {
        std::cerr << "error: -n must be followed by a number smaller or equal to 1,000" << std::endl;
        exit(1);
    }

    snap::snap_lock::initialize_lock_duration_timeout(duration_timeout);
    snap::snap_lock::initialize_lock_obtention_timeout(obtention_timeout);

    QString communicator_addr("127.0.0.1");
    int communicator_port(4040);
    try
    {
        tcp_client_server::get_addr_port(communicator_host, communicator_addr, communicator_port, "tcp");
    }
    catch(tcp_client_server::tcp_client_server_parameter_error const & e)
    {
        std::cerr << "tcp_client_server::tcp_client_server_parameter_error exception occurred in get_addr_port(): " << e.what() << std::endl;
        exit(1);
    }
    catch(std::exception const & e)
    {
        std::cerr << "!!! exception [" << getpid() << "]: " << e.what() << std::endl;
        exit(1);
    }
    snap::snap_lock::initialize_snapcommunicator(communicator_addr.toUtf8().data(), communicator_port);

    std::cout << "+ Starting test with " << process_count << " processes and repeat the lock " << repeat << " times" << std::endl;

    std::vector<pid_t> children;
    for(int i(0); i < process_count; ++i)
    {
        pid_t const child(fork());
        if(child == 0)
        {
            try
            {
                // the child connects to Cassandra
                QString mysql_addr("127.0.0.1");
                int mysql_port(3306);
                tcp_client_server::get_addr_port(mysql_host, mysql_addr, mysql_port, "tcp");
                Session::pointer_t cassandra_session(Session::create());
                cassandra_session->connect(cassandra_addr, cassandra_port);
                std::cout << "+ Cassandra Cluster for child " << getpid() << " ready." << std::endl;

                for(int r(0); r < repeat; ++r)
                {
                    sleep(1);

                    {
                        snap::snap_lock lock("test-snap-lock");

                        // got the lock!
                        //

                        int32_t v = 0;

                        // read current value
                        {
                            auto q( Query::create(cassandra_session));
                            // key = '*test_snap_lock*'
                            // column1 = 'counter'
                            q->query("SELECT value FROM snap_websites.domains WHERE key = 0x2a746573745f736e61705f6c6f636b2a AND column1 = 0x636f756e746572", 0);
                            q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
                            q->start();

                            // the very first time the value does not exist
                            if(q->nextRow())
                            {
                                QByteArray const value(q->getByteArrayColumn("value"));
                                v = safeInt32Value(value);
                            }
                        }

                        // increment counter by one
                        ++v;

                        std::cout << time(nullptr) << ": -> (" << getpid() << ") = " << v << std::endl << std::flush;

                        // write new value
                        {
                            QByteArray value;
                            setInt32Value(value, v);

                            auto q( Query::create(cassandra_session));
                            // key = '*test_snap_lock*'
                            // column1 = 'counter'
                            q->query("INSERT INTO snap_websites.domains (key, column1, value) VALUES (0x2a746573745f736e61705f6c6f636b2a, 0x636f756e746572, ?)", 1);
                            q->setConsistencyLevel(Query::consistency_level_t::level_quorum);
                            q->bindByteArray(0, value);
                            q->start();
                        }
                    }
                }
            }
            catch(tcp_client_server::tcp_client_server_parameter_error const & e)
            {
                std::cerr << "tcp_client_server::tcp_client_server_parameter_error exception occurred: " << e.what() << std::endl;
                exit(1);
            }
            catch(std::exception const & e)
            {
                std::cerr << "!!! exception [" << getpid() << "]: " << e.what() << std::endl;
                exit(1);
            }

            std::cout << std::endl;
            exit(0);
        }
        children.push_back(child);
    }

    // now wait on those children
    int err(0);
    while(!children.empty())
    {
        int status;
        waitpid(children[0], &status, 0);
        if(!WIFEXITED(status)
        || WEXITSTATUS(status) != 0)
        {
            ++err;
        }
        children.erase(children.begin());
    }

    // errors occurred?
    if(err > 0)
    {
        std::cerr << "\n" << err << " children exited with an error.\n";
        exit(1);
    }

    // all good!
    exit(0);
}

// vim: ts=4 sw=4 et
