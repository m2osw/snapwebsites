/**
 * Author/Copyright: Jorg Preiss
 *
 * @license MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <QFile>
#include <sys/file.h>

class QLockFile: public QFile
{
public:
    /** \brief Initialize the locked file.
     *
     * This function initializes a default locked file.
     */
    QLockFile()
        : QFile()
    {
    }

    /** \brief Initialize the locked file with a name.
     *
     * This function initializes a locked file with a filename.
     *
     * \param[in] name  The name of the file to open and lock.
     */
    QLockFile(QString const & name)
        : QFile(name)
    {
    }

    /** \brief Open the locked file
     *
     * Open a file and lock it in share mode (if iomode is read) or
     * exclusively (any other open mode.)
     *
     * The function blocks until the file is locked.
     *
     * When the file is closed the lock will automatically be released.
     *
     * \param[in] iomode  The I/O mode to use on the file.
     *
     * \return true if the open() succeeds, false otherwise
     */
    virtual bool open(OpenMode iomode)
    {
        if(!QFile::open(iomode))
		{
            return false;
        }
        // we want to ignore the text and unbuffered flags
        OpenMode m(iomode & ~(QIODevice::Text | QIODevice::Unbuffered));
        int op(m == QIODevice::ReadOnly ? LOCK_SH : LOCK_EX);
        // note: on close() the flock() is automatically released
        if(flock(handle(), op) != 0)
		{
            QFile::close();
            return false;
        }
        // this file is now open with a lock
        return true;
    }
};

// vim: ts=4 sw=4
