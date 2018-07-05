/////////////////////////////////////////////////////////////////////////////////
// Snap Mail Email Processor

// Copyright (c) 2018  Made to Order Software Corp.  All Rights Reserved
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
// This server reads in a configuration file and keeps specified services running.
// When signaled, it will terminate those services cleanly.
/////////////////////////////////////////////////////////////////////////////////

/** \file
 * \brief Our own /usr/sbin/sendmail tool.
 *
 * We use msmtp to send SMTP mail messages to Postfix computers. However,
 * once in a while that other computer may not be running (maybe it's being
 * rebooted or it crashed.)
 *
 * When that happens, our sendmail saves the email in /var/mail/root as
 * a fallback.
 *
 * The tool can be run to forward the emails found in /var/mail/root to
 * the SMTP server when that works. In other words, /var/mail/root can
 * be seen as an equivalent to an email queue.
 */

#include "version.h"

// snapwebsites lib
//
#include <snapwebsites/chownnm.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>
//#include <snapwebsites/raii_generic_deleter.h>
//#include <snapwebsites/snap_exception.h>
//#include <snapwebsites/snap_thread.h>
//#include <snapwebsites/snapwebsites.h>

// contrib lib
//
#include <advgetopt/advgetopt.h>

//// Qt lib
////
//#include <QFile>
//#include <QTime>

// C++ lib
//
//#include <exception>
//#include <memory>
//#include <vector>
//#include <iostream>
#include <fstream>
//#include <algorithm>

// C lib
//
//#include <stdlib.h>
#include <fcntl.h>
//#include <unistd.h>
#include <sys/file.h>
 #include <sys/mman.h>
//#include <sys/wait.h>
//#include <uuid/uuid.h>


// included last
//
#include <snapwebsites/poison.h>



namespace
{


// TODO: should we use the same lock file as other MTAs?
//
char const * g_lock_filename = "/run/lock/snapwebsites/sendmail.lock";


char const * g_root_mail = "/var/mail/root";



class sendmail
{
public:
    typedef std::vector<std::string>    args_t;

                        sendmail(args_t args);

    int                 run();

private:
    int                 init();
    void                read_email();       // read email from stdin (because msmtp reads at least the header which would prevent us from saving it to /var/mail/root)
    bool                smtp();             // attempt sending email to SMTP server
    void                usage();
    int                 enqueue();
    int                 dequeue();
    bool                lock();
    bool                become_root();

    args_t              f_args;
    std::string         f_email;
    std::string         f_msmtp_command;
    int                 f_lock_fd = false;
    bool                f_dequeue_emails = false;
    bool                f_no_dequeue = false;
    bool                f_locked = false;
};


sendmail::sendmail(args_t args)
    : f_args(args)
{
}


int sendmail::init()
{
    if(std::find(f_args.begin()
               , f_args.end()
               , "--version") != f_args.end())
    {
        std::cerr << SNAPBOUNCE_VERSION_STRING << std::endl;
        return -1;
    }

    if(std::find(f_args.begin()
               , f_args.end()
               , "--help") != f_args.end())
    {
        usage();
        return 1;
    }

    // known bug: if you use --no-dequeue twice on the command line then
    //            it gets sent to msmtp
    {
        auto it(std::find(f_args.begin()
                   , f_args.end()
                   , "--no-dequeue"));
        if(it != f_args.end())
        {
            f_no_dequeue = true;
            f_args.erase(it);
        }
    }

    // known bug: if you use --dequeue-mail twice on the command line then
    //            it gets sent to msmtp
    {
        auto it(std::find(f_args.begin()
                   , f_args.end()
                   , "--dequeue-emails"));
        if(it != f_args.end())
        {
            f_dequeue_emails = true;
            f_args.erase(it);
        }
    }

    if(f_no_dequeue
    && f_dequeue_emails)
    {
        std::cerr << "error: --no-dequeue and --dequeue-emails can't be used together." << std::endl;
        return 1;
    }

    // initialize the logs
    //
    static_assert(std::string::npos + 1 == 0, "somehow your std::string::npos value is not -1, algorithm change required");
    int const s(f_args[0].rfind('/') + 1);
    int const e(f_args[0].length() - s);
    std::string const basename(f_args[0].substr(s, e));
    snap::logging::set_progname(basename);
    snap::logging::configure_syslog();

    // create the command line
    //
    f_msmtp_command = "msmtp";

    // add all the arguments (we removed our specific arguments already)
    //
    std::for_each(
              f_args.begin()
            , f_args.end()
            , [this](auto const & a)
            {
                f_msmtp_command += " ";
                f_msmtp_command += a;
            });

    return 0;
}


void sendmail::usage()
{
    std::cerr << "Usage: cat mail.eml | sendmail [-opts] recipient" << std::endl;
    std::cerr << "where -opts are one of:" << std::endl;
    std::cerr << "  --dequeue-emails  attempt to dequeue, do not send an email now" << std::endl;
    std::cerr << "  --no-dequeue      prevent any dequeue" << std::endl;
    std::cerr << "  --help            print out this help screen" << std::endl;
    std::cerr << "  --version         print out the version of Snap! sendmail" << std::endl;
    std::cerr << "  -... | --...      option passed down to msmtp (see man msmtp)" << std::endl;
}


int sendmail::run()
{
    {
        int const r(init());
        if(r != 0)
        {
            return r > 0 ? r : 0;
        }
    }

    // if --dequeue-mail was used, we do not expect any input from stdin
    // and don't attempt to call msmtp for that
    //
    // we may still use msmtp to handle emails from /var/mail/root
    //
    if(!f_dequeue_emails)
    {
        // otherwise read email from stdin
        //
        read_email();

        // try running msmtp unless the --dequeue-mail was used
        //
        if(!smtp())
        {
            // msmtp did not work, so save that email in /var/mail/root
            // and we're done with an error if the enqueue fails
            //
            return enqueue();
        }

        // we know that msmtp works, so allow for additional dequeuing
        //
        f_dequeue_emails = true;
    }

    if(f_dequeue_emails)
    {
        // msmtp worked or --dequeue-mail was used on the command line
        // try sending more emails
        //
        return dequeue();
    }

    return 0;
}


void sendmail::read_email()
{
    // TODO: use a buffer to load many characters at once, this is slow!?
    //       also we should check for errors
    //
    for(;;)
    {
        int const c(getchar());
        if(c == EOF)
        {
            return;
        }
        // make sure all lines end with "\r\n"
        // (this may fail under Mac if they still use "\r" instead of "\n"
        // for newlines)
        //
        if(c != '\r')
        {
            if(c == '\n')
            {
                f_email += static_cast<char>('\r');
            }
            f_email += static_cast<char>(c);
        }
    }
}


bool sendmail::smtp()
{
    // WARNING: note that we want to know whether the msmtp command worked
    //          or not which is why we explicitly use popen() and pclose().
    //
    FILE * p(popen(f_msmtp_command.c_str(), "w"));
    fwrite(f_email.c_str(), f_email.length(), 1, p);
    int const r(pclose(p));
    return r == 0;
}


bool sendmail::become_root()
{
    // change both, user and group
    //
    return setuid(0) == 0
        && setgid(0) == 0;
}


bool sendmail::lock()
{
    if(f_locked)
    {
        return true;
    }

    f_lock_fd = open(g_lock_filename
                   , O_CLOEXEC | O_CREAT
                   , S_IRUSR | S_IWUSR);
    if(f_lock_fd == -1)
    {
        return false;
    }
    // TODO: we probably have access to some parameters with the name/group
    //       instead of hard coding those here?
    //
    snap::chownnm(QString::fromUtf8(g_lock_filename)
                , "snapwebsites"
                , "snapwebsites");

    if(flock(f_lock_fd, LOCK_EX) != 0)
    {
        return false;
    }

    f_locked = true;

    return true;
}


int sendmail::enqueue()
{
    // at this point we have to be root, so become root now
    //
    if(!become_root())
    {
        return 1;
    }

    // make sure we are the only ones working on the /var/mail/root file
    //
    if(!lock())
    {
        return 1;
    }

    std::ofstream m;
    m.open(g_root_mail, std::ios_base::app | std::ios_base::binary | std::ios_base::out);
    if(!m.is_open())
    {
        return 1;
    }

    time_t const now(time(nullptr));
    if(now == static_cast<time_t>(-1))
    {
        return 1;
    }
    struct tm * t(gmtime(&now));
    if(t != nullptr)
    {
        return 1;
    }
    char date[64];
    if(strftime(date, sizeof(date), "%a %b %d %H:%M:%S %Y", t) == 0)
    {
        // we can't properly handle the case when strftime() returns 0
        //
        return 1;
    }

    // email separator
    //
    m << "From SNAP_WEBSITES_SENDMAIL_TOOL@localhost  ";
    m << date;
    m << "\r\n";

    // email body
    //
    m << f_email;

    // an empty line between each email
    //
    m << "\r\n";

    return 0;
}


int sendmail::dequeue()
{
    // at this point we have to be root, so become root now
    //
    if(!become_root())
    {
        return 1;
    }

    // make sure we are the only ones working on the /var/mail/root file
    //
    if(!lock())
    {
        return 1;
    }

    // open the /var/mail/root file and then mmap() it so that way we
    // can just deal with memory to attempt to send them
    //
    // if all the emails get sent, then we unlink() the file
    //
    // if some emails are sent and then an error occur, we stop and
    // remove only the emails that were sent; with mmap()'ed files
    // we can just use memmove() and truncate() to rearrange the file
    //
    snap::raii_fd_t fd;
    fd.reset(open(g_root_mail, O_RDWR));

    ssize_t const size(lseek(fd.get(), 0, SEEK_END));
    if(size == -1)
    {
        return 1;
    }
    if(lseek(fd.get(), 0, SEEK_SET) == -1)
    {
        return 1;
    }

    // mmap returns a void * so we castto get a char * which is more useful
    //
    class emails
    {
    public:
        emails(int fd, ssize_t size)
            : f_fd(fd)
            , f_emails(reinterpret_cast<char *>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)))
            , f_end(f_emails + size)
            , f_ptr(f_emails)
            , f_last_valid(f_emails)
            , f_size(size)
            , f_valid(f_emails == MAP_FAILED)  // WARNING: `mmap()` returns `MAP_FAILED` and not `nullptr`
        {
        }

        ~emails()
        {
            if(f_valid)
            {
                size_t sz(f_end - f_last_valid);
                if(sz > 0
                && f_last_valid > f_emails)
                {
                    // remove the emails that were sent, we're ftruncate()
                    // too, after the munmap() to make sure there is no
                    // conflicts between the two functions
                    //
                    memmove(f_emails, f_last_valid, sz);
                }

                munmap(f_emails, f_size);

                if(sz > 0)
                {
                    // it broke at some point, keep what's left
                    //
                    if(ftruncate(f_fd, sz) != 0)
                    {
                        SNAP_LOG_WARNING("ftruncate() failed to fix the file of \"")
                                        (g_root_mail)
                                        ("\" at the right size");
                    }
                }
                else
                {
                    // the file is now empty, unlink
                    // (we can have the file still open and truncate just fine)
                    //
                    unlink(g_lock_filename);
                }
            }
        }

        bool is_valid() const
        {
            return f_valid;
        }

        std::string get_next_email()
        {
            if(!f_valid
            || f_ptr >= f_end)
            {
                // no more email, an empty string signals EOF
                //
                return std::string();
            }

            char * n(std::find_if(
                      f_ptr
                    , f_end
                    , [](auto s)
                    {
                        return strcmp(&s, "\nFrom ") == 0;
                    }));

            // remove the empty line "\r" if present
            //
            if(n > f_ptr
            && *n == '\r')
            {
                --n;
            }

            // if we do not find another email we are on the last email
            // already, it goes from f_ptr to f_end
            //
            std::string const result(f_ptr, n - f_ptr);

            // remove all the '\r' and '\n' as required
            //
            while(n < f_end && (*n == '\r' || *n == '\n'))
            {
                ++n;
            }

            f_ptr = n;
            return result;
        }

        void email_sent()
        {
            f_last_valid = f_ptr;
        }

    private:
        int const       f_fd;                       // the file description
        char * const    f_emails;                   // this is the start address, do not change it
        char * const    f_end;                      // end of file
        char *          f_ptr = nullptr;            // current pointer
        char *          f_last_valid = nullptr;     // end of last valid email
        ssize_t const   f_size;                     // this is the total length, do not change it
        bool            f_valid = false;            // whether this email object is valid
    };

    emails e(fd.get(), size);
    if(!e.is_valid())
    {
        return 1;
    }

    for(;;)
    {
        f_email = e.get_next_email();
        if(f_email.empty())
        {
            // we reached the end of the file
            //
            break;
        }
        if(!smtp())
        {
            return 1;
        }
        e.email_sent();
    }

    return 0;
}


}
// no name namespace



int main(int argc, char * argv [])
{
    try
    {
        // get the command line parameters, we forward them to msmtp which
        // is mostly compatible with sendmail
        //
        std::vector<std::string> args;
        for(int idx(0); idx < argc; ++argv)
        {
            args.push_back(argv[idx]);
        }

        // First, create a snap_mail object
        //
        sendmail sm(args);
        return sm.run();
    }
    catch(snap::snap_exception const & except)
    {
        SNAP_LOG_FATAL("sendmail: snap_exception caught! ")(except.what());
        return 1;
    }
    catch(std::invalid_argument const & std_except)
    {
        SNAP_LOG_FATAL("sendmail: invalid argument: ")(std_except.what());
        return 1;
    }
    catch(std::exception const & std_except)
    {
        SNAP_LOG_FATAL("sendmail: std::exception caught! ")(std_except.what());
        return 1;
    }
    catch(...)
    {
        SNAP_LOG_FATAL("sendmail: unknown exception caught!");
        return 1;
    }
}

// vim: ts=4 sw=4 et
