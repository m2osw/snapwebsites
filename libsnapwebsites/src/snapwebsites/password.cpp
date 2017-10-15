// Snap Websites Server -- handle creating / encrypting password
// Copyright (C) 2011-2017  Made to Order Software Corp.
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

// ourselves
//
#include <snapwebsites/hexadecimal_string.h>
#include "snapwebsites/not_used.h"
#include "snapwebsites/password.h"

// boost lib
//
#include <boost/static_assert.hpp>

// C++ lib
//
#include <memory>
#include <iostream>

// openssl lib
//
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

// C lib
//
#include <fcntl.h>
#include <termios.h>


namespace snap
{



namespace
{

/** \brief Size of the salt for a password.
 *
 * Whenever we encrypt a password, we use a corresponding salt.
 *
 * The salt is used to further encrypt the password so two users who
 * decided to use the exact same password will not be seen as having
 * the same password because of the salt since the salt renders any
 * password unique.
 *
 * \note
 * In the current implementation we do not in any way attempt to
 * make sure that each user gets a unique salt so it is always
 * possible for two users to end up with the exact same salt. However,
 * it is really very unlikely that those two users would also choose
 * the exact same password. Now, with a salt of 32 bytes, the real
 * likelihood for two people to end up with the same salt is really
 * very low (32 bytes is 256 bits, so one chance in 2 power 256, which
 * is a very small number, a little under 10 power -77.)
 *
 * \todo
 * We may want to offer the programmer a way to enter his own salt
 * size. Right now, this is fixed and cannot ever be changed since
 * the input of existing password will have a salt string of that
 * size exactly.
 */
int const SALT_SIZE = 32;
// to be worth something, the salt must be at least 6 bytes
BOOST_STATIC_ASSERT(SALT_SIZE >= 6);
// the salt size must be even
BOOST_STATIC_ASSERT((SALT_SIZE & 1) == 0);


/** \brief Delete an MD context.
 *
 * We allocate an EVP MD context in order to compute the hash according to
 * the digest specified by the programmer (or "sha512" by default.)
 *
 * The function using the \p mdctx may raise an exception on an error so
 * we save the context in a shared pointer which auto-deletes the context
 * once we are done with it by calling this very function.
 *
 * \note
 * The mdctx buffer is NOT allocated. It's created on the stack, but it
 * still needs cleanup and the digest may allocate buffers that need to
 * be released.
 *
 * \param[in] mdctx  The pointer to the MD context.
 */
void evp_md_ctx_deleter(EVP_MD_CTX * mdctx)
{
    // clean up the context
    // (note: the return value is not documented so we ignore it)
    EVP_MD_CTX_cleanup(mdctx);
}


/** \brief Close a file descriptor.
 *
 * This function will close the file descriptor pointer by fd.
 *
 * \param[in] fd  Pointer to the file descriptor to close.
 */
void close_file(int * fd)
{
    close(*fd);
}


}


/** \brief Initialize the password object.
 *
 * This function does nothing at this time. By default a password object
 * is empty.
 *
 * There are several ways the password object is used:
 *
 * \li To generate a new password automatically.
 *
 * \code
 *      password p;
 *      p.set_digest("sha512");   // always required in this case
 *      p.generate_password(10);  // necessary if you want to specify the size
 *      std::string hash(p.get_encrypted_password();
 *      std::string salt(p.get_salt());
 * \endcode
 *
 * The hash variable is the encypted password. Note that you will want to
 * also save the salt otherwise you won't be able to do anything with the
 * hash alone.
 *
 * \li To encrypt a password entered by a user.
 *
 * \code
 *      password p;
 *      p.set_digest("sha512");
 *      p.set_plain_password(user_entered_password);
 *      std::string hash(p.get_encrypted_password());
 *      std::string salt(p.get_salt());
 * \endcode
 *
 * \li To compare an already encrypted password against a password entered
 *     by a user.
 *
 * \code
 *      password p;
 *      p.set_digest(digest_of_existing_password);
 *      p.set_plain_password(user_entered_password, existing_password_salt);
 *      std::string hash(p.get_encrypted_password());
 *      if(hash == existing_password_hash) // ...got it right...
 * \endcode
 *
 * You may also defined two password objects and compare them against each
 * others to know whether the new login password is the same as the database
 * password:
 *
 * \code
 *      // or use two password objects:
 *      password p;
 *      p.set_digest(digest_of_existing_password);
 *      p.set_plain_password(user_entered_password, existing_password_salt);
 *      password op;  // old password
 *      op.set_encrypted_password();
 *      if(op == p) // ...got it right...
 * \endcode
 *
 * \warning
 * In the current implementation, the salt string must be exactly SALT_SIZE
 * in length. Although we use an std::string, the bytes can be any value
 * from '\0' to '\xFF'.
 */
password::password()
{
}


/** \brief Clean up a password object.
 *
 * This function cleans up the strings held by the password object.
 * That way they do not lay around in memory.
 */
password::~password()
{
    clear_string(f_plain_password);
    clear_string(f_encrypted_password);
    clear_string(f_salt);
}


/** \brief Define the OpenSSL function to use to encrypt the password.
 *
 * This function saves the digest to use to encrypt the password. Until
 * this is done, trying to retrieve an encrypted password from a plain
 * password will fail.
 *
 * For now, we use "sha512" as the default. We may also want to look
 * into using the bcrypt() function instead. However, Blowfish uses
 * only 64 bits and suffers from birthday attacks (guessing of words).
 *
 * \warning
 * This function has the side effect of clearing the cached encrypted
 * password.
 *
 * \todo
 * The test needs to verify that "sha512" exists so the default works.
 *
 * \exception password_exception_digest_not_available
 * If the digest is not defined in OpenSSL, then this exception is raised.
 *
 * \param[in] digest  The digest name.
 */
void password::set_digest(std::string const & digest)
{
    // Initialize so we gain access to all the necessary digests
    //
    OpenSSL_add_all_digests();

    // Make sure the digest actually exists
    //
    if(EVP_get_digestbyname(f_digest.c_str()) == nullptr)
    {
        throw password_exception_digest_not_available("the specified digest could not be found");
    }

    f_digest = digest;

    clear_string(f_encrypted_password);
}


/** \brief Retrieve the name of the current OpenSSL digest.
 *
 * This function returns the digest to use to encrypt the password.
 *
 * \return The name of the digest currently in use in this password.
 */
std::string const & password::get_digest() const
{
    return f_digest;
}


/** \brief Generate the password.
 *
 * In some cases an administrator may want to create an account for a user
 * which should then have a valid, albeit unknown, password.
 *
 * This function can be used to create that password.
 *
 * It is strongly advised to NOT send such passwords to the user via email
 * because they may contain "strange" characters and emails are notoriously
 * not safe.
 *
 * \note
 * The function verifies that the min_length parameter is at least 8. Note
 * that a safe password is more like 10 or more totally random characters.
 *
 * \note
 * The \p min_length parameter represent the minimum length, it is very
 * likely that the result will be longer.
 *
 * \warning
 * The function is not likely to generate a user friendly password. It is
 * expected to be used when a password is required but the user cannot
 * enter one and the user will have to run a Change Password procedure.
 *
 * \warning
 * Calling this functions also generates a new salt.
 *
 * \todo
 * Look into creating a genearte_human_password() with sets of characters
 * in each language instead of just basic ASCII and a length that makes
 * more sense (here the default is 64 because it is a "computer password").
 *
 * \todo
 * Also remove characters that can cause problems for users (i.e. spaces,
 * ampersand, look alike characters, etc.)
 *
 * \param[in] min_length  The minimum length of the password.
 */
void password::generate_password(int min_length)
{
    // restart from scratch
    //
    clear_string(f_plain_password);
    clear_string(f_encrypted_password);
    clear_string(f_salt);

    if(min_length < 8)
    {
        min_length = 8;
    }

    // a "large" set of random bytes
    //
    int const PASSWORD_SIZE(256);
    unsigned char buf[PASSWORD_SIZE];
    do
    {
        // get the random bytes
        //
        int const r(RAND_bytes(buf, sizeof(buf)));
        if(r != 1)
        {
            // something happened, RAND_bytes() failed!
            char err[256];
            ERR_error_string_n(ERR_peek_last_error(), err, sizeof(err));
            throw password_exception_function_failure(
                QString("RAND_bytes() error, it could not properly fill the salt buffer (%1: %2)")
                        .arg(ERR_peek_last_error())
                        .arg(err));
        }

        for(int i(0); i < PASSWORD_SIZE; ++i)
        {
            // only but all ASCII characters are accepted for now
            //
            if(buf[i] >= ' ' && buf[i] < 0x7F)
            {
                f_plain_password += static_cast<char>(buf[i]);
            }
            //else -- skip any other character
        }
    }
    while(f_plain_password.length() < static_cast<size_t>(min_length));
    // make sure it is long enough
}


/** \brief Define the password from a plain password.
 *
 * This function defines a password starting from a plain password.
 *
 * If this password comes from a log in screen, then you will need to
 * specify the existing salt. Otherwise, leave the salt string empty.
 * The password object will randomly generate a buffer of bytes
 * automatically for it.
 *
 * \note
 * Calling this function resets the encrypted password.
 *
 * \note
 * Although it is expected that the password is a valid C string,
 * this object does not check such. The password can include any
 * character, including '\0', and it can even be invalid UTF-8.
 * It is the caller's responsibility to verify the string if it
 * can be tainted in any special way.
 *
 * \param[in] plain_password  The plain password to encrypt.
 * \param[in] salt  The salt to use with the password encryption system.
 */
void password::set_plain_password(std::string const & plain_password, std::string const & salt)
{
    // the salt must be of the right length (or unspecified)
    //
    if(!salt.empty()
    && salt.length() != SALT_SIZE)
    {
        throw password_exception_invalid_parameter(QString("if defined, the salt must be exactly %1 bytes").arg(SALT_SIZE));
    }

    // that means the encrypted password is not going to be valid either
    //
    clear_string(f_plain_password);
    clear_string(f_encrypted_password);
    clear_string(f_salt);

    f_plain_password = plain_password;
    f_salt = salt;
}


/** \brief Ask the user to enter a password in his console.
 *
 * This function opens the process TTY ("/dev/tty") and reads a password.
 *
 * The function is responsible for cancel ECHO-ing in the console before
 * getting characters.
 *
 * This function accepts a \p salt parameter like the set_plain_password(),
 * it may be used to check the password of an existing user and not just to
 * create a new user entry so the salt is required.
 *
 * \note
 * The existing password information is cleared on entry. It is set to the
 * new password the user enters only if a valid password is entered. The
 * \p salt parameter is also used only if the new password is considered
 * valid.
 *
 * \todo
 * Add a minimum size for the password.
 *
 * \param[in] salt  The salt to encrypt the password.
 *
 * \return true if the password was properly entered, false otherwise.
 */
bool password::get_password_from_console(std::string const & salt)
{
    // read the new f_plain_password from the console
    //
    clear_string(f_plain_password);
    clear_string(f_encrypted_password);
    clear_string(f_salt);

    // the process must have a terminal
    //
    if(!isatty(STDIN_FILENO))
    {
        std::cerr << "snappassword:error: input terminal is not a TTY, cancel add with a --password option but no password." << std::endl;
        return 1;
    }

    // open process terminal
    //
    int tty(open("/dev/tty", O_RDONLY));
    if(tty == -1)
    {
        std::cerr << "snappassword:error: could not access process tty." << std::endl;
        return 1;
    }
    std::unique_ptr<int, decltype(&close_file)> raii_tty(&tty, close_file);

    // get current termios flags
    //
    struct safe_termios
    {
        safe_termios(int tty)
            : f_tty(tty)
        {
            // save the original termios flags
            //
            if(tcgetattr(f_tty, &f_original) != 0)
            {
                return;
            }

            // setup termios to not echo input characters
            // and return characters one by one (avoid buffering)
            //
            // TODO: tcsetattr() returns 0 on success of any attribute changes
            //       meaning that we should call it once per change!
            //
            struct termios t(f_original);
            t.c_lflag &= ~(ICANON | ECHO);
            t.c_cc[VMIN] = 1;
            t.c_cc[VTIME] = 0;
            f_valid = tcsetattr(f_tty, TCSAFLUSH, &t) == 0;
        }

        ~safe_termios()
        {
            // restore the termios flags
            // ignore failures... it is likely to work since we did not
            // change the original data, but who knows.
            //
            NOTUSED(tcsetattr(f_tty, TCSAFLUSH, &f_original));
        }

        bool is_valid() const
        {
            return f_valid;
        }

    private:
        bool f_valid = false;
        int f_tty;
        struct termios f_original;
    };

    safe_termios st(tty);
    if(!st.is_valid())
    {
        std::cerr << "password:error: could not change terminal attributes to make it safe to read a password." << std::endl;
        return false;
    }

    std::string new_password;

    std::cout << "Password: " << std::flush;
    for(;;)
    {
        char c;
        if(read(tty, &c, 1) != 1)
        {
            std::cout << std::endl << std::flush;
            std::cerr << "snappassword:error: I/O error while reading from TTY." << std::endl;
            return false;
        }
        switch(c)
        {
        case '\b': // backspace
            if(!new_password.empty())
            {
                // replace that last character with '.;, just in case
                // then forget about it
                //
                new_password[new_password.length() - 1] = '.';
                new_password.resize(new_password.length() - 1);
            }
            break;

        case '\n': // enter
            std::cout << std::endl << std::flush;
            if(new_password.empty())
            {
                // we could allow empty passwords at some point?
                //
                std::cerr << "snappassword:error: password cannot be empty." << std::endl;
                return false;
            }
            f_plain_password = new_password;
            f_salt = salt;
            clear_string(new_password);
            return true;

        default:
            if(c >= ' ')
            {
                new_password += c;
            }
            break;

        }
    }
}


/** \brief Retrieve the plain password.
 *
 * This function returns a copy of the plain password.
 *
 * Note that the plain password is not available if the password object
 * was just set to an encrypted password (i.e. the "encryption" is a one
 * way hashing so we cannot get the password back out.) So you can get
 * the pain password only if the \p set_plain_password() was called earlier.
 *
 * \return The plain password.
 */
std::string const & password::get_plain_password() const
{
    return f_plain_password;
}


/** \brief Retrieve the salt of this password.
 *
 * When generating or encrypting a new password, the password object
 * also generates a new salt value. This salt has to be saved along
 * the encrypted password in order to be able to re-encrypt the same
 * password to the same value.
 *
 * \note
 * There is no set_salt() function. Instead, we expect you will call
 * the set_plain_password() including the salt parameter.
 *
 * \warning
 * The salt is not a printable string. It is a buffer of binary codes,
 * which may include '\0' bytes at any location. You must make sure to
 * use the length() function to know the exact size of the salt.
 *
 * \return The current salt key or an empty string if not defined.
 */
std::string const & password::get_salt() const
{
    return f_salt;
}


/** \brief Define the encrypted password.
 *
 * You may use this function to define the password object as an encrypted
 * password. This is used to one can compare two password for equality.
 *
 * This function let you set the salt. This is generally used when reading
 * the password from a file or a database. That way it can be read with
 * the get_salt() function and used with the plain password to encrypt it.
 *
 * \param[in] encrypted_password  The already encrypted password.
 * \param[in] salt  Set the password salt.
 */
void password::set_encrypted_password(std::string const & encrypted_password, std::string const & salt)
{
    // clear the previous data
    //
    clear_string(f_plain_password);
    clear_string(f_encrypted_password);
    clear_string(f_salt);

    f_encrypted_password = encrypted_password;
    f_salt = salt;
}


/** \brief Retrieve a copy of the encrypted password.
 *
 * In most cases this function is used to retrieve the resulting encrypted
 * password and then save it in a database.
 *
 * \note
 * The function caches the encrypted password so calling this function
 * multiple times is considered fast. However, if you change various
 * parameters, it is expected to recompute the new corresponding value.
 *
 * \return The encrypted password.
 */
std::string const & password::get_encrypted_password() const
{
    if(f_encrypted_password.empty())
    {
        // the encrypt password changes f_encrypted_password and
        // if required generates the password and salt strings
        //
        const_cast<password *>(this)->encrypt_password();
    }

    return f_encrypted_password;
}


/** \brief Check whether the encrypted passwords are equal.
 *
 * This function calls the get_encrypted_password() and compares
 * the results against each others. If the encrypted passwords
 * of both objects are the same, then it returns true.
 *
 * \param[in] rhs  The right hand side password to compare with this.
 *
 * \return true if the encrypted passwords are the same.
 */
bool password::operator == (password const & rhs) const
{
    return get_encrypted_password() == rhs.get_encrypted_password();
}


/** \brief Check whether this passwords is considered smaller.
 *
 * This function calls the get_encrypted_password() and compares
 * the results against each others. If this encrypted password
 * is considered smaller, then the function returns true.
 *
 * \param[in] rhs  The right hand side password to compare with this.
 *
 * \return true if this encrypted password is the smallest.
 */
bool password::operator < (password const & rhs) const
{
    return get_encrypted_password() < rhs.get_encrypted_password();
}


/** \brief Generate a new salt for a password.
 *
 * Every time you get to encrypt a new password, call this function to
 * get a new salt. This is important to avoid having the same hash for
 * the same password for multiple users.
 *
 * Imagine a user creating 3 accounts and each time using the exact same
 * password. Just using an md5sum it would encrypt that password to
 * exactly the same 16 bytes. In other words, if you crack one, you
 * crack all 3 (assuming you have access to the database you can
 * immediately see that all those accounts have the exact same password.)
 *
 * The salt prevents such problems. Plus we add 256 bits of completely
 * random entropy to the digest used to encrypt the passwords. This
 * in itself makes it for a much harder to decrypt hash.
 *
 * The salt is expected to be saved in the database along the password.
 */
void password::generate_password_salt()
{
    clear_string(f_salt);

    // we use 16 bytes before and 16 bytes after the password
    // so create a salt of SALT_SIZE bytes (256 bits at time of writing)
    //
    unsigned char buf[SALT_SIZE];
    int const r(RAND_bytes(buf, sizeof(buf)));
    if(r != 1)
    {
        // something happened, RAND_bytes() failed!
        //
        char err[256];
        ERR_error_string_n(ERR_peek_last_error(), err, sizeof(err));
        throw password_exception_function_failure(
            QString("RAND_bytes() error, it could not properly fill the salt buffer (%1: %2)")
                    .arg(ERR_peek_last_error())
                    .arg(err));
    }

    f_salt = std::string(reinterpret_cast<char *>(buf), sizeof(buf));
}


/** \brief Encrypt a password.
 *
 * This function generates a strong hash of a user password to prevent
 * easy brute force "decryption" of the password. (i.e. an MD5 can be
 * decrypted in 6 hours, and a SHA1 password, in about 1 day, with a
 * $100 GPU as of 2012.)
 *
 * Here we use 2 random salts (using RAND_bytes() which is expected to
 * be random enough for encryption like algorithms) and the specified
 * digest to encrypt (okay, hash--a one way "encryption") the password.
 *
 * Read more about hash functions on
 * http://ehash.iaik.tugraz.at/wiki/The_Hash_Function_Zoo
 *
 * \exception users_exception_size_mismatch
 * This exception is raised if the salt byte array is not exactly SALT_SIZE
 * bytes. For new passwords, you want to call the create_password_salt()
 * function to create the salt buffer.
 *
 * \exception users_exception_digest_not_available
 * This exception is raised if any of the OpenSSL digest functions fail.
 * This include an invalid digest name and adding/retrieving data to/from
 * the digest.
 *
 * \param[in] digest  The name of the digest to use (i.e. "sha512").
 * \param[in] password  The password to encrypt.
 * \param[in] salt  The salt information, necessary to encrypt passwords.
 */
void password::encrypt_password()
{
    // make sure we reset by default, if it fails, we get an empty string
    //
    clear_string(f_encrypted_password);

    if(f_plain_password.empty())
    {
        generate_password();
    }

    if(f_salt.empty())
    {
        generate_password_salt();
    }

    // Initialize so we gain access to all the necessary digests
    OpenSSL_add_all_digests();

    // retrieve the digest we want to use
    // (TODO: allows website owners to change this value)
    EVP_MD const * md(EVP_get_digestbyname(f_digest.c_str()));
    if(md == nullptr)
    {
        throw password_exception_digest_not_available("the specified digest could not be found");
    }

    // initialize the digest context
    EVP_MD_CTX mdctx;
    EVP_MD_CTX_init(&mdctx);
    if(EVP_DigestInit_ex(&mdctx, md, nullptr) != 1)
    {
        throw password_exception_encryption_failed("EVP_DigestInit_ex() failed digest initialization");
    }

    // RAII cleanup
    //
    std::unique_ptr<EVP_MD_CTX, decltype(&evp_md_ctx_deleter)> raii_mdctx(&mdctx, evp_md_ctx_deleter);

    // add first salt
    //
    if(EVP_DigestUpdate(&mdctx, f_salt.c_str(), SALT_SIZE / 2) != 1)
    {
        throw password_exception_encryption_failed("EVP_DigestUpdate() failed digest update (salt1)");
    }

    // add password
    //
    if(EVP_DigestUpdate(&mdctx, f_plain_password.c_str(), f_plain_password.length()) != 1)
    {
        throw password_exception_encryption_failed("EVP_DigestUpdate() failed digest update (password)");
    }

    // add second salt
    //
    if(EVP_DigestUpdate(&mdctx, f_salt.c_str() + SALT_SIZE / 2, SALT_SIZE / 2) != 1)
    {
        throw password_exception_encryption_failed("EVP_DigestUpdate() failed digest update (salt2)");
    }

    // retrieve the result of the hash
    //
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len(EVP_MAX_MD_SIZE);
    if(EVP_DigestFinal_ex(&mdctx, md_value, &md_len) != 1)
    {
        throw password_exception_encryption_failed("EVP_DigestFinal_ex() digest finalization failed");
    }
    f_encrypted_password.append(reinterpret_cast<char *>(md_value), md_len);
}


/** \brief Clear a string so password do not stay in memory if possible.
 *
 * This function is used to clear the memory used by passwords. This
 * is a useful security trick, although really with encrypted passwords
 * in the Cassandra database, we will have passwords laying around anyway.
 *
 * \todo
 * See whether we could instead extend the std::string class? That way
 * we have that in the destructor and we can reuse it all over the place
 * where we load a password.
 *
 * \param[in,out] str  The string to clear.
 */
void password::clear_string(std::string & str)
{
    std::for_each(
              str.begin()
            , str.end()
            , [](auto & c)
            {
                c = 0;
            });
    str.clear();
}




/** \brief Handle a password file.
 *
 * This constructor creates an object that knows how to handle a password
 * file.
 *
 * We only support our own format as follow:
 *
 * \li we support 4 fields (4 columns)
 * \li the fields are separated by colons
 * \li the first field is the user name
 * \li the second field is the digest used to hash the password
 * \li the third field is the password salt written in hexadecimal
 * \li the forth field is the password itself
 * \li lines are separated by '\\n'
 *
 * IMPORTANT NOTE: the password may include the ':' character.
 *
 * \warning
 * The password file will be loaded once and cached. If you are running
 * an application which sits around for a long time and other applications
 * may modify the password file, you want to use this class only
 * temporarilly (i.e. use it on your stack, make the necessary find/save
 * calls, then lose it.)
 *
 * \param[in] filename  The path and name of the password file.
 */
password_file::password_file(std::string const & filename)
    : f_passwords(filename)
{
}


/** \brief Clean up the file.
 *
 * This function makes sure to clean up the file.
 */
password_file::~password_file()
{
    // TODO: we need to be able to clear the file_content buffer
}


/** \brief Search for the specified user in this password file.
 *
 * This function scans the password file for the specified user
 * name (i.e. a line that starts with "name + ':'".)
 *
 * \exception password_exception_invalid_parameter
 * If the \p name parameter is an empty string, then this exception is raised.
 *
 * \param[in] name  The name of the user to search.
 * \param[out] password  The password if found.
 *
 * \return true if the password was found in the file.
 */
bool password_file::find(std::string const & name, password & p)
{
    if(name.empty())
    {
        throw password_exception_invalid_parameter("the password_file::find() function cannot be called with an empty string in 'name'");
    }

    // read the while file at once
    //
    if(!load_passwords())
    {
        return false;
    }

    // search the user
    //
    std::string const & passwords(f_passwords.get_content());
    std::string::size_type const user_pos(passwords.find(name + ":"));

    // did we find it?
    //
    // Note: if npos, obviously we did not find it at all
    //       if not npos, the character before must be a '\n', unless pos == 0
    //
    if(user_pos == std::string::npos
    || (user_pos != 0 && passwords[user_pos - 1] != '\n'))
    {
        return false;
    }

    // get the end of the line
    //
    // we use the end of line as the boundary of future searches
    //
    std::string::size_type const digest_position(user_pos + name.length() + 1);
    std::string::size_type const end_position(passwords.find("\n", digest_position));
    if(end_position == std::string::npos)
    {
        return false;
    }

    // search for the second ":"
    //
    std::string::size_type const salt_position(passwords.find(":", digest_position));
    if(salt_position == std::string::npos
    || digest_position == salt_position
    || salt_position >= end_position)
    {
        // either we did not find the next ":"
        // or the digest is an empty string, which is not considered valid
        //
        return false;
    }

    // search for the third ":"
    //
    std::string::size_type const password_position(passwords.find(":", salt_position + 1));
    if(password_position == std::string::npos
    || salt_position + 1 == password_position
    || password_position + 1 >= end_position)
    {
        // either we did not find the next ":"
        // or the salt is an empty string
        // or the password is empty
        //
        return false;
    }

    char const * ptr(passwords.c_str());

    // setup the digest
    //
    std::string digest(ptr + digest_position, salt_position - digest_position);
    p.set_digest(digest);
    password::clear_string(digest);

    // setup the encrypted password and salt
    //
    std::string password_hex_salt(ptr + salt_position + 1, password_position - salt_position - 1);
    std::string password_salt(hex_to_bin(password_hex_salt));
    std::string encrypted_hex_password(ptr + password_position + 1, end_position - password_position - 1);
    std::string encrypted_password(hex_to_bin(encrypted_hex_password));
    p.set_encrypted_password(encrypted_password, password_salt);
    password::clear_string(password_hex_salt);
    password::clear_string(password_salt);
    password::clear_string(encrypted_password);

    // done with success
    //
    return true;
}


/** \brief Save a password in this password_file.
 *
 * This function saves the specified password for the named user in
 * this password_file. This function updates the content of the
 * file so a future find() will find the new information as expected.
 * However, if another application can make changes to the file, those
 * will not be caught.
 *
 * If the named user already has a password defined in this file, then
 * it gets replaced. Otherwise the new entry is added at the end.
 *
 * \warning
 * This function has the side effect of calling rewind() so the next
 * time you call the next() function, you will get the first user
 * again.
 *
 * \param[in] name  The name of the user.
 * \param[in] p  The password to save in this file.
 */
bool password_file::save(std::string const & name, password const & p)
{
    if(name.empty())
    {
        throw password_exception_invalid_parameter("the password_file::save() function cannot be called with an empty string in 'name'");
    }

    // read the while file at once
    //
    if(!load_passwords())
    {
        // ... we are about to create the file if it does not exist yet ...
    }

    std::string const new_line(
              name
            + ":"
            + p.get_digest()
            + ":"
            + bin_to_hex(p.get_salt())
            + ":"
            + bin_to_hex(p.get_encrypted_password())
            + "\n");

    // search the user
    //
    std::string const & passwords(f_passwords.get_content());
    std::string::size_type const user_pos(passwords.find(name + ":"));

    std::string new_content;

    // did we find it?
    //
    // Note: if npos, obviously we did not find it at all
    //       if not npos, the character before must be a '\n', unless pos == 0
    //
    if(user_pos == std::string::npos
    || (user_pos != 0 && passwords[user_pos - 1] != '\n'))
    {
        // not found, append at the end
        //
        new_content = passwords + new_line;
    }
    else
    {
        // get the end of the line
        //
        // we will have 3 parts:
        //
        //    . what comes before 'user_pos'
        //    . the line defining that user password
        //    . what comas after the 'user_pos'
        //
        std::string::size_type const digest_position(user_pos + name.length() + 1);
        std::string::size_type const end(passwords.find("\n", digest_position));
        if(end == std::string::npos)
        {
            return false;
        }

        char const * s(passwords.c_str());
        std::string before(s, user_pos);
        std::string after(s + end + 1, passwords.length() - end - 1);
        // XXX: in regard to security, the + operator creates temporary buffers
        //      (i.e. we would need to allocate our own buffer and copy there.)
        new_content = before
                    + new_line
                    + after;
        password::clear_string(before);
        password::clear_string(after);
    }

    // we are about to change the file so the f_next pointer is not unlikely
    // to be invalidated, so we rewind it
    //
    rewind();

    // save the new content in the file_content object
    //
    f_passwords.set_content(new_content);

    password::clear_string(new_content);

    // write the new file to disk
    //
    f_passwords.write_all();

    // done with success
    //
    return true;
}


/** \brief Delete a user and his password from password_file.
 *
 * This function searches for the specified user, if found, then it gets
 * removed from the password file. If that user is not defined in the
 * password file, nothing happens.
 *
 * \warning
 * This function has the side effect of calling rewind() so the next
 * time you call the next() function, you will get the first user
 * again.
 *
 * \param[in] name  The name of the user.
 * \param[in] p  The password to save in this file.
 */
bool password_file::remove(std::string const & name)
{
    if(name.empty())
    {
        throw password_exception_invalid_parameter("the password_file::delete_user() function cannot be called with an empty string in 'name'");
    }

    // read the while file at once
    //
    if(!load_passwords())
    {
        return false;
    }

    // search the user
    //
    std::string const & passwords(f_passwords.get_content());
    std::string::size_type const user_pos(passwords.find(name + ":"));

    // did we find it?
    //
    // Note: if npos, obviously we did not find it at all
    //       if not npos, the character before must be a '\n', unless pos == 0
    //
    if(user_pos == std::string::npos
    || (user_pos != 0 && passwords[user_pos - 1] != '\n'))
    {
        // not found, done
        //
        return true;
    }

    // get the end of the line
    //
    // we will have 3 parts:
    //
    //    . what comes before 'user_pos'
    //    . the line defining that user password
    //    . what comas after the 'user_pos'
    //
    std::string::size_type const digest_position(user_pos + name.length() + 1);
    std::string::size_type const end(passwords.find("\n", digest_position));
    if(end == std::string::npos)
    {
        return false;
    }

    char const * s(passwords.c_str());
    std::string before(s, user_pos);
    std::string after(s + end + 1, passwords.length() - end - 1);
    // XXX: in regard to security, the + operator creates temporary buffers
    //      (i.e. we would need to allocate our own buffer and copy there.)
    std::string new_content(before + after);
    password::clear_string(before);
    password::clear_string(after);

    // we are about to change the file so the f_next pointer is not unlikely
    // to be invalidated, so we rewind it
    //
    rewind();

    // save the new content in the file_content object
    //
    f_passwords.set_content(new_content);

    password::clear_string(new_content);

    // write the new file to disk
    //
    f_passwords.write_all();

    // done with success
    //
    return true;
}


/** \brief Read the next entry.
 *
 * This function can be used to read all the users one by one.
 *
 * The function returns the name of the user, which cannot be defined in
 * the password object. Once the end of the file is reached, the function
 * returns an empty string and does not modify \p p.
 *
 * \note
 * The function may hit invalid input data, in which case it will return
 * an empty string as if the end of the file was reached.
 *
 * \param[in,out] p  The password object where data gets saved.
 *
 * \return The username or an empty string once the end of the file is reached.
 */
std::string password_file::next(password & p)
{
    if(!load_passwords())
    {
        return std::string();
    }

    // get the end of the line
    //
    std::string const & passwords(f_passwords.get_content());
    std::string::size_type const next_user_pos(passwords.find("\n", f_next));
    if(next_user_pos == std::string::npos)
    {
        return std::string();
    }

    // retrieve the position of the end of the user name
    //
    std::string::size_type const end_name_pos(passwords.find(":", f_next, next_user_pos - f_next));
    if(end_name_pos == std::string::npos)
    {
        return std::string();
    }

    if(f_next == end_name_pos)
    {
        return std::string();
    }

    // the find() function does all the parsing of the elements, use it
    // instead of rewriting that code here (although we search for the
    // user again... we could have a sub-function to avoid the double
    // search!)
    //
    std::string const username(passwords.c_str(), end_name_pos - f_next);
    find(username, p);

    // the next user will be found on the next line
    //
    f_next = next_user_pos + 1;

    return username;
}


/** \brief Reset the next pointer to the start of the file.
 *
 * This function allows you to restart the next() function to the beginning
 * of the file.
 */
void password_file::rewind()
{
    f_next = 0;
}


/** \brief Load the password file once.
 *
 * This function loads the password file. It makes sure to not re-load
 * it if it was already loaded.
 */
bool password_file::load_passwords()
{
    if(!f_file_loaded)
    {
        if(!f_passwords.read_all())
        {
            return false;
        }
        f_file_loaded = true;
    }

    return true;
}



} // namespace snap

// vim: ts=4 sw=4 et
