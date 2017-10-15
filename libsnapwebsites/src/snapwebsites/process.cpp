// Snap Websites Server -- C++ object to run advance processes
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

// self
//
#include "snapwebsites/process.h"

// Our lib
//
#include "snapwebsites/log.h"

// C lib
//
#include <stdio.h>
#include <unistd.h>
#include <proc/readproc.h>
#include <sys/wait.h>

#include "snapwebsites/poison.h"


extern char ** environ;

namespace snap
{


/** \class process::process_output_callback
 * \brief Callback to inform the caller of new output.
 *
 * In case the input depends no the output of a command line process, we
 * create a callback. This class is used for that purpose.
 *
 * The callback is called any time some output is received and the read()
 * returns. The callback itself is called with exactly what is received.
 * However, the output data read from the read() function is added to the
 * f_output variable member of the process generating a larger output buffer,
 * eventually encompassing the whole output of the process.
 *
 * The process class shows a graph of the different processes including the
 * callback.
 *
 * \todo
 * We may want to reconsider keeping all the output of the read() function
 * in case we have a callback since that requires a lot of memory in one
 * single block.
 */


/** \fn process::process_output_callback::output_available();
 * \brief The function called any time output is available.
 *
 * This callback function is called whenever the read() of the output
 * thread returns some data. In case of the interactive version, that may
 * me quite often as the pipe is made non-blocking.
 *
 * The callback is generally expected to make use of the output and then
 * eventually send more input by calling the process set_input() function.
 *
 * In most cases, no output callback is defined unless interaction with
 * the output of the child process is required. In that case things go
 * faster as the data gets cached in many ways.
 *
 * Note that whether your callback function received enough data or not,
 * it must return normally for the thread to continue processing the
 * output of the child process.
 *
 * \todo
 * At this point the output thread is not protected against exceptions.
 * However, it is likely to block the entire process if an exception occurs!
 *
 * \param[in] p  A pointer to the process.
 * \param[in] output  The output just received by the last read() call.
 */





/** \class process
 * \brief A process class to run a process and get information about the results.
 *
 * This class is used to run processes. Especially, it can run with in and
 * out capabilities (i.e. piping) although this is generally not recommanded
 * because piping can block (if you do not send enough data, or do not read
 * enough data, then the pipes can get stuck.) We use a thread to read the
 * results. We do not currently expect that the use of this class will require
 * the input read to be necessary to know what needs to be written (i.e. in
 * most cases all we want is to convert a file [input] from one format to
 * another [output] avoiding reading/writing on disk.)
 *
 * The whole process, when using the interactive mode, is quite complicated
 * so I wrote the following diagram. As you can see, the loop of sending
 * and receiving data from the child process is fairly simple. Note that the
 * callback is called from the Output Thread, not the main process. This does
 * not make much of a difference because no other function can be running on
 * the main process when that happens. The output is blocked and thus the
 * output variable is safe. The input is not blocked but adding input was
 * made safe internally.
 *
 * \msc
 * hscale = "2";
 * a [label="Function"],b [label="Process"],c [label="Input Thread"],d [label="Output Thread"],e [label="Child Process"];
 *
 * a=>b [label="run()"];
 * b=>e [label="fork()"];
 * e->e [label="execvpe()"];
 * b=>c [label="pthread_create()"];
 * b=>d [label="pthread_create()"];
 * b=>e [label="wait() child's death"];
 *
 * --- [label="start repeat"];
 * c->e [label="write() (Input Data)"];
 * d<-e [label="read() (Output Data)"];
 * b<:d [label="output shared"];
 * a<<=d [label="output callback"];
 * a=>b [label="set_input()"];
 * b=>c [label="add input"];
 * --- [label="end repeat"];
 *
 * b<-e [label="child died"];
 * b->c [label="stop()"];
 * b<-c [label="stopped"];
 * b->d [label="stop()"];
 * b<-d [label="stopped"];
 * a<<b [label="run()"];
 * \endmsc
 */


/** \brief Initialize the process object.
 *
 * This function saves the name of the process. The name is generally a
 * static string and it is used to distinguish between processes when
 * managing several at once. The function makes a copy of the name.
 *
 * \param[in] name  The name of the process.
 */
process::process(QString const & name)
    : f_name(name)
    //, f_mode(mode_t::PROCESS_MODE_COMMAND) -- auto-init
    //, f_command("") -- auto-init
    //, f_arguments() -- auto-init
    //, f_environment() -- auto-init
    //, f_input() -- auto-init
    //, f_output() -- auto-init
    //, f_forced_environment("") -- auto-init
    //, f_output_callback(nullptr) -- auto-init
    //, f_mutex() -- auto-init
{
}


/** \brief Retrieve the name of this process object.
 *
 * This process object is given a name on creation. In most cases this is
 * a static name that is used to determine which process is which.
 *
 * \return The name of the process.
 */
QString const & process::get_name() const
{
    return f_name;
}


/** \brief Set the management mode.
 *
 * This function defines the mode that the process is going to use when
 * running. It cannot be changed once the process is started (the run()
 * function was called.)
 *
 * The available modes are:
 *
 * \li PROCESS_MODE_COMMAND
 *
 * Run a simple command (i.e. very much like system() would.)
 *
 * \li PROCESS_MODE_INPUT
 *
 * Run a process that wants some input. We write data to its input. It
 * does not generate output (i.e. sendmail).
 *
 * \li PROCESS_MODE_OUTPUT
 *
 * Run a process that generates output. We read the output.
 *
 * \li PROCESS_MODE_INOUT
 *
 * Run the process in a way so we can write input to it, and read its
 * output from it. This is one of the most useful mode. This mode does
 * not give you any interaction capabilities (i.e. what comes in the
 * output cannot be used to intervene with what is sent to the input.)
 *
 * This is extermely useful to run filter commands (i.e. html2text).
 *
 * \li PROCESS_MODE_INOUT_INTERACTIVE
 *
 * Run the process interactively, meaning that its output is going to be
 * read and interpreted to determine what the input is going to be. This
 * is a very complicated mode and it should be avoided if possible because
 * it is not unlikely that the process will end up blocking. To be on the
 * safe side, look into whether it would be possible to transform that
 * process in a server and connect to it instead.
 *
 * Otherwise this mode is similar to the in/out mode only the output is
 * used to know to further feed in the input.
 *
 * \param[in] mode  The mode of the process.
 */
void process::set_mode(mode_t mode)
{
    f_mode = mode;
}


/** \brief Set how the environment variables are defined in the process.
 *
 * By default all the environment variables from the current process are
 * passed to the child process. If the child process is not 100% trustworthy,
 * however, it may be preferable to only pass a specific set of environment
 * variable (as added by the add_environment() function) to the child process.
 * This function, when called with true (the default) does just that.
 *
 * \param[in] forced  Whether the environment will be forced.
 */
void process::set_forced_environment(bool forced)
{
    f_forced_environment = forced;
}


/** \brief Define the command to run.
 *
 * The command name may be a full path or just the command filename.
 * (i.e. the execvp() function makes use of the PATH variable to find
 * the command on disk unless the \p command parameter includes a
 * slash character.)
 *
 * If the process cannot be found an error is generated at the time you
 * call the run() function.
 *
 * \param[in] command  The command to start the new process.
 */
void process::set_command(QString const & command)
{
    f_command = command;
}


/** \brief Add an argument to the command line.
 *
 * This function adds one individual arguement to the command line.
 * You have to add all the arguments in the right order.
 *
 * \param[in] arg  The argument to be added.
 */
void process::add_argument(QString const & arg)
{
    f_arguments.push_back(arg);
}


/** \brief Add an environment to the command line.
 *
 * This function adds a new environment variable for the child process to
 * use. In most cases this function doesn't get used.
 *
 * By default all the parent process (this current process) environment
 * variables are passed down to the child process. To avoid this behavior,
 * call the set_forced_environment() function before the run() function.
 *
 * An environment variable is defined as a name, and a value as in:
 *
 * \code
 * add_environ("HOME", "/home/snap");
 * \endcode
 *
 * If the value is set to the empty string, then the environment variable
 * is removed from the list.
 *
 * \param[in] name  The name of the environment variable to add.
 * \param[in] value  The new value of that environment variable.
 */
void process::add_environ(QString const & name, QString const & value)
{
    if(value.isEmpty())
    {
        environment_map_t::const_iterator it(f_environment.find(name.toUtf8().data()));
        if(it != f_environment.end())
        {
            f_environment.erase(it);
        }
    }
    else
    {
        f_environment[name.toUtf8().data()] = value.toUtf8().data();
    }
}


/** \brief Run the process and return once done.
 *
 * This function creates all the necessary things that the process requires
 * and run the command and then return the exit code as the result.
 *
 * If the function encounters problems before it can run the child process,
 * it returns -1 instead.
 *
 * The process tries to use system() or popen() if possible making it less
 * likely to fail as those are programmed in the C library. If more is asked
 * of the process (i.e. input and output are required) then the more
 * complicated process is used, creating two pipes, creating a child process
 * with fork() and two threads to handle the read and writing of the input and
 * output pipes.
 *
 * \todo
 * The interactive version of the process is not yet implemented. We do not
 * need it yet so we did not work on it.
 *
 * \todo
 * The threads only handle standard exceptions. Non-standard exceptions
 * will likely terminate the whole process.
 *
 * \todo
 * Change the current pipe() call with the "two way pipes" socketpair()
 * if that indeed runs faster. Both sockets can be used to read and
 * write data, instead of 4 descriptors in case of the pipe() call.
 *
 * \todo
 * The raii_pipe class blocks the SIGPIPE and then restores it to whatever
 * it was before. This may clash with other such calls in other places.
 * We should instead have one place where we block such calls, always.
 *
 * \return The exit code of the child process (0 to 255)
 *         or -1 if an error occurs
 */
int process::run()
{
    class raii_pipe
    {
    public:
        raii_pipe(QString const & command, snap_string_list const & arguments)
        {
            QString cmd(command);
            if(!arguments.isEmpty())
            {
                cmd += " " + arguments.join(" ");
            }
            f_command = cmd;

            // block the SIGPIPE signal so the process does not end up with
            // a SIGPIPE error; instead you should be able to detect the
            // return type as erroneous (i.e. not 0.)
            //
            sigset_t set;
            sigemptyset(&set);
            sigaddset(&set, SIGPIPE);
            sigprocmask(SIG_BLOCK, &set, &f_signal_mask);
        }

        ~raii_pipe()
        {
            // make sure the f_file gets closed
            close_pipe();

            // restore the status of the process signal mask as it was
            // before entering the run() function
            //
            sigprocmask(SIG_BLOCK, &f_signal_mask, nullptr);
        }

        QString const & command_line() const
        {
            return f_command;
        }

        FILE * open_pipe(char const * mode)
        {
            f_file = popen(f_command.toUtf8().data(), mode);
            return f_file;
        }

        int close_pipe()
        {
            int r(-1);
            if(f_file != nullptr)
            {
                if(ferror(f_file))
                {
                    // must return -1 on error, ignore pclose() return value
                    int const e(pclose(f_file));
                    SNAP_LOG_ERROR("pclose() returned ")(e)(", but it will be ignored because the pipe was marked as erroneous.");
                    f_file = nullptr;
                }
                else
                {
                    r = pclose(f_file);
                    f_file = nullptr;
                }
            }
            return r;
        }

    private:
        FILE *                      f_file = nullptr;
        QString                     f_command;
        sigset_t                    f_signal_mask;
    };

    raii_pipe rp(/*this,*/ f_command, f_arguments);
    SNAP_LOG_INFO("Running process \"")(rp.command_line())("\" in mode ")(static_cast<int>(static_cast<mode_t>(f_mode)));

    // if the user imposes environment restrictions we cannot use system()
    // or popen(). In that case just use the more complex case anyway.
    if(!f_forced_environment && f_environment.empty())
    {
        switch(f_mode)
        {
        case mode_t::PROCESS_MODE_COMMAND:
            return system(rp.command_line().toUtf8().data());

        case mode_t::PROCESS_MODE_INPUT:
            {
                FILE * f(rp.open_pipe("w"));
                if(f == nullptr)
                {
                    return -1;
                }
                QByteArray data(f_input);
                if(fwrite(data.data(), data.size(), 1, f) != 1)
                {
                    return -1;
                }
                return rp.close_pipe();
            }

        case mode_t::PROCESS_MODE_OUTPUT:
            {
                FILE * f(rp.open_pipe("r"));
                if(f == nullptr)
                {
                    return -1;
                }
                while(!feof(f) && !ferror(f))
                {
                    char buf[4096];
                    size_t l(fread(buf, 1, sizeof(buf), f));
                    if(l > 0)
                    {
                        f_output.append(buf, static_cast<int>(l));
                    }
                }
                return rp.close_pipe();
            }

        default:
            // In/Out modes require the more complex case
            break;

        }
    }

    if(mode_t::PROCESS_MODE_INOUT_INTERACTIVE == f_mode && !f_output_callback)
    {
        // mode is not compatible with the current setup
        throw snap_process_exception_invalid_mode_error("mode cannot be in/out interactive without a callback");
    }

    // in this case we want to create a pipe(), fork(), execvp() the
    // command and have a thread to handle the output separately
    // from the input
    class raii_inout_pipes
    {
    public:
        raii_inout_pipes()
            //; f_pipes({0, 0}) -- initialized below
        {
            for(int i(0); i < 4; ++i)
            {
                f_pipes[i] = -1;
            }
        }

        ~raii_inout_pipes()
        {
            close();
        }

        void close()
        {
            for(int i(0); i < 4; ++i)
            {
                if(f_pipes[i] != -1)
                {
                    ::close(f_pipes[i]);
                    f_pipes[i] = -1;
                }
            }
        }

        int open()
        {
            close();
            if(pipe(f_pipes + 0) != 0)
            {
                return -1;
            }
            return pipe(f_pipes + 2);
        }

        int f_pipes[4];
    };
    raii_inout_pipes inout;
    if(inout.open() == -1)
    {
        return -1;
    }

    class raii_fork
    {
    public:
        raii_fork()
            //: f_child(-1)
            //, f_exit(-1)
        {
            f_child = fork();
        }

        ~raii_fork()
        {
            // in this case f_child should already be zero, if not we are
            // throwing or exiting with -1 anyway
            wait();
        }

        int wait()
        {
            // TODO: use wait4() to get usage and save that usage in the log
            if(f_child > 0)
            {
                int status;
                waitpid( f_child, &status, 0 );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
                if(WIFEXITED(status))
                {
                    f_exit = WEXITSTATUS(status);
                }
                else
                {
                    f_exit = -1;
                }
#pragma GCC diagnostic pop
            }
            f_child = -1;
            return f_exit;
        }

        int get_pid() const
        {
            return f_child;
        }

        int get_exit() const
        {
            return f_exit;
        }

    private:
        pid_t           f_child = -1;
        int             f_exit = -1;
    };
    raii_fork child;
    switch(child.get_pid())
    {
    case -1:
        // fork failed
        return -1;

    case 0:
        // child
        try
        {
            // convert arguments so we can use them with execvpe()
            std::vector<char const *> args_strings;
            std::string const cmd(f_command.toUtf8().data());
            args_strings.push_back(strdup(cmd.c_str()));
            int const args_max(f_arguments.size());
            for(int i(0); i < args_max; ++i)
            {
                args_strings.push_back(strdup(f_arguments[i].toUtf8().data()));
            }
            args_strings.push_back(nullptr);

            // convert arguments so we can use them with execvpe()
            environment_map_t src_envs(f_environment);
            if(!f_forced_environment)
            {
                // since we do not limit the child to only the specified
                // environment, add ours but do not overwrite anything
                for(char ** env(environ); *env != nullptr; ++env)
                {
                    char const * s(*env);
                    char const * n(s);
                    while(*s != '\0')
                    {
                        if(*s == '=')
                        {
                            std::string name(n, s - n);
                            // do not overwrite
                            if(src_envs.find(name) == src_envs.end())
                            {
                                // in Linux all is UTF-8 so we are already good here
                                src_envs[name] = s + 1;
                            }
                            break;
                        }
                        ++s;
                    }
                }
            }
            std::vector<char const *> envs_strings;
            for(environment_map_t::const_iterator it(src_envs.begin()); it != src_envs.end(); ++it)
            {
                envs_strings.push_back(strdup((it->first + "=" + it->second).c_str()));
            }
            envs_strings.push_back(nullptr);

            // replace the stdin and stdout with the pipes
            // TODO test result of the dup() command because if -1 things
            //      fail and we cannot really go on
            close(0);  // stdin
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
            if(dup(inout.f_pipes[0]) < 0)
            {
                throw snap_process_exception_initialization_failed("dup() of the stdin pipe failed");
            }
            close(1);  // stdout
            if(dup(inout.f_pipes[3]) < 0)
            {
                throw snap_process_exception_initialization_failed("dup() of the stdout pipe failed");
            }
#pragma GCC diagnostic pop
            // TODO should we redirect stderr somewhere else?

            inout.close();

            execvpe(
                f_command.toUtf8().data(),
                const_cast<char * const *>(&args_strings[0]),
                const_cast<char * const *>(&envs_strings[0])
            );
            // the child returns only if execvp() fails, which is possible
            SNAP_LOG_FATAL("Starting child process \"")(f_command.toUtf8().data())(" ")(f_arguments.join(" "))("\" failed.");
        }
        catch( snap_exception const & except )
        {
            SNAP_LOG_FATAL("process::run(): snap_exception caught: ")(except.what());
        }
        catch( std::exception const & std_except )
        {
            // the snap_logic_exception is not a snap_exception
            // and other libraries may generate other exceptions
            // (i.e. libtld, libQtCassandra...)
            SNAP_LOG_FATAL("process::run(): std::exception caught: ")(std_except.what());
        }
        catch( ... )
        {
            SNAP_LOG_FATAL("process::run(): unknown exception caught!");
        }
        exit(1);
        NOTREACHED();
        return -1;

    default:
        // parent
        {
            // close the sides we do not use here
            close(inout.f_pipes[0]);
            inout.f_pipes[0] = -1;
            close(inout.f_pipes[3]);
            inout.f_pipes[3] = -1;

            class in_t : public snap_thread::snap_runner
            {
            public:
                in_t(QByteArray const & input, int & pipe)
                    : snap_runner("process::in")
                    , f_input(input)
                    , f_pipe(pipe)
                {
                }

                virtual void run()
                {
                    // TODO: this is not handling the interactive case
                    //       in the interactive case, additional input
                    //       may be added as we receive new output
                    //
                    //       more or less, this means making the data buffer
                    //       a copy of any extra input before returning
                    QByteArray data(f_input);
                    if(write(f_pipe, data.data(), data.size()) != data.size())
                    {
                        // what do we do here? (i.e. we're in a thread)
                    }

                    // TODO:
                    // the only way to wake up the other side is to close
                    // once we are done writing data--this won't help in
                    // case we want some interactive support... (i.e. write
                    // in the pipe depending on what the output is)
                    close(f_pipe);
                    f_pipe = -1;
                }

                QByteArray const &  f_input;
                int &               f_pipe;
            } in(f_input, inout.f_pipes[1]);
            snap_thread in_thread("process::in::thread", &in);
            if(!in_thread.start())
            {
                return -1;
            }

            class out_t : public snap_thread::snap_runner
            {
            public:
                out_t(QByteArray & output)
                    : snap_runner("process::out")
                    , f_output(output)
                    //, f_pipe() -- auto-init
                    //, f_callback() -- auto-init
                    //, f_process() -- auto-init
                {
                }

                virtual void run()
                {
                    // TODO: we need to support the interactive capability
                    //       at some point, which means making the pipe a
                    //       non-blocking call which we wait on with a
                    //       select() and send the output to the callback
                    //       as soon as available instead of loading as
                    //       much as possible first (i.e. no buffering)
                    //       later we could have a line based handler which
                    //       calls the output_available() whenever a new
                    //       line of data, delimited by new line (\r or \n)
                    //       characters, is read (semi-buffering)
                    for(;;)
                    {
                        char buf[4096];
                        ssize_t l(read(f_pipe, buf, sizeof(buf)));
                        if(l <= 0)
                        {
                            //if(l < 0) ... manage error?
                            break;
                        }
                        QByteArray output(buf, static_cast<int>(l));
                        f_output.append(output);
                        if(f_callback)
                        {
                            f_callback->output_available(f_process, output);
                        }
                    }
                }

                QByteArray &                    f_output;
                int32_t                         f_pipe = -1;
                process_output_callback *       f_callback = nullptr;
                process *                       f_process = nullptr;
            } out(f_output);
            out.f_pipe = inout.f_pipes[2];
            out.f_callback = f_output_callback;
            out.f_process = this;
            snap_thread out_thread("process::out::thread", &out);
            if(!out_thread.start())
            {
                return -1;
            }

            // wait for the child process first
            int const r( child.wait() );
            //
            // then wait on the two threads
            in_thread.stop();
            out_thread.stop();

            return r;
        }
    }
}


/** \brief The input to be sent to stdin.
 *
 * Add the input data to be written to the stdin pipe. Note that the input
 * cannot be modified once the run() command was called unless the mode
 * is PROCESS_MODE_INOUT_INTERACTIVE.
 *
 * Note that in case the mode is interactive, calling this function adds
 * more data to the input. It does not erase what was added before.
 * The thread may eat some of the input in which case it gets removed
 * from the internal variable.
 *
 * \note
 * The function is safe and adding new input from the output thread
 * (which happens in interactive mode) is protected.
 *
 * \warning
 * Strings are converted to UTF-8 before getting sent to stdin. If another
 * convertion is required, make sure to use a QByteArray instead.
 *
 * \param[in] input  The input of the process (stdin).
 */
void process::set_input(QString const & input)
{
    // this is additive!
    f_input += input.toUtf8();
}


/** \brief Binary data to be sent to stdin.
 *
 * When the input data is binary, use the QByteArray instead of a QString
 * so you are sure it gets properly added.
 *
 * Calling this function multiple times appends the new data to the
 * existing data.
 *
 * Please, see the other set_input() function for additional information.
 *
 * \note
 * When sending a QString, remember that these are converted to UTF-8
 * which is not compatible with purely binary data (i.e. UTF-8, for example,
 * does not allow for 0xFE and 0xFF.)
 *
 * \param[in] input  The input of the process (stdin).
 */
void process::set_input(QByteArray const & input)
{
    // this is additive!
    f_input += input;
}


/** \brief Read the output of the command.
 *
 * This function reads the output of the process. This function converts
 * the output to UTF-8. Note that if some bytes are missing this function
 * is likely to fail. If you are reading the data little by little as it
 * comes in, you may want to use the get_binary_output() function
 * instead. That way you can detect characters such as the "\n" and at
 * that point convert the data from the previous "\n" you found in the
 * buffer to that new "\n". This will generate valid UTF-8 strings.
 *
 * This function is most often used by users of commands that process
 * one given input and generate one given output all at once.
 *
 * \param[in] reset  Whether the output so far should be cleared.
 *
 * \return The current output buffer.
 *
 * \sa get_binary_output()
 */
QString process::get_output(bool reset)
{
    QString const output(QString::fromUtf8(f_output));
    if(reset)
    {
        f_output.clear();
    }
    return output;
}


/** \brief Read the output of the command as a binary buffer.
 *
 * This function reads the output of the process in binary (untouched).
 *
 * This function does not fail like the get_output() which attempts to
 * convert the output of the function to UTF-8. Also the output of the
 * command may not be UTF-8 in which case you would have to use the
 * binary version and use a different conversion.
 *
 * \param[in] reset  Whether the output so far should be cleared.
 *
 * \return The current output buffer.
 *
 * \sa get_output()
 */
QByteArray process::get_binary_output(bool reset)
{
    QByteArray const output(f_output);
    if(reset)
    {
        f_output.clear();
    }
    return output;
}


/** \brief Setup a callback to receive the output as it comes in.
 *
 * This function is used to setup a callback. That callback is expected
 * to be called each time data arrives in our input pipe (i.e. stdout
 * or the output pipe of the child process.)
 *
 * \param[in] callback  The callback class that is called on output arrival.
 */
void process::set_output_callback(process_output_callback * callback)
{
    f_output_callback = callback;
}








/** \brief Initialize the proc_info object.
 *
 * This function saves the proc_t pointer and flags in this object.
 *
 * \exception snap_process_exception_data_not_available
 * If the pointer p is null, the function raises this exception.
 *
 * \param[in] p  The proc_t pointer as returned by readproc().
 * \param[in] flags  The set of flags from the PROCTAB used to read this data.
 */
process_list::proc_info::proc_info(std::shared_ptr<proc_t> p, int flags)
    : f_proc(p)
    , f_flags(flags)
{
    if(!f_proc)
    {
        throw snap_process_exception_data_not_available("process_list::proc_info::proc_info(): parameter p cannot be a null pointer");
    }
}


/** \brief Get the process identifier.
 *
 * This function retrieves the process identifier of this proc_info object.
 *
 * \return The process identifier.
 */
pid_t process_list::proc_info::get_pid() const
{
    // 't' stands for 'task' which is a process or a thread
    return static_cast<pid_t>(f_proc->tid);
}


/** \brief Get the parent process identifier.
 *
 * This function retrieves the parent process identifier of this
 * proc_info object.
 *
 * \return The parent process identifier.
 */
pid_t process_list::proc_info::get_ppid() const
{
    return static_cast<pid_t>(f_proc->ppid);
}


/** \brief Get the parent process identifier.
 *
 * This function retrieves the parent process identifier of this
 * proc_info object.
 *
 * \param[out] major  The major page fault since last update.
 * \param[out] minor  The minor page fault since last update.
 *
 * \return The parent process identifier.
 */
void process_list::proc_info::get_page_faults(unsigned long & major, unsigned long & minor) const
{
    major = f_proc->maj_delta;
    minor = f_proc->min_delta;
}


/** \brief Get the immediate percent of CPU usage for this process.
 *
 * This function retrieves the CPU usage as a percent of total CPU
 * available.
 *
 * \return The immediate CPU usage as a percent.
 */
unsigned process_list::proc_info::get_pcpu() const
{
    return f_proc->pcpu;
}


/** \brief Get the immediate process status.
 *
 * This function retrieves the CPU status of the process.
 *
 * The status is one of the following:
 *
 * \li D -- uninterruptible sleep (usually I/O)
 * \li R -- running or runnable
 * \li S -- Sleeping
 * \li T -- stopped by a job control signal or trace
 * \li W -- paging (should not occur)
 * \li X -- dead (should never appear)
 * \li Z -- defunct zombie process
 *
 * \return The immediate CPU usage as a percent.
 */
char process_list::proc_info::get_status() const
{
    return f_proc->state;
}


/** \brief Get the amount of time spent by this process.
 *
 * This function gives you information about the four variables
 * available cummulating the amount of time the process spent
 * running so far.
 *
 * \param[out] utime  The accumulated user time of this very task.
 * \param[out] stime  The accumulated kernel time of this very task.
 * \param[out] cutime  The accumulated user time of this task and
 *                     its children.
 * \param[out] cstime  The accumulated kernel time of this task
 *                     and its children.
 */
void process_list::proc_info::get_times(unsigned long long & utime,
                                         unsigned long long & stime,
                                         unsigned long long & cutime,
                                         unsigned long long & cstime) const
{
    utime = f_proc->utime;
    stime = f_proc->stime;
    cutime = f_proc->cutime;
    cstime = f_proc->cstime;
}


/** \brief Get the kernel priority of this process.
 *
 * This function returns the kernel priority of the process.
 *
 * \return The process kernel priority.
 */
long process_list::proc_info::get_priority() const
{
    return f_proc->priority;
}


/** \brief Get the unix nice of this process.
 *
 * This function returns the unix nice of the process.
 *
 * \return The process unix nice.
 */
long process_list::proc_info::get_nice() const
{
    return f_proc->nice;
}


/** \brief Get the size of this process.
 *
 * This function returns the total size of the process defined as
 * the virtual memory size.
 *
 * \return The process total virtual size.
 */
long process_list::proc_info::get_total_size() const
{
    return f_proc->size;
}


/** \brief Get the resident size of this process.
 *
 * This function returns the resident total size of the process.
 *
 * This size represents the amount of real memory currently used by
 * the process.
 *
 * \return The process resident memory size.
 */
long process_list::proc_info::get_resident_size() const
{
    return f_proc->resident;
}


/** \brief Get the process (command) name.
 *
 * This function return the name of the command. This includes the full
 * path.
 *
 * This field is available only if field_t::COMMAND_LINE was set.
 *
 * \return The process name.
 */
std::string process_list::proc_info::get_process_name() const
{
    if((f_flags & (PROC_FILLCOM | PROC_FILLARG)) == 0)
    {
        throw snap_process_exception_data_not_available("process_list::proc_info::get_process_name(): data not available");
    }

    if(f_proc->cmdline == nullptr)
    {
        return "";
    }

    return f_proc->cmdline[0];
}


/** \brief Get the number of arguments defined on the command line.
 *
 * This function counts the number of arguments, including all the many
 * empty arguments.
 *
 * Count will be positive or null. The count does not include the command
 * line (program name.)
 *
 * \return Count the number of arguments.
 */
int process_list::proc_info::get_args_size() const
{
    if(f_count == -1)
    {
        char ** s(f_proc->cmdline);
        if(s != nullptr)
        {
            while(*s != nullptr)
            {
                ++s;
            }
            f_count = static_cast<int32_t>(s - f_proc->cmdline - 1);
        }
        if(f_count < 0)
        {
            // it could be negative if we could not even get the command
            // line program name (in most cases: permission denied)
            f_count = 0;
        }
    }

    return f_count;
}


/** \brief Get the argument at the specified index.
 *
 * This function returns one of the arguments of the command line of
 * this process. Note that very often arguments are empty strings.
 *
 * \param[in] index  The index of the argument to retrieve.
 *
 * \return The specified argument.
 */
std::string process_list::proc_info::get_arg(int index) const
{
    if(static_cast<uint32_t>(index) >= static_cast<uint32_t>(f_count))
    {
        throw snap_process_exception_data_not_available(QString("process_list::proc_info::get_arg(): index %1 is larger than f_count %1").arg(index).arg(f_count));
    }

    return f_proc->cmdline[index + 1];
}


/** \brief Get the controlling terminal of this process.
 *
 * This function returns the full device number of the
 * controlling terminal.
 *
 * \return The number of the process controlling terminal.
 */
int process_list::proc_info::get_tty() const
{
    return f_proc->resident;
}


/** \brief Convert a field number to a process flag.
 *
 * This function converts a field number to a process flag.
 *
 * \li field_t::MEMORY -- get the various memory fields.
 * \li field_t::STATUS -- retrieve the current status of the process
 * such as 'R' for running and 'S' for sleeping.
 * \li field_t::STATISTICS -- read the variable statistics.
 * \li field_t::WAIT_CHANNEL -- kernel wait channel.
 * \li field_t::COMMAND_LINE -- command line with arguments.
 * \li field_t::ENVIRON -- environment at the time the process started.
 * \li field_t::USER_NAME -- user name owner of the process.
 * \li field_t::GROUP_NAME -- group name owner of the process.
 * \li field_t::CONTROL_GROUPS -- list of control groups.
 * \li field_t::SUPPLEMENTARY_GROUPS -- supplementary groups.
 * \li field_t::OUT_OF_MEMORY -- information about various OOM events.
 * \li field_t::NAMESPACE -- process namespace (to hide sub-pids.)
 *
 * \param[in] fld  The field number to convert.
 *
 * \return The PROC_... flag corresponding to the specified field.
 */
int process_list::field_to_flag(field_t fld) const
{
    switch(fld)
    {
    case field_t::MEMORY:
        return PROC_FILLMEM;

    case field_t::STATUS:
        return PROC_FILLSTATUS;

    case field_t::STATISTICS:
        return PROC_FILLSTAT;

    case field_t::WAIT_CHANNEL:
#ifdef PROC_FILLWCHAN
        // in version 6, this was removed
        return PROC_FILLWCHAN;
#else
        break;
#endif

    case field_t::COMMAND_LINE:
        return PROC_FILLCOM | PROC_FILLARG;

    case field_t::ENVIRON:
        return PROC_FILLENV;

    case field_t::USER_NAME:
        return PROC_FILLUSR;

    case field_t::GROUP_NAME:
        return PROC_FILLGRP;

    case field_t::CGROUP:
        return PROC_FILLCGROUP;

    case field_t::SUPPLEMENTARY_GROUP:
        return PROC_FILLSUPGRP;

    case field_t::OOM:
        return PROC_FILLOOM;

    case field_t::NAMESPACE:
        return PROC_FILLNS;

    }
    throw snap_process_exception_unknown_flag("process_list::field_to_flag(): invalid field number");
}


/** \brief Check whether a field was set or cleared.
 *
 * \param[in] fld  The field to check.
 *
 * \return true if the field was set, false otherwise.
 */
bool process_list::get_field(field_t fld) const
{
    return (f_flags & field_to_flag(fld)) != 0;
}


/** \brief Set a field.
 *
 * Set the flag so the specified field(s) get loaded on calls to the next()
 * function.
 *
 * This function must be called once per field group you are interested in.
 *
 * Note that each field corresponds to a file in the /proc file system.
 * It is smart to really only get those that you really need.
 *
 * \param[in] fld  The field to set.
 */
void process_list::set_field(field_t fld)
{
    if(f_proctab)
    {
        throw snap_process_exception_already_initialized("process_list::set_flag(): process flags cannot be set after next() was called");
    }
    f_flags |= field_to_flag(fld);
}


/** \brief Clear a field.
 *
 * Clear the flag so the specified field(s) do NOT get loaded on calls to
 * the next() function.
 *
 * This function can be used to reset fields that were previously set
 * with the set_field() function.
 *
 * \param[in] fld  The field to clear.
 */
void process_list::clear_field(field_t fld)
{
    if(f_proctab)
    {
        throw snap_process_exception_already_initialized("process_list::set_flag(): process flags cannot be reset after next() was called");
    }
    f_flags &= ~field_to_flag(fld);
}


/** \brief Reset the listing of processes.
 *
 * This function reset the list of processes by clearing the internal
 * pointer to the PROCTAB object.
 *
 * This can be called at any time.
 *
 * After a call to rewind() you may change the sets of flags with calls
 * to set_field() and clear_field().
 */
void process_list::rewind()
{
    f_proctab.reset();
}


/** \brief Read the next process.
 *
 * This function reads the information about the next process and
 * returns it in a shared pointer. The shared pointer can simply
 * be destroyed to release any memory allocated by the process_list
 * object.
 *
 * The object returned holds a pointer to the proc_t data
 * as read by the readproc() function. You can find this structure
 * and additional information in /usr/include/proc/readproc.h (assuming
 * you have the libprocps3-dev package installed.)
 *
 * \return A shared pointer to a proc_t structure.
 */
process_list::proc_info::pointer_t process_list::next()
{
    struct deleters
    {
        static void delete_proctab(PROCTAB * ptr)
        {
            if(ptr != nullptr)
            {
                closeproc(ptr);
            }
        }

        static void delete_proc(proc_t * ptr)
        {
            if(ptr != nullptr)
            {
                freeproc(ptr);
            }
        }
    };

    if(!f_proctab)
    {
        f_proctab = std::shared_ptr<PROCTAB>(openproc(f_flags, 0, 0), deleters::delete_proctab);
        if(!f_proctab)
        {
            throw snap_process_exception_openproc("process_list::next(): openproc() failed opening \"proc\", cannot read processes.");
        }
    }

    // I tested and if readproc() is called again after returning nullptr, it
    // continues to return nullptr so no need to do anything more
    //
    std::shared_ptr<proc_t> p(std::shared_ptr<proc_t>(readproc(f_proctab.get(), nullptr), deleters::delete_proc));
    if(p)
    {
        return proc_info::pointer_t(new proc_info(p, f_flags));
    }

    return proc_info::pointer_t();
}


} // namespace snap

// vim: ts=4 sw=4 et
