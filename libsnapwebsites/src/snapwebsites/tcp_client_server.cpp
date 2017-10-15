// TCP Client & Server -- classes to ease handling sockets
// Copyright (C) 2012-2017  Made to Order Software Corp.
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

#include "snapwebsites/tcp_client_server.h"

#include "snapwebsites/log.h"
#include "snapwebsites/not_reached.h"
#include "snapwebsites/not_used.h"

#include <sstream>

#include <netdb.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "snapwebsites/poison.h"


#ifndef OPENSSL_THREADS
#error "OPENSSL_THREADS is not defined. Snap! requires support for multiple threads in OpenSSL."
#endif

namespace tcp_client_server
{


namespace
{

/** \brief Address info class to auto-free the structures.
 *
 * This class is used so we can auto-free the addrinfo structure(s)
 * because otherwise we find ourselves with many freeaddrinfo()
 * calls (and that's not safe in case you have exceptions.)
 */
class addrinfo_t
{
public:
    /** \brief Initialize the structure pointer to nullptr.
     *
     * The constructor ensures that the pointer is nullptr.
     */
    addrinfo_t()
        : f_addrinfo(nullptr)
    {
    }

    /** \brief Ensures that the structures get cleaned up.
     *
     * This function ensures that the addrinfo pointers get freed with a
     * call to the freeaddrinfo().
     *
     * If no structure was allocated, nothing happens.
     */
    ~addrinfo_t()
    {
        if(f_addrinfo != nullptr)
        {
            freeaddrinfo(f_addrinfo);
        }
    }

    /** \brief The address info.
     *
     * This is the address, it is public because we just use that internally
     * in the client and server constructors (see below.)
     */
    struct addrinfo *   f_addrinfo;
};


/** \brief Data handled by each lock.
 *
 * This function holds the data handled on a per lock basis.
 * Even if your daemon is not using multiple threads, this
 * is likely to kick in.
 */
class crypto_lock_t
{
public:
    typedef std::vector<crypto_lock_t>  vector_t;

                        crypto_lock_t()
                        {
                            pthread_mutex_init(&f_mutex, nullptr);
                        }

                        ~crypto_lock_t()
                        {
                            pthread_mutex_destroy(&f_mutex);
                        }

    void                lock()
                        {
                            pthread_mutex_lock(&f_mutex);
                        }

    void                unlock()
                        {
                            pthread_mutex_unlock(&f_mutex);
                        }

private:
    pthread_mutex_t     f_mutex = pthread_mutex_t();
};


/** \brief The vector of locks.
 *
 * This function is initialized by the crypto_thread_setup().
 *
 * It is defined as a pointer in case someone was to try to access this
 * pointer before entering main().
 */
crypto_lock_t::vector_t *   g_locks = nullptr;


/** \brief Retrieve the system thread identifier.
 *
 * This function is used by the OpenSSL library to attach an internal thread
 * identifier (\p tid) to a system thread identifier.
 *
 * \param[in] tid  The crypto internal thread identifier.
 */
void pthreads_thread_id(CRYPTO_THREADID * tid)
{
    CRYPTO_THREADID_set_numeric(tid, static_cast<unsigned long>(pthread_self()));
}


/** \brief Handle locks and unlocks.
 *
 * This function is a callback used to lock and unlock mutexes as required.
 *
 * \param[in] mode  Whether lock or unlock in read or write mode.
 * \param[in] type  The "type" of lock (i.e. the index).
 * \param[in] file  The filename of the source asking for a lock/unlock.
 * \param[in] line  The line number in file where the call was made.
 */
void pthreads_locking_callback(int mode, int type, char const * file, int line)
{
    snap::NOTUSED(file);
    snap::NOTUSED(line);

    if(g_locks == nullptr)
    {
        throw tcp_client_server_initialization_missing_error("g_locks was not initialized");
    }

/*
# ifdef undef
    BIO_printf(bio_err, "thread=%4d mode=%s lock=%s %s:%d\n",
               CRYPTO_thread_id(),
               (mode & CRYPTO_LOCK) ? "l" : "u",
               (type & CRYPTO_READ) ? "r" : "w", file, line);
# endif
    if (CRYPTO_LOCK_SSL_CERT == type)
            BIO_printf(bio_err,"(t,m,f,l) %ld %d %s %d\n",
                       CRYPTO_thread_id(),
                       mode,file,line);
*/

    // Note: at this point we ignore READ | WRITE because we do not have
    //       such a concept with a simple mutex; we could take those in
    //       account with a semaphore though.
    //
    if((mode & CRYPTO_LOCK) != 0)
    {
        (*g_locks)[type].lock();
    }
    else
    {
        (*g_locks)[type].unlock();
    }
}


/** \brief This function is called once on initialization.
 *
 * This function is called when the bio_initialize() function. It is
 * expected that the bio_initialize() function is called once by the
 * main thread before any other thread has a chance to do so.
 */
void crypto_thread_setup()
{
    if(g_locks != nullptr)
    {
        throw tcp_client_server_initialization_error("crypto_thread_setup() called for the second time. This usually means two threads are initializing the BIO environment simultaneously.");
    }

    g_locks = new crypto_lock_t::vector_t(CRYPTO_num_locks());

    CRYPTO_THREADID_set_callback(pthreads_thread_id);
    CRYPTO_set_locking_callback(pthreads_locking_callback);
}


/** \brief This function cleans up the thread setup.
 *
 * This function could be called to clean up the setup created to support
 * multiple threads running with the OpenSSL library.
 *
 * \note
 * At this time this function never gets called.
 */
void thread_cleanup()
{
    CRYPTO_set_locking_callback(nullptr);

    delete g_locks;
    g_locks = nullptr;
}







/** \brief Whether the bio_initialize() function was already called.
 *
 * This flag is used to know whether the bio_initialize() function was
 * already called. Only the bio_initialize() function is expected to
 * make use of this flag. Other functions should simply call the
 * bio_initialize() function (future versions may include addition
 * flags or use various bits in an integer instead.)
 */
bool g_bio_initialized = false;


/** \brief Initialize the BIO library.
 *
 * This function is called by the BIO implementations to initialize the
 * BIO library as required. It can be called any number of times. The
 * initialization will happen only once.
 */
void bio_initialize()
{
    // already initialized?
    if(g_bio_initialized)
    {
        return;
    }
    g_bio_initialized = true;

    // Make sure the SSL library gets initialized
    SSL_library_init();

    // TBD: should we call the load string functions only when we
    //      are about to generate the first error?
    //
    ERR_load_crypto_strings();
    ERR_load_SSL_strings();
    SSL_load_error_strings();

    // TODO: define a way to only define safe algorithms?
    //       (it looks like we can force TLSv1.2 below at least)
    //
    OpenSSL_add_all_algorithms();

    // TBD: need a PRNG seeding before creating a new SSL context?

    // then initialize the library so it works in a multithreaded
    // environment
    //
    crypto_thread_setup();
}


/** \brief Get all the error messages and output them in our logs.
 *
 * This function reads all existing errors from the OpenSSL library
 * and send them to our logs.
 */
void bio_log_errors()
{
    // allow for up to 5 errors in one go, but we have a HUGE problem
    // at this time as in some cases the same error is repeated forever
    //
    for(;;)
    {
        char const * filename(nullptr);
        int line(0);
        char const * data(nullptr);
        int flags(0);
        unsigned long bio_errno(ERR_get_error_line_data(&filename, &line, &data, &flags));
        if(bio_errno == 0)
        {
            // no more errors
            //
            return;
        }

        // get corresponding messages too
        //
        // Note: current OpenSSL documentation on Ubuntu says errmsg[]
        //       should be at least 120 characters BUT the code actually
        //       use a limit of 256...
        //
        char errmsg[256];
        ERR_error_string_n(bio_errno, errmsg, sizeof(errmsg) / sizeof(errmsg[0]));
        // WARNING: the ERR_error_string() function is NOT multi-thread safe

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        int const lib_num(ERR_GET_LIB(bio_errno));
        int const func_num(ERR_GET_FUNC(bio_errno));
#pragma GCC diagnostic pop
        char const * lib_name(ERR_lib_error_string(lib_num));
        char const * func_name(ERR_func_error_string(func_num));
        int const reason_num(ERR_GET_REASON(bio_errno));
        char const * reason(ERR_reason_error_string(reason_num));

        // the format used by the OpenSSL library is as follow:
        //
        //     [pid]:error:[error code]:[library name]:[function name]:[reason string]:[file name]:[line]:[optional text message]
        //
        // we do not duplicate the [pid] and "error" but include all the
        // other fields
        //
        SNAP_LOG_ERROR("OpenSSL: [")
                      (bio_errno) // should be shown in hex...
                      ("/")
                      (lib_num)
                      ("|")
                      (func_num)
                      ("|")
                      (reason_num)
                      ("]:[")
                      (lib_name)
                      ("]:[")
                      (func_name)
                      ("]:[")
                      (reason)
                      ("]:[")
                      (filename)
                      ("]:[")
                      (line)
                      ("]:[")
                      ((flags & ERR_TXT_STRING) != 0 && data != nullptr ? data : "(no details)")
                      ("]");
    }
}


/** \brief Free a BIO object.
 *
 * This deleter is used to make sure that the BIO object gets freed
 * whenever the object holding it gets destroyed.
 *
 * Note that deleting a BIO connection calls shutdown() and close()
 * on the socket. In other words, it hangs up.
 *
 * \param[in] bio  The BIO object to be freed.
 */
void bio_deleter(BIO * bio)
{
    // IMPORTANT NOTE:
    //
    //   The BIO_free_all() calls shutdown() on the socket. This is not
    //   acceptable in a normal Unix application that makes use of fork().
    //   So... instead we ask the BIO interface to not close the socket,
    //   and instead we close it ourselves. This means the shutdown()
    //   never gets called.
    //
    BIO_set_close(bio, BIO_NOCLOSE);

    int c;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
    BIO_get_fd(bio, &c);
#pragma GCC diagnostic pop
    if(c != -1)
    {
        close(c);
    }

    BIO_free_all(bio);
}


/** \brief Free an SSL_CTX object.
 *
 * This deleter is used to make sure that the SSL_CTX object gets
 * freed whenever the object holding it gets destroyed.
 */
void ssl_ctx_deleter(SSL_CTX * ssl_ctx)
{
    SSL_CTX_free(ssl_ctx);
}


}
// no name namespace



// ========================= CLIENT =========================


/** \class tcp_client
 * \brief Create a client socket and connect to a server.
 *
 * This class is a client socket implementation used to connect to a server.
 * The server is expected to be running at the time the client is created
 * otherwise it fails connecting.
 *
 * This class is not appropriate to connect to a server that may come and go
 * over time.
 */

/** \brief Contruct a tcp_client object.
 *
 * The tcp_client constructor initializes a TCP client object by connecting
 * to the specified server. The server is defined with the \p addr and
 * \p port specified as parameters.
 *
 * \exception tcp_client_server_parameter_error
 * This exception is raised if the \p port parameter is out of range or the
 * IP address is an empty string or otherwise an invalid address.
 *
 * \exception tcp_client_server_runtime_error
 * This exception is raised if the client cannot create the socket or it
 * cannot connect to the server.
 *
 * \param[in] addr  The address of the server to connect to. It must be valid.
 * \param[in] port  The port the server is listening on.
 */
tcp_client::tcp_client(std::string const & addr, int port)
    : f_socket(-1)
    , f_port(port)
    , f_addr(addr)
{
    if(f_port < 0 || f_port >= 65536)
    {
        throw tcp_client_server_parameter_error("invalid port for a client socket");
    }
    if(f_addr.empty())
    {
        throw tcp_client_server_parameter_error("an empty address is not valid for a client socket");
    }

    std::stringstream decimal_port;
    decimal_port << f_port;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo_t addr_info;
    std::string port_str(decimal_port.str());
    int r(getaddrinfo(addr.c_str(), port_str.c_str(), &hints, &addr_info.f_addrinfo));
    if(r != 0 || addr_info.f_addrinfo == nullptr)
    {
        int const e(errno);
        SNAP_LOG_FATAL("getaddrinfo() failed to parse the address and port strings (errno: ")(e)(" -- ")(strerror(e))(")");
        throw tcp_client_server_runtime_error("invalid address or port: \"" + addr + ":" + port_str + "\"");
    }

    f_socket = socket(addr_info.f_addrinfo->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if(f_socket < 0)
    {
        int const e(errno);
        SNAP_LOG_FATAL("socket() failed to create a socket descriptor (errno: ")(e)(" -- ")(strerror(e))(")");
        throw tcp_client_server_runtime_error("could not create socket for client");
    }

    if(connect(f_socket, addr_info.f_addrinfo->ai_addr, addr_info.f_addrinfo->ai_addrlen) < 0)
    {
        int const e(errno);
        SNAP_LOG_FATAL("connect() failed to connect a socket (errno: ")(e)(" -- ")(strerror(e))(")");
        close(f_socket);
        throw tcp_client_server_runtime_error("could not connect client socket to \"" + f_addr + "\"");
    }
}

/** \brief Clean up the TCP client object.
 *
 * This function cleans up the TCP client object by closing the attached socket.
 *
 * \note
 * DO NOT use the shutdown() call since we may end up forking and using
 * that connection in the child.
 */
tcp_client::~tcp_client()
{
    close(f_socket);
}

/** \brief Get the socket descriptor.
 *
 * This function returns the TCP client socket descriptor. This can be
 * used to change the descriptor behavior (i.e. make it non-blocking for
 * example.)
 *
 * \return The socket descriptor.
 */
int tcp_client::get_socket() const
{
    return f_socket;
}

/** \brief Get the TCP client port.
 *
 * This function returns the port used when creating the TCP client.
 * Note that this is the port the server is listening to and not the port
 * the TCP client is currently connected to.
 *
 * \return The TCP client port.
 */
int tcp_client::get_port() const
{
    return f_port;
}

/** \brief Get the TCP server address.
 *
 * This function returns the address used when creating the TCP address as is.
 * Note that this is the address of the server where the client is connected
 * and not the address where the client is running (although it may be the
 * same.)
 *
 * Use the get_client_addr() function to retrieve the client's TCP address.
 *
 * \return The TCP client address.
 */
std::string tcp_client::get_addr() const
{
    return f_addr;
}

/** \brief Get the TCP client port.
 *
 * This function retrieve the port of the client (used on your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \return The port or -1 if it cannot be determined.
 */
int tcp_client::get_client_port() const
{
    struct sockaddr addr;
    socklen_t len(sizeof(addr));
    int r(getsockname(f_socket, &addr, &len));
    if(r != 0)
    {
        return -1;
    }
    // Note: I know the port is at the exact same location in both
    //       structures in Linux but it could change on other Unices
    if(addr.sa_family == AF_INET)
    {
        // IPv4
        return reinterpret_cast<sockaddr_in *>(&addr)->sin_port;
    }
    if(addr.sa_family == AF_INET6)
    {
        // IPv6
        return reinterpret_cast<sockaddr_in6 *>(&addr)->sin6_port;
    }
    return -1;
}

/** \brief Get the TCP client address.
 *
 * This function retrieve the IP address of the client (your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \return The IP address as a string.
 */
std::string tcp_client::get_client_addr() const
{
    struct sockaddr addr;
    socklen_t len(sizeof(addr));
    int const r(getsockname(f_socket, &addr, &len));
    if(r != 0)
    {
        throw tcp_client_server_runtime_error("address not available");
    }
    char buf[BUFSIZ];
    switch(addr.sa_family)
    {
    case AF_INET:
        inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in *>(&addr)->sin_addr, buf, sizeof(buf));
        break;

    case AF_INET6:
        inet_ntop(AF_INET6, &reinterpret_cast<struct sockaddr_in6 *>(&addr)->sin6_addr, buf, sizeof(buf));
        break;

    default:
        throw tcp_client_server_runtime_error("unknown address family");

    }
    return buf;
}

/** \brief Read data from the socket.
 *
 * A TCP socket is a stream type of socket and one can read data from it
 * as if it were a regular file. This function reads \p size bytes and
 * returns. The function returns early if the server closes the connection.
 *
 * If your socket is blocking, \p size should be exactly what you are
 * expecting or this function will block forever or until the server
 * closes the connection.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \param[in,out] buf  The buffer where the data is read.
 * \param[in] size  The size of the buffer.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 */
int tcp_client::read(char *buf, size_t size)
{
    return static_cast<int>(::read(f_socket, buf, size));
}


/** \brief Read one line.
 *
 * This function reads one line from the current location up to the next
 * '\\n' character. We do not have any special handling of the '\\r'
 * character.
 *
 * The function may return 0 in which case the server closed the connection.
 *
 * \param[out] line  The resulting line read from the server.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 *         If the function returns 0 or more, then the \p line parameter
 *         represents the characters read on the network.
 */
int tcp_client::read_line(std::string& line)
{
    line.clear();
    int len(0);
    for(;;)
    {
        char c;
        int r(read(&c, sizeof(c)));
        if(r <= 0)
        {
            return len == 0 && r < 0 ? -1 : len;
        }
        if(c == '\n')
        {
            return len;
        }
        ++len;
        line += c;
    }
}


/** \brief Write data to the socket.
 *
 * A TCP socket is a stream type of socket and one can write data to it
 * as if it were a regular file. This function writes \p size bytes to
 * the socket and then returns. This function returns early if the server
 * closes the connection.
 *
 * If your socket is not blocking, less than \p size bytes may be written
 * to the socket. In that case you are responsible for calling the function
 * again to write the remainder of the buffer until the function returns
 * a number of bytes written equal to \p size.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \param[in] buf  The buffer with the data to send over the socket.
 * \param[in] size  The number of bytes in buffer to send over the socket.
 *
 * \return The number of bytes that were actually accepted by the socket
 * or -1 if an error occurs.
 */
int tcp_client::write(const char *buf, size_t size)
{
    return static_cast<int>(::write(f_socket, buf, size));
}


// ========================= SERVER =========================

/** \brief Initialize the server and start listening for connections.
 *
 * The server constructor creates a socket, binds it, and then listen to it.
 *
 * By default the server accepts a maximum of \p max_connections (set to
 * 0 or less to get the default tcp_server::MAX_CONNECTIONS) in its waiting queue.
 * If you use the server and expect a low connection rate, you may want to
 * reduce the count to 5. Although some very busy servers use larger numbers.
 * This value gets clamped to a minimum of 5 and a maximum of 1,000.
 *
 * Note that the maximum number of connections is actually limited to
 * /proc/sys/net/core/somaxconn connections. This number is generally 128 
 * in 2016. So the  super high limit of 1,000 is anyway going to be ignored
 * by the OS.
 *
 * The address is made non-reusable (which is the default for TCP sockets.)
 * It is possible to mark the server address as immediately reusable by
 * setting the \p reuse_addr to true.
 *
 * By default the server is marked as "keepalive". You can turn it off
 * using the keepalive() function with false.
 *
 * \exception tcp_client_server_parameter_error
 * This exception is raised if the address parameter is an empty string or
 * otherwise an invalid IP address, or if the port is out of range.
 *
 * \exception tcp_client_server_runtime_error
 * This exception is raised if the socket cannot be created, bound to
 * the specified IP address and port, or listen() fails on the socket.
 *
 * \param[in] addr  The address to listen on. It may be set to "0.0.0.0".
 * \param[in] port  The port to listen on.
 * \param[in] max_connections  The number of connections to keep in the listen queue.
 * \param[in] reuse_addr  Whether to mark the socket with the SO_REUSEADDR flag.
 * \param[in] auto_close  Automatically close the client socket in accept and the destructor.
 */
tcp_server::tcp_server(std::string const & addr, int port, int max_connections, bool reuse_addr, bool auto_close)
    : f_max_connections(max_connections <= 0 ? MAX_CONNECTIONS : max_connections)
    //, f_socket(-1) -- auto-init
    , f_port(port)
    , f_addr(addr)
    //, f_accepted_socket(-1) -- auto-init
    //, f_keepalive(true) -- auto-init
    , f_auto_close(auto_close)
{
    if(f_addr.empty())
    {
        throw tcp_client_server_parameter_error("the address cannot be an empty string.");
    }
    if(f_port < 0 || f_port >= 65536)
    {
        throw tcp_client_server_parameter_error("invalid port for a client socket.");
    }
    if(f_max_connections < 5)
    {
        f_max_connections = 5;
    }
    else if(f_max_connections > 1000)
    {
        f_max_connections = 1000;
    }

    //char decimal_port[16];
    std::stringstream decimal_port;
    decimal_port << f_port;
    //snprintf(decimal_port, sizeof(decimal_port), "%d", f_port);
    //decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo_t addr_info;
    std::string port_str(decimal_port.str());
    int r(getaddrinfo(addr.c_str(), port_str.c_str(), &hints, &addr_info.f_addrinfo));
    if(r != 0 || addr_info.f_addrinfo == nullptr)
    {
        throw tcp_client_server_runtime_error("invalid address or port: \"" + addr + ":" + port_str + "\"");
    }

    f_socket = socket(addr_info.f_addrinfo->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if(f_socket < 0)
    {
        int const e(errno);
        SNAP_LOG_FATAL("socket() failed to create a socket descriptor (errno: ")(e)(" -- ")(strerror(e))(")");
        throw tcp_client_server_runtime_error("could not create socket for client");
    }

    // this should be optional as reusing an address for TCP/IP is not 100% safe
    if(reuse_addr)
    {
        // try to mark the socket address as immediately reusable
        // if this fails, we ignore the error (TODO log an INFO message)
        int optval(1);
        socklen_t const optlen(sizeof(optval));
        snap::NOTUSED(setsockopt(f_socket, SOL_SOCKET, SO_REUSEADDR, &optval, optlen));
    }

    if(bind(f_socket, addr_info.f_addrinfo->ai_addr, addr_info.f_addrinfo->ai_addrlen) < 0)
    {
        close(f_socket);
        throw tcp_client_server_runtime_error("could not bind the socket to \"" + f_addr + "\"");
    }

    // start listening, we expect the caller to then call accept() to
    // acquire connections
    if(listen(f_socket, f_max_connections) < 0)
    {
        close(f_socket);
        throw tcp_client_server_runtime_error("could not listen to the socket bound to \"" + f_addr + "\"");
    }
}


/** \brief Clean up the server sockets.
 *
 * This function ensures that the server sockets get cleaned up.
 *
 * If the \p auto_close parameter was set to true in the constructor, then
 * the last accepter socket gets closed by this function.
 *
 * \note
 * DO NOT use the shutdown() call since we may end up forking and using
 * that connection in the child.
 */
tcp_server::~tcp_server()
{
    close(f_socket);
    if(f_auto_close && f_accepted_socket != -1)
    {
        close(f_accepted_socket);
    }
}


/** \brief Retrieve the socket descriptor.
 *
 * This function returns the socket descriptor. It can be used to
 * tweak things on the socket such as making it non-blocking or
 * directly accessing the data.
 *
 * \return The socket descriptor.
 */
int tcp_server::get_socket() const
{
    return f_socket;
}


/** \brief Retrieve the maximum number of connections.
 *
 * This function returns the maximum number of connections that can
 * be accepted by the socket. This was set by the constructor and
 * it cannot be changed later.
 *
 * \return The maximum number of incoming connections.
 */
int tcp_server::get_max_connections() const
{
    return f_max_connections;
}


/** \brief Return the server port.
 *
 * This function returns the port the server was created with. This port
 * is exactly what the server currently uses. It cannot be changed.
 *
 * \return The server port.
 */
int tcp_server::get_port() const
{
    return f_port;
}


/** \brief Retrieve the server IP address.
 *
 * This function returns the IP address used to bind the socket. This
 * is the address clients have to use to connect to the server unless
 * the address was set to all zeroes (0.0.0.0) in which case any user
 * can connect.
 *
 * The IP address cannot be changed.
 *
 * \return The server IP address.
 */
std::string tcp_server::get_addr() const
{
    return f_addr;
}


/** \brief Return the current status of the keepalive flag.
 *
 * This function returns the current status of the keepalive flag. This
 * flag is set to true by default (in the constructor.) It can be
 * changed with the keepalive() function.
 *
 * The flag is used to mark new connections with the SO_KEEPALIVE flag.
 * This is used whenever a service may take a little to long to answer
 * and avoid losing the TCP connection before the answer is sent to
 * the client.
 *
 * \return The current status of the keepalive flag.
 */
bool tcp_server::get_keepalive() const
{
    return f_keepalive;
}


/** \brief Set the keepalive flag.
 *
 * This function sets the keepalive flag to either true (i.e. mark connection
 * sockets with the SO_KEEPALIVE flag) or false. The default is true (as set
 * in the constructor,) because in most cases this is a feature people want.
 *
 * \param[in] yes  Whether to keep new connections alive even when no traffic
 * goes through.
 */
void tcp_server::set_keepalive(bool yes)
{
    f_keepalive = yes;
}


/** \brief Accept a connection.
 *
 * A TCP server accepts incoming connections. This call is a blocking call.
 * If no connections are available on the line, then the call blocks until
 * a connection becomes available.
 *
 * To prevent being blocked by this call you can either check the status of
 * the file descriptor (use the get_socket() function to retrieve the
 * descriptor and use an appropriate wait with 0 as a timeout,) or transform
 * the socket in a non-blocking socket (not tested, though.)
 *
 * This TCP socket implementation is expected to be used in one of two ways:
 *
 * (1) the main server accepts connections and then fork()'s to handle the
 * transaction with the client, in that case we want to set the \p auto_close
 * parameter of the constructor to true so the accept() function automatically
 * closes the last accepted socket.
 *
 * (2) the main server keeps a set of connections and handles them alongside
 * the main server connection. Although there are limits to what you can do
 * in this way, it is very efficient, but this also means the accept() call
 * cannot close the last accepted socket since the rest of the software may
 * still be working on it.
 *
 * The function returns a client/server socket. This is the socket one can
 * use to communicate with the client that just connected to the server. This
 * descriptor can be written to or read from.
 *
 * This function is the one that applies the keepalive flag to the
 * newly accepted socket.
 *
 * \note
 * If you prevent SIGCHLD from stopping your code, you may want to allow it
 * when calling this function (that is, if you're interested in getting that
 * information immediately, otherwise it is cleaner to always block those
 * signals.)
 *
 * \note
 * DO NOT use the shutdown() call since we may end up forking and using
 * that connection in the child.
 *
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for
 *            a message. If set to -1 (the default), accept() will block
 *            indefintely.
 *
 * \return A client socket descriptor or -1 if an error occured, -2 if timeout and max_wait is set.
 */
int tcp_server::accept( int const max_wait_ms )
{
    // auto-close?
    if(f_auto_close && f_accepted_socket != -1)
    {
        // if the close is interrupted, make sure we try again otherwise
        // we could lose that stream until next restart (this could happen
        // if you have SIGCHLD)
        if(close(f_accepted_socket) == -1)
        {
            if(errno == EINTR)
            {
                close(f_accepted_socket);
            }
        }
    }
    f_accepted_socket = -1;

    if( max_wait_ms > -1 )
    {
        fd_set s;
        //
        FD_ZERO(&s);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        FD_SET(f_socket, &s);
#pragma GCC diagnostic pop
        //
        struct timeval timeout;
        timeout.tv_sec = max_wait_ms / 1000;
        timeout.tv_usec = (max_wait_ms % 1000) * 1000;
        int const retval = select(f_socket + 1, &s, NULL, &s, &timeout);
        //
        if( retval == -1 )
        {
            // error
            //
            return -1;
        }
        else if( retval == 0 )
        {
            // timeout
            //
            return -2;
        }
    }

    // accept the next connection
    struct sockaddr_in accepted_addr;
    socklen_t addr_len(sizeof(accepted_addr));
    memset(&accepted_addr, 0, sizeof(accepted_addr));
    f_accepted_socket = ::accept(f_socket, reinterpret_cast<struct sockaddr *>(&accepted_addr), &addr_len);

    // mark the new connection with the SO_KEEPALIVE flag
    if(f_accepted_socket != -1 && f_keepalive)
    {
        // if this fails, we ignore the error, but still log the event
        int optval(1);
        socklen_t const optlen(sizeof(optval));
        if(setsockopt(f_accepted_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) != 0)
        {
            SNAP_LOG_WARNING("tcp_server::accept(): an error occurred trying to mark accepted socket with SO_KEEPALIVE.");
        }
    }

    return f_accepted_socket;
}


/** \brief Retrieve the last accepted socket descriptor.
 *
 * This function returns the last accepted socket descriptor as retrieved by
 * accept(). If accept() was never called or failed, then this returns -1.
 *
 * Note that it is possible that the socket was closed in between in which
 * case this value is going to be an invalid socket.
 *
 * \return The last accepted socket descriptor.
 */
int tcp_server::get_last_accepted_socket() const
{
    return f_accepted_socket;
}




// ========================= BIO CLIENT =========================


/** \class bio_client
 * \brief Create a BIO client and connect to a server, eventually with TLS.
 *
 * This class is a client socket implementation used to connect to a server.
 * The server is expected to be running at the time the client is created
 * otherwise it fails connecting.
 *
 * This class is not appropriate to connect to a server that may come and go
 * over time.
 *
 * The BIO extension is from the OpenSSL library and it allows the client
 * to connect using SSL. At this time connections are either secure or
 * not secure. If a secure connection fails, you may attempt again without
 * TLS or other encryption mechanism.
 */


/** \brief Contruct a bio_client object.
 *
 * The bio_client constructor initializes a BIO connector and connects
 * to the specified server. The server is defined with the \p addr and
 * \p port specified as parameters. The connection tries to use TLS if
 * the \p mode parameter is set to MODE_SECURE. Note that you may force
 * a secure connection using MODE_SECURE_REQUIRED. With MODE_SECURE,
 * the connection to the server can be obtained even if a secure
 * connection could not be made to work.
 *
 * \todo
 * Create another client with BIO_new_socket() so one can create an SSL
 * connection with a socket retrieved from an accept() call.
 *
 * \exception tcp_client_server_parameter_error
 * This exception is raised if the \p port parameter is out of range or the
 * IP address is an empty string or otherwise an invalid address.
 *
 * \exception tcp_client_server_initialization_error
 * This exception is raised if the client cannot create the socket or it
 * cannot connect to the server.
 *
 * \param[in] addr  The address of the server to connect to. It must be valid.
 * \param[in] port  The port the server is listening on.
 * \param[in] mode  Whether to use SSL when connecting.
 */
bio_client::bio_client(std::string const & addr, int port, mode_t mode)
    //: f_ssl_ctx(nullptr) -- auto-init
    //, f_bio(nullptr) -- auto-init
{
    if(port < 0 || port >= 65536)
    {
        throw tcp_client_server_parameter_error("invalid port for a client socket");
    }
    if(addr.empty())
    {
        throw tcp_client_server_parameter_error("an empty address is not valid for a client socket");
    }

    bio_initialize();

    switch(mode)
    {
    case mode_t::MODE_SECURE:
    case mode_t::MODE_ALWAYS_SECURE:
        {
            // Use TLS v1 only as all versions of SSL are flawed...
            // (see below the SSL_CTX_set_options() for additional details
            // about that since here it does indeed say SSLv23...)
            //
            std::shared_ptr<SSL_CTX> ssl_ctx(SSL_CTX_new(SSLv23_client_method()), ssl_ctx_deleter);
            if(!ssl_ctx)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed creating an SSL_CTX object");
            }

            // allow up to 4 certificates in the chain otherwise fail
            // (this is not a very strong security feature though)
            // TODO: allow user to specify the depth
            SSL_CTX_set_verify_depth(ssl_ctx.get(), 4);

            // make sure SSL v2/3 is not used, also compression in SSL is
            // known to have security issues
            // TODO: allow client to specify some of these options
            SSL_CTX_set_options(ssl_ctx.get(), SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_COMPRESSION);

            // limit the number of ciphers the connection can use
            if(mode == mode_t::MODE_SECURE)
            {
                // this is used by local connections and we get a very strong
                // algorithm anyway, but at this point I do not know why it
                // does not work with the limited list below...
                //
                // TODO: test with adding DH support in the server then
                //       maybe (probably) that the "HIGH" will work for
                //       this entry too...
                //
                SSL_CTX_set_cipher_list(ssl_ctx.get(), "ALL");
            }
            else
            {
                SSL_CTX_set_cipher_list(ssl_ctx.get(), "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4");
            }

            // load root certificates (correct path for Ubuntu?)
            // TODO: allow client to set the path to certificates
            if(SSL_CTX_load_verify_locations(ssl_ctx.get(), NULL, "/etc/ssl/certs") != 1)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed loading verification certificates in an SSL_CTX object");
            }

            // create a BIO connected to SSL ciphers
            std::shared_ptr<BIO> bio(BIO_new_ssl_connect(ssl_ctx.get()), bio_deleter);
            if(!bio)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed initializing a BIO object");
            }

            // verify that the connection worked
            //
            SSL * ssl(nullptr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_get_ssl(bio.get(), &ssl);
#pragma GCC diagnostic pop
            if(ssl == nullptr)
            {
                // TBD: does this mean we would have a plain connection?
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed connecting BIO object with SSL_CTX object");
            }

            // allow automatic retries in case the connection somehow needs
            // an SSL renegotiation (maybe we should turn that off for cases
            // where we connect to a secure payment gateway?)
            //
            SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

            // TODO: other SSL initialization?

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_set_conn_hostname(bio.get(), const_cast<char *>(addr.c_str()));
            BIO_set_conn_int_port(bio.get(), &port);
#pragma GCC diagnostic pop

            // connect to the server (open the socket)
            if(BIO_do_connect(bio.get()) <= 0)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed connecting BIO object to server");
            }

            // encryption handshake
            if(BIO_do_handshake(bio.get()) != 1)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed establishing a secure BIO connection with server");
            }

            // verify that the peer certificate was signed by a
            // recognized root authority
            //
            if(SSL_get_peer_certificate(ssl) == nullptr)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("peer failed presenting a certificate for security verification");
            }

            // XXX: check that the call below is similar to the example
            //      usage of SSL_CTX_set_verify() which checks the name
            //      of the certificate, etc.
            //
            if(SSL_get_verify_result(ssl) != X509_V_OK)
            {
                if(mode != mode_t::MODE_SECURE)
                {
                    bio_log_errors();
                    throw tcp_client_server_initialization_error("peer certificate could not be verified");
                }
                SNAP_LOG_WARNING("connecting with SSL but certificate verification failed.");
            }

            // it worked, save the results
            //
            f_ssl_ctx.swap(ssl_ctx);
            f_bio.swap(bio);

            // secure connection ready
            char const * cipher_name(SSL_get_cipher(ssl));
            int cipher_bits(0);
            SSL_get_cipher_bits(ssl, &cipher_bits);
            SNAP_LOG_DEBUG("connected with SSL cipher \"")(cipher_name)("\" representing ")(cipher_bits)(" bits of encryption.");
        }
        break;

    case mode_t::MODE_PLAIN:
        {
            // create a plain BIO connection
            std::shared_ptr<BIO> bio(BIO_new(BIO_s_connect()), bio_deleter);
            if(!bio)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed initializing a BIO object");
            }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_set_conn_hostname(bio.get(), const_cast<char *>(addr.c_str()));
            BIO_set_conn_int_port(bio.get(), &port);
#pragma GCC diagnostic pop

            // connect to the server (open the socket)
            if(BIO_do_connect(bio.get()) <= 0)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed connecting BIO object to server");
            }

            // it worked, save the results
            f_bio = bio;

            // plain connection ready
        }
        break;

    }
}


/** \brief Create a BIO client object from an actual BIO pointer.
 *
 * This function is called by the server whenever it accepts a new BIO
 * connection. The server then can return the bio_client object instead
 * of a BIO object.
 *
 * \param[in] bio  The BIO pointer representing a BIO connection with a client.
 */
bio_client::bio_client(std::shared_ptr<BIO> bio)
    //: f_ssl_ctx(nullptr) -- auto-init
    : f_bio(bio)
{
    if(bio)
    {
        // TODO: somehow this does not seem to give us any information
        //       about the cipher and other details...
        //
        //       this is because it is (way) too early, we did not even
        //       receive the HELLO yet!
        //
        SSL * ssl(nullptr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_get_ssl(bio.get(), &ssl);
#pragma GCC diagnostic pop
        if(ssl != nullptr)
        {
            char const * cipher_name(SSL_get_cipher(ssl));
            int cipher_bits(0);
            SSL_get_cipher_bits(ssl, &cipher_bits);
            SNAP_LOG_DEBUG("accepted BIO client with SSL cipher \"")(cipher_name)("\" representing ")(cipher_bits)(" bits of encryption.");
        }
    }
}


/** \brief Clean up the BIO client object.
 *
 * This function cleans up the BIO client object by freeing the SSL_CTX
 * and the BIO objects.
 */
bio_client::~bio_client()
{
    // f_bio and f_ssl_ctx are allocated using shared pointers with
    // a deleter so we have nothing to do here.
}


/** \brief Close the connection.
 *
 * This function closes the connection by losing the f_bio object.
 *
 * As we are at it, we also lose the SSL context since we are not going
 * to use it anymore either.
 */
void bio_client::close()
{
    f_bio.reset();
    f_ssl_ctx.reset();
}


/** \brief Get the socket descriptor.
 *
 * This function returns the TCP client socket descriptor. This can be
 * used to change the descriptor behavior (i.e. make it non-blocking for
 * example.)
 *
 * \note
 * If the socket was closed, then the function returns -1.
 *
 * \warning
 * This socket is generally managed by the BIO library and thus it may
 * create unwanted side effects to change the socket under the feet of
 * the BIO library...
 *
 * \return The socket descriptor.
 */
int bio_client::get_socket() const
{
    if(f_bio)
    {
        int c;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_get_fd(f_bio.get(), &c);
#pragma GCC diagnostic pop
        return c;
    }

    return -1;
}


/** \brief Get the TCP client port.
 *
 * This function returns the port used when creating the TCP client.
 * Note that this is the port the server is listening to and not the port
 * the TCP client is currently connected to.
 *
 * \note
 * If the connection was closed, return -1.
 *
 * \return The TCP client port.
 */
int bio_client::get_port() const
{
    if(f_bio)
    {
        return BIO_get_conn_int_port(f_bio.get());
    }

    return -1;
}


/** \brief Get the TCP server address.
 *
 * This function returns the address used when creating the TCP address as is.
 * Note that this is the address of the server where the client is connected
 * and not the address where the client is running (although it may be the
 * same.)
 *
 * Use the get_client_addr() function to retrieve the client's TCP address.
 *
 * \note
 * If the connection was closed, this function returns "".
 *
 * \return The TCP client address.
 */
std::string bio_client::get_addr() const
{
    if(f_bio)
    {
        return BIO_get_conn_hostname(f_bio.get());
    }

    return "";
}


/** \brief Get the TCP client port.
 *
 * This function retrieve the port of the client (used on your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \return The port or -1 if it cannot be determined.
 */
int bio_client::get_client_port() const
{
    // get_socket() returns -1 if f_bio is nullptr
    //
    int const s(get_socket());
    if(s < 0)
    {
        return -1;
    }

    struct sockaddr addr;
    socklen_t len(sizeof(addr));
    int const r(getsockname(s, &addr, &len));
    if(r != 0)
    {
        return -1;
    }
    // Note: I know the port is at the exact same location in both
    //       structures in Linux but it could change on other Unices
    switch(addr.sa_family)
    {
    case AF_INET:
        // IPv4
        return reinterpret_cast<sockaddr_in *>(&addr)->sin_port;

    case AF_INET6:
        // IPv6
        return reinterpret_cast<sockaddr_in6 *>(&addr)->sin6_port;

    default:
        return -1;

    }
    snap::NOTREACHED();
}


/** \brief Get the TCP client address.
 *
 * This function retrieve the IP address of the client (your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \note
 * The function returns an empty string if the connection was lost
 * or purposefully closed.
 *
 * \return The IP address as a string.
 */
std::string bio_client::get_client_addr() const
{
    // the socket may be invalid, i.e. f_bio may have been deallocated.
    //
    int const s(get_socket());
    if(s < 0)
    {
        return std::string();
    }

    struct sockaddr addr;
    socklen_t len(sizeof(addr));
    int const r(getsockname(s, &addr, &len));
    if(r != 0)
    {
        throw tcp_client_server_runtime_error("failed reading address");
    }
    char buf[BUFSIZ];
    switch(addr.sa_family)
    {
    case AF_INET:
        // TODO: verify that 'r' >= sizeof(something)
        inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in *>(&addr)->sin_addr, buf, sizeof(buf));
        break;

    case AF_INET6:
        // TODO: verify that 'r' >= sizeof(something)
        inet_ntop(AF_INET6, &reinterpret_cast<struct sockaddr_in6 *>(&addr)->sin6_addr, buf, sizeof(buf));
        break;

    default:
        throw tcp_client_server_runtime_error("unknown address family");

    }
    return buf;
}


/** \brief Read data from the socket.
 *
 * A TCP socket is a stream type of socket and one can read data from it
 * as if it were a regular file. This function reads \p size bytes and
 * returns. The function returns early if the server closes the connection.
 *
 * If your socket is blocking, \p size should be exactly what you are
 * expecting or this function will block forever or until the server
 * closes the connection.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \note
 * If the connection was closed, this function returns -1.
 *
 * \warning
 * When the function returns zero, it is likely that the server closed
 * the connection. It may also be that the buffer was empty and that
 * the BIO decided to return early. Since we use a blocking mechanism
 * by default, that should not happen.
 *
 * \todo
 * At this point, I do not know for sure whether errno is properly set
 * or not. It is not unlikely that the BIO library does not keep a clean
 * errno error since they have their own error management.
 *
 * \param[in,out] buf  The buffer where the data is read.
 * \param[in] size  The size of the buffer.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 *
 * \sa read_line()
 * \sa write()
 */
int bio_client::read(char * buf, size_t size)
{
    if(!f_bio)
    {
        errno = EBADF;
        return -1;
    }

    int const r(static_cast<int>(BIO_read(f_bio.get(), buf, size)));
    if(r <= -2)
    {
        // the BIO is not implemented
        bio_log_errors();
        errno = EIO;
        return -1;
    }
    if(r == -1 || r == 0)
    {
        if(BIO_should_retry(f_bio.get()))
        {
            errno = EAGAIN;
            return 0;
        }
        // did we reach the "end of the file"? i.e. did the server
        // close our connection? (this better replicates what a
        // normal socket does when reading from a closed socket)
        //
        if(BIO_eof(f_bio.get()))
        {
            return 0;
        }
        // the BIO generated an error
        bio_log_errors();
        errno = EIO;
        return -1;
    }
    return r;
}


/** \brief Read one line.
 *
 * This function reads one line from the current location up to the next
 * '\\n' character. We do not have any special handling of the '\\r'
 * character.
 *
 * The function may return 0 (an empty string) when the server closes
 * the connection.
 *
 * \note
 * If the connection was closed then this function returns -1.
 *
 * \warning
 * A return value of zero can mean "empty line" and not end of file. It
 * is up to you to know whether your protocol allows for empty lines or
 * not. If so, you may not be able to make use of this function.
 *
 * \param[out] line  The resulting line read from the server. The function
 *                   first clears the contents.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 *         If the function returns 0 or more, then the \p line parameter
 *         represents the characters read on the network without the '\n'.
 *
 * \sa read()
 */
int bio_client::read_line(std::string & line)
{
    line.clear();
    int len(0);
    for(;;)
    {
        char c;
        int r(read(&c, sizeof(c)));
        if(r <= 0)
        {
            return len == 0 && r < 0 ? -1 : len;
        }
        if(c == '\n')
        {
            return len;
        }
        ++len;
        line += c;
    }
}


/** \brief Write data to the socket.
 *
 * A BIO socket is a stream type of socket and one can write data to it
 * as if it were a regular file. This function writes \p size bytes to
 * the socket and then returns. This function returns early if the server
 * closes the connection.
 *
 * If your socket is not blocking, less than \p size bytes may be written
 * to the socket. In that case you are responsible for calling the function
 * again to write the remainder of the buffer until the function returns
 * a number of bytes written equal to \p size.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \note
 * If the connection was closed, return -1.
 *
 * \todo
 * At this point, I do not know for sure whether errno is properly set
 * or not. It is not unlikely that the BIO library does not keep a clean
 * errno error since they have their own error management.
 *
 * \param[in] buf  The buffer with the data to send over the socket.
 * \param[in] size  The number of bytes in buffer to send over the socket.
 *
 * \return The number of bytes that were actually accepted by the socket
 * or -1 if an error occurs.
 *
 * \sa read()
 */
int bio_client::write(char const * buf, size_t size)
{
#ifdef _DEBUG
    // This write is useful when developing APIs against 3rd party
    // servers, otherwise, it's just too much debug
    //SNAP_LOG_TRACE("bio_client::write(): buf=")(buf)(", size=")(size);
#endif
    if(!f_bio)
    {
        errno = EBADF;
        return -1;
    }

    int const r(static_cast<int>(BIO_write(f_bio.get(), buf, size)));
    if(r <= -2)
    {
        // the BIO is not implemented
        bio_log_errors();
        errno = EIO;
        return -1;
    }
    if(r == -1 || r == 0)
    {
        if(BIO_should_retry(f_bio.get()))
        {
            errno = EAGAIN;
            return 0;
        }
        // the BIO generated an error (TBD should we check BIO_eof() too?)
        bio_log_errors();
        errno = EIO;
        return -1;
    }
    BIO_flush(f_bio.get());
    return r;
}









// ========================= BIO SERVER =========================


/** \class bio_server
 * \brief Create a BIO server, bind it, and listen for connections.
 *
 * This class is a server socket implementation used to listen for
 * connections that are to use TLS encryptions.
 *
 * The bind address must be available for the server initialization
 * to succeed.
 *
 * The BIO extension is from the OpenSSL library and it allows the server
 * to allow connections using SSL (TLS really now a day). The server
 * expects to be given information about a certificate and a private
 * key to function. You may also use the server in a non-secure manner
 * (without the TLS layer) so you do not need to implement two instances
 * of your server, one with bio_server and one with tcp_server.
 */




/** \brief Contruct a bio_server object.
 *
 * The bio_server constructor initializes a BIO server and listens
 * for connections from the specified address and port.
 *
 * The \p certificate and \p private_key filenames are expected to point
 * to a PEM file (.pem extension) that include the encryption information.
 *
 * The certificate file may include a chain in which case the whole chain
 * will be taken in account.
 *
 * \warning
 * Currently the max_connections parameter is pretty much ignored since
 * there is no way to pass that paramter down to the BIO interface. In
 * that code they use the SOMAXCONN definition which under Linux is
 * defined at 128 (Ubuntu 16.04.1). See:
 * /usr/include/x86_64-linux-gnu/bits/socket.h
 *
 * \param[in] addr_port  The address and port defined in a snap_addr object.
 * \param[in] max_connections  The number of connections to keep in the listen queue.
 * \param[in] reuse_addr  Whether to mark the socket with the SO_REUSEADDR flag.
 * \param[in] certificate  The server certificate filename (PEM).
 * \param[in] private_key  The server private key filename (PEM).
 * \param[in] mode  The mode used to create the listening socket.
 */
bio_server::bio_server(snap_addr::addr const & addr_port, int max_connections, bool reuse_addr, std::string const & certificate, std::string const & private_key, mode_t mode)
    : f_max_connections(max_connections <= 0 ? MAX_CONNECTIONS : max_connections)
    //, f_bio(nullptr) -- auto-init
    //, f_ssl_ctx(nullptr) -- auto-init
    //, f_keepalive(true) -- auto-init
{
    if(f_max_connections < 5)
    {
        f_max_connections = 5;
    }
    else if(f_max_connections > 1000)
    {
        f_max_connections = 1000;
    }

    bio_initialize();

    switch(mode)
    {
    case mode_t::MODE_SECURE:
        {
            // the following code is based on the example shown in the man page
            //
            //        man BIO_f_ssl
            //
            if(certificate.empty()
            || private_key.empty())
            {
                throw tcp_client_server_parameter_error("with MODE_SECURE you must specify a certificate and a private_key filename");
            }

            std::shared_ptr<SSL_CTX> ssl_ctx(SSL_CTX_new(SSLv23_server_method()), ssl_ctx_deleter);
            if(!ssl_ctx)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed creating an SSL_CTX server object");
            }

            SSL_CTX_set_cipher_list(ssl_ctx.get(), "ALL");//"HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4");

            // Assign the certificate to the SSL context
            //
            // TBD: we may want to use SSL_CTX_use_certificate_file() instead
            //      (i.e. not the "chained" version)
            //
            if(!SSL_CTX_use_certificate_chain_file(ssl_ctx.get(), certificate.c_str()))
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed initializing an SSL_CTX server object certificate");
            }

            // Assign the private key to the SSL context
            //
            if(!SSL_CTX_use_PrivateKey_file(ssl_ctx.get(), private_key.c_str(), SSL_FILETYPE_PEM))
            {
                // on failure, try again again with the RSA version, just in case
                // (probably useless?)
                //
                if(!SSL_CTX_use_RSAPrivateKey_file(ssl_ctx.get(), private_key.c_str(), SSL_FILETYPE_PEM))
                {
                    bio_log_errors();
                    throw tcp_client_server_initialization_error("failed initializing an SSL_CTX server object private key");
                }
            }

            // Verify that the private key and certifcate are a match
            //
            if(!SSL_CTX_check_private_key(ssl_ctx.get()))
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed initializing an SSL_CTX server object private key");
            }

            // create a BIO connection with SSL
            //
            std::unique_ptr<BIO, void (*)(BIO *)> bio(BIO_new_ssl(ssl_ctx.get(), 0), bio_deleter);
            if(!bio)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed initializing a BIO server object");
            }

            // get the SSL pointer, which generally means that the BIO
            // allocate succeeded fully, so we can set auto-retry
            //
            SSL * ssl(nullptr);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_get_ssl(bio.get(), &ssl);
#pragma GCC diagnostic pop
            if(ssl == nullptr)
            {
                // TBD: does this mean we would have a plain connection?
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed connecting BIO object with SSL_CTX object");
            }

            // allow automatic retries in case the connection somehow needs
            // an SSL renegotiation (maybe we should turn that off for cases
            // where we connect to a secure payment gateway?)
            //
            SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

            // create a listening connection
            //
            std::shared_ptr<BIO> listen(BIO_new_accept(addr_port.get_ipv4or6_string(true).c_str()), bio_deleter);
            if(!listen)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed initializing a BIO server object");
            }

            BIO_set_bind_mode(listen.get(), reuse_addr ? BIO_BIND_REUSEADDR : BIO_BIND_NORMAL);

            // Attach the SSL bio to the listening BIO, this means whenever
            // a new connection is accepted, it automatically attaches it to
            // an SSL connection
            //
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            BIO_set_accept_bios(listen.get(), bio.get());
#pragma GCC diagnostic pop

            // WARNING: the listen object takes ownership of the `bio`
            //          pointer and thus we have to make sure that we
            //          do not keep it in our unique_ptr<>().
            //
            snap::NOTUSED(bio.release());

            // Actually call bind() and listen() on the socket
            //
            // IMPORTANT NOTE: The BIO_do_accept() is overloaded, it does
            // two things: (a) it bind() + listen() when called the very
            // first time (i.e. the call right here); (b) it actually
            // accepts a client connection
            //
            int const r(BIO_do_accept(listen.get()));
            if(r <= 0)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed initializing the BIO server socket to listen for client connections");
            }

            // it worked, save the results
            f_ssl_ctx.swap(ssl_ctx);
            f_listen.swap(listen);

            // secure connection ready
        }
        break;

    case mode_t::MODE_PLAIN:
        {
            std::shared_ptr<BIO> listen(BIO_new_accept(addr_port.get_ipv4or6_string(true).c_str()));
            if(!listen)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed initializing a BIO server object");
            }

            BIO_set_bind_mode(listen.get(), BIO_BIND_REUSEADDR);

            // Actually call bind() and listen() on the socket
            //
            // IMPORTANT NOTE: The BIO_do_accept() is overloaded, it does
            // two things: (a) it bind() + listen() when called the very
            // first time (i.e. the call right here); (b) it actually
            // accepts a client connection
            //
            int const r(BIO_do_accept(listen.get()));
            if(r <= 0)
            {
                bio_log_errors();
                throw tcp_client_server_initialization_error("failed initializing the BIO server socket to listen for client connections");
            }

            // it worked, save the results
            //
            f_listen.swap(listen);
        }
        break;

    }
}


/** \brief Tell you whether the server uses a secure BIO or not.
 *
 * This function checks whether the BIO is using encryption (true)
 * or is a plain connection (false).
 *
 * \return true if the BIO was created in secure mode.
 */
bool bio_server::is_secure() const
{
    return static_cast<bool>(f_ssl_ctx);
}


/** \brief Get the listening socket.
 *
 * This function returns the file descriptor of the listening socket.
 * By default the socket is in blocking mode.
 *
 * \return The listening socket file descriptor.
 */
int bio_server::get_socket() const
{
    if(f_listen)
    {
        int c;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_get_fd(f_listen.get(), &c);
#pragma GCC diagnostic pop
        return c;
    }

    return -1;
}


/** \brief Retrieve one new connection.
 *
 * This function will wait until a new connection arrives and returns a
 * new bio_client object for each new connection.
 *
 * If the socket is made non-blocking then the function may return without
 * a bio_client object (i.e. a null pointer instead.)
 *
 * \return A shared pointer to a newly allocated bio_client object.
 */
bio_client::pointer_t bio_server::accept()
{
    // TBD: does one call to BIO_do_accept() accept at most one connection
    //      at a time or could it be that 'r' will be set to 2, 3, 4...
    //      as more connections get accepted?
    //
    int const r(BIO_do_accept(f_listen.get()));
    if(r <= 0)
    {
        // TBD: should we instead return an empty shared pointer in this case?
        //
        bio_log_errors();
        throw tcp_client_server_runtime_error("failed accepting a new BIO");
    }

    // retrieve the new connection by "popping it"
    //
    std::shared_ptr<BIO> bio(BIO_pop(f_listen.get()), bio_deleter);
    if(bio == nullptr)
    {
        bio_log_errors();
        throw tcp_client_server_runtime_error("failed retrieving the accepted BIO");
    }

    // mark the new connection with the SO_KEEPALIVE flag
    if(f_keepalive)
    {
        // retrieve the socket (we do not yet have a bio_client object
        // so we cannot call a get_socket() function...)
        //
        int socket;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_get_fd(bio.get(), &socket);
#pragma GCC diagnostic pop
        if(socket >= 0)
        {
            // if this call fails, we ignore the error, but still log the event
            //
            int optval(1);
            socklen_t const optlen(sizeof(optval));
            if(setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) != 0)
            {
                SNAP_LOG_WARNING("bio_server::accept(): an error occurred trying to mark accepted socket with SO_KEEPALIVE.");
            }
        }
    }

    return bio_client::pointer_t(new bio_client(bio));
}





















// ========================= HELPER FUNCTIONS =========================



/** \brief Check wether a string represents an IPv4 address.
 *
 * This function quickly checks whether the specified string defines a
 * valid IPv4 address. It supports all classes (a.b.c.d, a.b.c., a.b, a)
 * and all numbers can be in decimal, hexadecimal, or octal.
 *
 * \note
 * The function can be called with a null pointer in which case it
 * immediate returns false.
 *
 * \param[in] ip  A pointer to a string holding an address.
 *
 * \return true if the \p ip string represents an IPv4 address.
 *
 * \sa snap_inet::inet_pton_v6()
 */
bool is_ipv4(char const * ip)
{
    if(ip == nullptr)
    {
        return false;
    }

    // we must have (1) a number then (2) a dot or end of string
    // with a maximum of 4 numbers and 3 dots
    //
    int64_t addr[4];
    size_t pos(0);
    for(;; ++ip, ++pos)
    {
        if(*ip < '0' || *ip > '9' || pos >= sizeof(addr) / sizeof(addr[0]))
        {
            // not a valid number
            return false;
        }
        int64_t value(0);

        // number, may be decimal, octal, or hexadecimal
        if(*ip == '0')
        {
            if(ip[1] == 'x' || ip[1] == 'X')
            {
                // expect hexadecimal
                bool first(true);
                for(ip += 2;; ++ip, first = false)
                {
                    if(*ip >= '0' && *ip <= '9')
                    {
                        value = value * 16 + *ip - '0';
                    }
                    else if(*ip >= 'a' && *ip <= 'f')
                    {
                        value = value * 16 + *ip - 'a' + 10;
                    }
                    else if(*ip >= 'A' && *ip <= 'F')
                    {
                        value = value * 16 + *ip - 'A' + 10;
                    }
                    else
                    {
                        if(first)
                        {
                            // not even one digit, not good
                            return false;
                        }
                        // not valid hexadecimal, may be '.' or '\0' (tested below)
                        break;
                    }
                    if(value >= 0x100000000)
                    {
                        // too large even if we have no dots
                        return false;
                    }
                }
            }
            else
            {
                // expect octal
                for(++ip; *ip >= '0' && *ip <= '8'; ++ip)
                {
                    value = value * 8 + *ip - '0';
                    if(value >= 0x100000000)
                    {
                        // too large even if we have no dots
                        return false;
                    }
                }
            }
        }
        else
        {
            // expect decimal
            for(; *ip >= '0' && *ip <= '9'; ++ip)
            {
                value = value * 10 + *ip - '0';
                if(value >= 0x100000000)
                {
                    // too large even if we have no dots
                    return false;
                }
            }
        }
//std::cerr << "value[" << pos << "] = " << value << "\n";
        addr[pos] = value;
        if(*ip != '.')
        {
            if(*ip != '\0')
            {
                return false;
            }
            ++pos;
            break;
        }
    }

//std::cerr << "pos = " << pos << "\n";
    switch(pos)
    {
    case 1:
        // one large value is considered valid for IPv4
        // max. was already checked
        return true;

    case 2:
        return addr[0] < 256 && addr[1] < 0x1000000;

    case 3:
        return addr[0] < 256 && addr[1] < 256 && addr[2] < 0x10000;

    case 4:
        return addr[0] < 256 && addr[1] < 256 && addr[2] < 256 && addr[3] < 256;

    //case 0: (can happen on empty string)
    default:
        // no values, that is incorrect!?
        return false;

    }

    snap::NOTREACHED();
}


/** \brief Check wether a string represents an IPv6 address.
 *
 * This function quickly checks whether the specified string defines a
 * valid IPv6 address. It supports the IPv4 notation at times used
 * inside an IPv6 notation.
 *
 * \note
 * The function can be called with a null pointer in which case it
 * immediate returns false.
 *
 * \param[in] ip  A pointer to a string holding an address.
 *
 * \return true if the \p ip string represents an IPv6 address.
 */
bool is_ipv6(char const * ip)
{
    if(ip == nullptr)
    {
        return false;
    }

    // an IPv6 is a set of 16 bit numbers separated by colon
    // the last two numbers can represented in dot notation (ipv4 class a)
    //
    bool found_colon_colon(false);
    int count(0);
    if(*ip == ':'
    && ip[1] == ':')
    {
        found_colon_colon = true;
        ip += 2;
    }
    for(; *ip != '\0'; ++ip)
    {
        if(count >= 8)
        {
            return false;
        }

        // all numbers are in hexadecimal
        int value(0);
        bool first(true);
        for(;; ++ip, first = false)
        {
            if(*ip >= '0' && *ip <= '9')
            {
                value = value * 16 + *ip - '0';
            }
            else if(*ip >= 'a' && *ip <= 'f')
            {
                value = value * 16 + *ip - 'a' + 10;
            }
            else if(*ip >= 'A' && *ip <= 'F')
            {
                value = value * 16 + *ip - 'A' + 10;
            }
            else
            {
                if(first)
                {
                    // not even one digit, not good
                    return false;
                }
                // not valid hexadecimal, may be ':' or '\0' (tested below)
                break;
            }
            if(value >= 0x10000)
            {
                // too large, must be 16 bit numbers
                return false;
            }
        }
        ++count;
//std::cerr << count << ". value=" << value << " -> " << static_cast<int>(*ip) << "\n";
        if(*ip == '\0')
        {
            break;
        }

        // note: if we just found a '::' then here *ip == ':' still
        if(*ip == '.')
        {
            // if we have a '.' we must end with an IPv4 and we either
            // need found_colon_colon to be true or the count must be
            // exactly 6 (1 "missing" colon)
            //
            if(!found_colon_colon
            && count != 7)  // we test with 7 because the first IPv4 number was already read
            {
                return false;
            }
            // also the value is 0 to 255 or it's an error too, but the
            // problem here is that we need a decimal number and we just
            // checked it as an hexadecimal...
            //
            if((value & 0x00f) >= 0x00a
            || (value & 0x0f0) >= 0x0a0
            || (value & 0xf00) >= 0xa00)
            {
                return false;
            }
            // transform back to a decimal number to verify the max.
            //
            value = (value & 0x00f) + (value & 0x0f0) / 16 * 10 + (value & 0xf00) / 256 * 100;
            if(value > 255)
            {
                return false;
            }
            // now check the other numbers
            int pos(1); // start at 1 since we already have 1 number checked
            for(++ip; *ip != '\0'; ++ip, ++pos)
            {
                if(*ip < '0' || *ip > '9' || pos >= 4)
                {
                    // not a valid number
                    return false;
                }

                // only expect decimal in this case in class d (a.b.c.d)
                value = 0;
                for(; *ip >= '0' && *ip <= '9'; ++ip)
                {
                    value = value * 10 + *ip - '0';
                    if(value > 255)
                    {
                        // too large
                        return false;
                    }
                }

                if(*ip != '.')
                {
                    if(*ip != '\0')
                    {
                        return false;
                    }
                    break;
                }
            }

            // we got a valid IPv4 at the end of IPv6 and we
            // found the '\0' so we are all good...
            //
            return true;
        }

        if(*ip != ':')
        {
            return false;
        }

        // double colon?
        if(ip[1] == ':')
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
            if(!found_colon_colon && count < 6)
#pragma GCC diagnostic pop
            {
                // we can accept one '::'
                ++ip;
                found_colon_colon = true;
            }
            else
            {
                // a second :: is not valid for an IPv6
                return false;
            }
        }
    }

    return count == 8 || (count >= 1 && found_colon_colon);
}


/** \brief Retrieve an address and a port from a string.
 *
 * This function breaks up an address and a port number from a string.
 *
 * The address can either be an IPv4 address followed by a colon and
 * the port number, or an IPv6 address written between square brackets
 * ([::1]) followed by a colon and the port number. We also support
 * just a port specification as in ":4040".
 *
 * Port numbers are limited to a number between 1 and 65535 inclusive.
 * They can only be specified in base 10.
 *
 * The port is optional only if a \p default_port is provided (by
 * default the \p default_port parameter is set to zero meaning that
 * it is not specified.)
 *
 * If the addr_port string is empty, then the addr and port parameters
 * are not modified, which means you want to define them with defaults
 * before calling this function.
 *
 * \exception snapwebsites_exception_invalid_parameters
 * If any parameter is considered invalid (albeit the validity of the
 * address is not checked since it could be a fully qualified domain
 * name) then this exception is raised.
 *
 * \param[in] addr_port  The address and port pair.
 * \param[in,out] addr  The address part, without the square brackets for
 *                IPv6 addresses.
 * \param[in,out] port  The port number (1 to 65535 inclusive.)
 * \param[in] protocol  The protocol for the port (i.e. "tcp" or "udp")
 */
void get_addr_port(QString const & addr_port, QString & addr, int & port, char const * protocol)
{
    // if there is a colon, we may have a port or IPv6
    //
    int const p(addr_port.lastIndexOf(":"));
    if(p != -1)
    {
        QString port_str;

        // if there is a ']' then we have an IPv6
        //
        int const bracket(addr_port.lastIndexOf("]"));
        if(bracket != -1)
        {
            // we must have a starting '[' otherwise it is wrong
            //
            if(addr_port[0] != '[')
            {
                SNAP_LOG_FATAL("invalid address/port specification in \"")(addr_port)("\" (missing '[' at the start.)");
                throw tcp_client_server_parameter_error("get_addr_port(): invalid [IPv6]:port specification, '[' missing.");
            }

            // extract the address
            //
            addr = addr_port.mid(1, bracket - 1); // exclude the '[' and ']'

            // is there a port?
            //
            if(p == bracket + 1)
            {
                // IPv6 port specification is just after the ']'
                //
                port_str = addr_port.mid(p + 1); // ignore the ':'
            }
            else if(bracket != addr_port.length() - 1)
            {
                // the ']' is not at the very end when no port specified
                //
                SNAP_LOG_FATAL("invalid address/port specification in \"")(addr_port)("\" (']' is not at the end)");
                throw tcp_client_server_parameter_error("get_addr_port(): invalid [IPv6]:port specification, ']' not at the end.");
            }
        }
        else
        {
            // IPv4 port specification
            //
            if(p > 0)
            {
                // if p is zero, then we just had a port (:4040)
                //
                addr = addr_port.mid(0, p); // ignore the ':'
            }
            port_str = addr_port.mid(p + 1); // ignore the ':'
        }

        // if port_str is still empty, we had an IPv6 without port
        //
        if(!port_str.isEmpty())
        {
            // first check whether the port is a number
            //
            bool ok(false);
            port = port_str.toInt(&ok, 10); // force base 10
            if(!ok)
            {
                // not a valid number, try to get it from /etc/services
                //
                struct servent const * s(getservbyname(port_str.toUtf8().data(), protocol));
                if(s == nullptr)
                {
                    SNAP_LOG_FATAL("invalid port specification in \"")(addr_port)("\", port not a decimal number nor a known service name.");
                    throw tcp_client_server_parameter_error("get_addr_port(): invalid addr:port specification, port number or name is not valid.");
                }
                port = s->s_port;
            }
        }
    }
    else if(!addr_port.isEmpty())
    {
        // just an IPv4 address specified, no port
        //
        addr = addr_port;
    }

    // the address could end up being the empty string here
    if(addr.isEmpty())
    {
        SNAP_LOG_FATAL("invalid address/port specification in \"")(addr_port)("\", address is empty.");
        throw tcp_client_server_parameter_error("get_addr_port(): invalid addr:port specification, address is empty (this generally happens when a request is done with no default address).");
    }

    // finally verify that the port is in range
    if(port <= 0
    || port > 65535)
    {
        SNAP_LOG_FATAL("invalid address/port specification in \"")(addr_port)("\", port out of bounds.");
        throw tcp_client_server_parameter_error("get_addr_port(): invalid addr:port specification, port number is out of bounds (1 .. 65535).");
    }
}


} // namespace tcp_client_server
// vim: ts=4 sw=4 et
