/////////////////////////////////////////////////////////////////////////////////
// Snap Mail Email Processor

// Copyright (c) 2018-2019  Made to Order Software Corp.  All Rights Reserved
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
#include <snapwebsites/mkdir_p.h>


// snapdev lib
//
#include <snapdev/not_reached.h>
#include <snapdev/not_used.h>


// advgetopt lib
//
#include <advgetopt/advgetopt.h>


// boost lib
//
#include <boost/algorithm/string/replace.hpp>


// C++ lib
//
#include <fstream>


// C lib
//
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>


// included last
//
#include <snapdev/poison.h>



namespace
{


// TODO: should we use the same lock file as other MTAs?
//
char const * g_lock_filename = "/run/lock/snapwebsites/sendmail.lock";


char const * g_root_mail = "/var/mail/root";


bool is_newline(char const c)
{
    return c == '\r' || c == '\n';
}


bool is_space(char const c)
{
    return c == ' ' || c == '\t';
}




class sendmail
{
public:
    typedef std::vector<std::string>    args_t;

                        sendmail(args_t args);

    int                 run();

private:
    int                 init();
    bool                read_email();       // read email from stdin (because msmtp reads at least the header which would prevent us from saving it to /var/mail/root)
    bool                smtp();             // attempt sending email to SMTP server
    void                usage();
    int                 enqueue();
    int                 dequeue();
    bool                lock();
    bool                become_root();

    args_t              f_args = args_t();
    std::string         f_email = std::string();
    std::string         f_msmtp_command = std::string();
    snap::raii_fd_t     f_lock_fd = snap::raii_fd_t();
    bool                f_dequeue_emails = false;
    bool                f_no_dequeue = false;
    bool                f_locked = false;
    bool                f_debug = false;
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

    // known bug: if you use --dequeue-mail twice on the command line then
    //            it gets sent to msmtp
    {
        auto it(std::find(f_args.begin()
                   , f_args.end()
                   , "--debug"));
        if(it != f_args.end())
        {
            f_debug = true;
            f_args.erase(it);
        }
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
    if(isatty(fileno(stdout)))
    {
        // if run in a TTY, use console instead of syslog
        //
        snap::logging::configure_console();
    }
    else
    {
        snap::logging::configure_syslog();
    }

    // change the log level to TRACE if --debug was used
    //
    snap::logging::set_log_output_level(f_debug
                    ? snap::logging::log_level_t::LOG_LEVEL_TRACE
                    : snap::logging::log_level_t::LOG_LEVEL_INFO);

    if(f_args.size() <= 1
    && !f_dequeue_emails)
    {
        std::cerr << "error: at least one recipient is expected on the command line when --dequeue-emails is not used." << std::endl;
        return 1;
    }

    // create the command line
    //
    f_msmtp_command = "msmtp";

    // add all the arguments (we removed our specific arguments already)
    //
    std::for_each(
              f_args.begin() + 1   // skip argv[0]
            , f_args.end()
            , [this](auto const & a)
            {
                f_msmtp_command += " \"";
                f_msmtp_command += boost::replace_all_copy(a, "\"", "\\\"");
                f_msmtp_command += "\"";
            });

    // for debug purposes we suppot a --send-error argument which
    // needs to be removed here
    {
        auto it(std::find(f_args.begin()
                   , f_args.end()
                   , "--send-error"));
        if(it != f_args.end())
        {
            f_args.erase(it);
        }
    }

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
        if(!read_email())
        {
            SNAP_LOG_FATAL("no email data sent via stdin, nothing can be sent.");
            return 1;
        }

        // try running msmtp unless the --dequeue-mail was used
        //
        if(!smtp())
        {
            // msmtp did not work, so save that email in /var/mail/root
            // and we are done with an error if the enqueue fails
            //
            SNAP_LOG_INFO("smtp() failed, enqueue email instead.");
            int const r(enqueue());
            if(r != 0)
            {
                // TODO: if this happens much, we want to look into a fallback
                //
                SNAP_LOG_FATAL("could not enqueue, email is lost.");
            }
            return r;
        }

        // we know that msmtp works, so allow for additional dequeuing
        // if --no-dequeue was not used
        //
        f_dequeue_emails = !f_no_dequeue;
    }

    if(f_dequeue_emails)
    {
        // msmtp worked or --dequeue-mail was used on the command line
        // try sending more emails
        //
        SNAP_LOG_DEBUG("attempt dequeuing.");
        return dequeue();
    }

    return 0;
}


bool sendmail::read_email()
{
    // TODO: use a buffer to load many characters at once, this is slow!?
    //       also we should check for errors
    //

    // ignore \r & \n at the start (empty lines at the beginning are
    // not valid, though--there should be headers like Subject: and
    // Data:)
    //
    int c(getchar());
    while(c == '\r' || c == '\n')
    {
        c = getchar();
    }

    while(c != EOF)
    {
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
        c = getchar();
    }

    return !f_email.empty();
}


bool sendmail::smtp()
{
    // WARNING: note that we want to know whether the msmtp command worked
    //          or not which is why we explicitly use popen() and pclose().
    //
    FILE * p(popen(f_msmtp_command.c_str(), "w"));
    fwrite(f_email.c_str(), f_email.length(), 1, p);
    errno = 0; // reset error to make sure it is an error on exit of pclose())
    int const r(pclose(p));
    if(r != 0)
    {
        int const e(errno);
        if(e == 0)
        {
            SNAP_LOG_ERROR("could not run \"")
                          (f_msmtp_command)
                          ("\", got exit code ")
                          (r >> 8)
                          (" instead of 0.");
        }
        else
        {
            SNAP_LOG_ERROR("could not run \"")
                          (f_msmtp_command)
                          ("\", got exit code ")
                          (r >> 8)
                          (" instead of 0. (errno: ")
                          (e)
                          (" -- ")
                          (strerror(e))
                          (")");
        }
    }
    return r == 0;
}


bool sendmail::become_root()
{
    // change both, user and group
    //
    bool const r(setuid(0) == 0 && setgid(0) == 0);
    if(!r)
    {
        SNAP_LOG_ERROR("sendmail could not become root.");
    }
    return r;
}


bool sendmail::lock()
{
    if(f_locked)
    {
        return true;
    }

    // attempt creating the directory, just in case
    // (it should already be there on a valid snap install)
    {
        int const r(snap::mkdir_p(g_lock_filename, true));
        if(r == -1)
        {
            SNAP_LOG_FATAL("could not create path to lock file \"")
                          (g_lock_filename)
                          ("\".");
            return false;
        }
    }

    f_lock_fd.reset(open(g_lock_filename
                   , O_CLOEXEC | O_CREAT
                   , S_IRUSR | S_IWUSR));
    if(f_lock_fd == nullptr)
    {
        SNAP_LOG_ERROR("could not open lock filename \"")
                      (g_lock_filename)
                      ("\".");
        return false;
    }
    // TODO: we probably have access to some parameters with the name/group
    //       instead of hard coding those here?
    //
    snap::chownnm(QString::fromUtf8(g_lock_filename)
                , "snapwebsites"
                , "snapwebsites");

    if(flock(f_lock_fd.get(), LOCK_EX) != 0)
    {
        // no need to keep the file opened
        //
        f_lock_fd.reset();

        SNAP_LOG_ERROR("could not obtain lock on \"")
                      (g_lock_filename)
                      ("\".");
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
        SNAP_LOG_ERROR("could not open \"")
                      (g_root_mail)
                      ("\". Can't enqueue email.");
        return 1;
    }
    // Linux usually fixes the ownership of emails but we should have the
    // correct user/group anyway, so here we go:
    //
    snap::chownnm(QString::fromUtf8(g_root_mail) , "root" , "mail");

    time_t const now(time(nullptr));
    if(now == static_cast<time_t>(-1))
    {
        SNAP_LOG_ERROR("time() failed.");
        return 1;
    }
    struct tm * t(gmtime(&now));
    if(t == nullptr)
    {
        SNAP_LOG_ERROR("gmtime() failed.");
        return 1;
    }
    char date[64];
    if(strftime(date, sizeof(date), "%a %b %d %H:%M:%S %Y", t) == 0)
    {
        // we can't properly handle the case when strftime() returns 0
        //
        SNAP_LOG_ERROR("strftime() failed.");
        return 1;
    }

    // email separator
    //
    m << "From SNAP_WEBSITES_SENDMAIL_TOOL@localhost  "
      << date
      << "\r\n";

    // the command arguments so we can restore them when dequeuing
    //
    m << "X-Snap-Sendmail-Args: ";

    // TODO: we should check the length and if more than 80 chars break
    //       the args on multiple lines; we currently are likely to only
    //       use tools that support any length, but you never know what
    //       users os Snap! Websites are going to have to read emails
    //
    if(f_args.size() > 1)
    {
        m << boost::replace_all_copy(f_args[1], ",", "\\,");

        if(f_args.size() > 2)
        {
            std::for_each(
                      f_args.begin() + 2   // skip argv[0] and argv[1]
                    , f_args.end()
                    , [&m](auto const & a)
                    {
                        m << ","
                          << boost::replace_all_copy(a, ",", "\\,");
                    });
        }
    }

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
    if(fd == nullptr)
    {
        int const e(errno);
        if(e == ENOENT)
        {
            // this is not an error, the file doesn't exist so there
            // is nothing to dequeue
            //
            SNAP_LOG_TRACE("nothing to dequeue");
            return 0;
        }

        // any other error we generate an error message
        //
        SNAP_LOG_ERROR("could not open \"")
                      (g_root_mail)
                      ("\"; can't dequeue email (errno: ")
                      (e)
                      (" -- ")
                      (strerror(e))
                      (").");
        return 1;
    }

    ssize_t const size(lseek(fd.get(), 0, SEEK_END));
    if(size == -1)
    {
        SNAP_LOG_ERROR("could not determine size of \"")
                      (g_root_mail)
                      ("\"; can't dequeue email.");
        return 1;
    }
    if(lseek(fd.get(), 0, SEEK_SET) == -1)
    {
        SNAP_LOG_ERROR("could not rewind \"")
                      (g_root_mail)
                      ("\"; can't dequeue email.");
        return 1;
    }

    SNAP_LOG_DEBUG("ready to mmap() \"")
                  (g_root_mail)
                  ("\" (size: ")
                  (size)
                  (")");

    // mmap returns a void * so we castto get a char * which is more useful
    //
    class emails
    {
    public:
        emails(int fd, ssize_t size)
            : f_fd(fd)
            , f_size(size)
        {
            f_emails = reinterpret_cast<char *>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

            // WARNING: `mmap()` returns `MAP_FAILED` and not `nullptr`
            //
            f_valid = f_emails != MAP_FAILED;
            if(!f_valid)
            {
                int const e(errno);
                SNAP_LOG_ERROR("could not mmap() \"")
                              (g_root_mail)
                              ("\" (errno: ")
                              (e)
                              (" -- ")
                              (strerror(e))
                              (".)");
                return;
            }

            f_end = f_emails + f_size;
            f_ptr = f_emails;
            f_last_valid = f_emails;
        }

        emails(emails const & rhs) = delete;
        emails & operator = (emails const & rhs) = delete;

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
                    // Note: the unlink() works only if the /var/mail/root
                    //       file is only accessed by us, otherwise, it may
                    //       break as other tools could add/remove to the
                    //       file in parallel
                    //
                    unlink(g_root_mail);
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
            if(f_ptr + 5 > f_end
            || memcmp(f_ptr, "From ", 5) != 0)
            {
                SNAP_LOG_FATAL("the format of \"")
                              (g_root_mail)
                              ("\" is not understood. Can't dequeue anymore.");
                return std::string();
            }

            // skip first line, it's not part of the SMTP data
            // (This is the From <name>@localhost <date>)
            //
            for(; f_ptr < f_end && !is_newline(*f_ptr); ++f_ptr);
            for(; f_ptr < f_end && is_newline(*f_ptr); ++f_ptr);
            if(f_ptr >= f_end)
            {
                // somehow we reached the end of the file too soon!?
                //
                SNAP_LOG_FATAL("somehow we have a \"From ...\" that's not followed by an email in \"")
                              (g_root_mail)
                              ("\".");
                return std::string();
            }

            // find the next "From " or EOF
            //
            char * n(f_ptr);
            for(; n + 6 < f_end; ++n)
            {
                if(n[0] == '\n'
                && (n[1] == 'F' || n[1] == 'f')
                && (n[2] == 'R' || n[2] == 'r')
                && (n[3] == 'O' || n[3] == 'o')
                && (n[4] == 'M' || n[4] == 'm')
                && n[5] == ' ')
                {
                    break;
                }
            }
            if(n + 6 >= f_end)
            {
                // skip the last few characters if we reached EOF
                //
                n = f_end;
            }

            // remove the empty line "\r" if present
            //
            if(n > f_ptr
            && n[-1] == '\r')
            {
                --n;
            }

            // if we do not find another email we are on the last email
            // already, it goes from f_ptr to f_end
            //
            std::string result(f_ptr, n - f_ptr);
            f_args.clear();

            // check for an X-Snap-Sendmail-Args header, if present we
            // want to read it and remove it too
            //
            std::string::size_type const args_pos(result.find("X-Snap-Sendmail-Args:"));
            if(args_pos != std::string::npos)
            {
                // found it, find the end and then extract
                //
                std::string::size_type end_pos(args_pos);
                for(;;)
                {
                    end_pos = result.find("\n", end_pos);
                    if(end_pos == std::string::npos)
                    {
                        end_pos = result.length();
                    }
                    if(end_pos + 1 >= result.length()
                    || !is_space(result[end_pos + 1]))
                    {
                        break;
                    }
                }

                // extract the args including newlines
                //
                char const * a(result.c_str() + args_pos + 21);
                char const * e(result.c_str() + end_pos);

                // skip spaces (there is likely one after the ':')
                //
                for(; a < e && std::isspace(*a); ++a);

                std::string arg;
                while(a < e)
                {
                    if(is_newline(*a))
                    {
                        do
                        {
                            ++a;
                        }
                        while(a < e && (is_space(*a) || is_newline(*a)));
                    }
                    else if(*a == ',')
                    {
                        f_args.push_back(arg);
                        arg.clear();
                        ++a;
                    }
                    else
                    {
                        if(*a == '\\')
                        {
                            // escaped characters, remove the \ and keep the
                            // next character whatever it is (usually a ,)
                            //
                            if(a + 1 < e)
                            {
                                ++a;
                                arg += *a;
                                ++a;
                            }
                        }
                        else
                        {
                            arg += *a;
                            ++a;
                        }
                    }
                }
                if(!arg.empty())
                {
                    // this should happen every time with the last argument
                    //
                    f_args.push_back(arg);
                }

                for(; end_pos < result.length() && is_newline(result[end_pos]); ++end_pos);
                result = result.substr(0, args_pos) + result.substr(end_pos);
            }

            if(f_args.empty())
            {
                // looks like we did not have a header named X-Snap-Sendmail-Args
                // so instead we'll use "To: ..."
                //
                char const * h(result.c_str());
                char const * e(h + result.length());
                while(h + 3 < e)
                {
                    if((h[0] == 'T' || h[0] == 't')
                    && (h[1] == 'O' || h[1] == 'o')
                    && h[2] == ':')
                    {
                        h += 3; // skip the "To:"

                        // skip the space(s) after To: ...
                        //
                        for(; h < e && is_space(*h); ++h);

                        // this is the beginning of the email address
                        //
                        char const * email_start(h);
                        for(; h < e && !is_newline(*h); ++h);
                        std::string const arg(std::string(email_start, h - email_start));
                        f_args.push_back(arg);
                        break;
                    }
                    else
                    {
                        // not To:, skip this entire line
                        //
                        for(; h < e && !is_newline(*h); ++h);
                        for(; h < e && is_newline(*h); ++h);
                    }
                }
                result.find("To:");
            }

            if(f_args.empty())
            {
                SNAP_LOG_ERROR("email has no arguments or To: header.");
                return std::string();
            }

            // remove all the '\r' and '\n' as required
            //
            while(n < f_end && is_newline(*n))
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

        sendmail::args_t const & args() const
        {
            return f_args;
        }

    private:
        int const           f_fd;                           // the file description
        char *              f_emails = nullptr;             // this is the start address, do not change it
        char *              f_end = nullptr;                // end of file
        char *              f_ptr = nullptr;                // current pointer
        char *              f_last_valid = nullptr;         // end of last valid email
        ssize_t const       f_size;                         // this is the total length, do not change it
        bool                f_valid = false;                // whether this email object is valid
        sendmail::args_t    f_args = sendmail::args_t();    // arguments as found in the email (X-Snap-Sendmail-Args or To)
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

//std::cerr << "------------------------------------------------------ NEXT EMAIL:\n"
//          << f_email
//          << "==================================================================\n";

        // generate the command line for that specific email
        //
        f_msmtp_command = "msmtp";
        sendmail::args_t const & args(e.args());
        std::for_each(
                  args.begin()       // DO NOT skip argv[0], in this case we don't include a progname
                , args.end()
                , [this](auto const & a)
                {
                    f_msmtp_command += " \"";
                    f_msmtp_command += boost::replace_all_copy(a, "\"", "\\\"");
                    f_msmtp_command += "\"";
                });

//std::cerr << "++++++++++++ NEW COMMAND: [" << f_msmtp_command << "]\n"
//          << "\n\n\n\n";


        if(!smtp())
        {
            SNAP_LOG_ERROR("smtp() failed, while dequeuing email.");
            return 1;
        }
        e.email_sent();
    }

    SNAP_LOG_DEBUG("full dequeue succeeded.");

    return 0;
}


}
// no name namespace



int main(int argc, char * argv [])
{
#if 0
    // since sendmail may be invoked from CRON and other tools, it may
    // be useful to see the command line arguments used in those situations
    // the following helps we determining such
    //
    // my problem on my test server was that my hourly cron script was not
    // executable (chmod 755 /etc/cron.hourly/myscript) and thus it did not
    // kick start at all...
    {
        snap::raii_fd_t fd;
        fd.reset(open("/tmp/sendmail-run.txt"
                       , O_CLOEXEC | O_CREAT | O_RDWR
                       , S_IRUSR | S_IWUSR));
        if(fd != nullptr)
        {
            std::string msg("args =\n");
            snap::NOTUSED(write(fd.get(), msg.c_str(), msg.length()));

            for(int idx(0); idx < argc; ++idx)
            {
                msg = std::to_string(idx) + ". " + argv[idx] + "\n";
                snap::NOTUSED(write(fd.get(), msg.c_str(), msg.length()));
            }
        }
        else
        {
            std::cerr << "error: fd for \"/tmp/sendmail-run.txt\" is nullptr?\n";
        }
    }
#endif

    try
    {
        // get the command line parameters, we forward them to msmtp which
        // is mostly compatible with sendmail
        //
        std::vector<std::string> args;
        for(int idx(0); idx < argc; ++idx)
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
