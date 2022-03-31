// Snap Websites Servers -- prepare a sendmail output stream
// Copyright (c) 2016-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
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

// self
//
#include "email.h"

// snapwebsites lib
//
#include "snap_magic.h"
#include "snap_pipe.h"
#include "quoted_printable.h"
#include "snapwebsites.h"

// eventdispatcher lib
//
#include    <cppprocess/process.h>


// snaplogger lib
//
#include    <snaplogger/message.h>


// libQtSerialization lib
//
#include <QtSerialization/QSerializationComposite.h>
#include <QtSerialization/QSerializationFieldBasicTypes.h>
#include <QtSerialization/QSerializationFieldString.h>

// libtld lib
//
#include <libtld/tld.h>

// Qt lib
//
#include <QFileInfo>




// last include
//
#include <snapdev/poison.h>


namespace snap
{

namespace
{

/** \brief Copy the filename if defined.
 *
 * Check whether the filename is defined in the Content-Disposition
 * or the Content-Type fields and make sure to duplicate it in
 * both fields. This ensures that most email systems have access
 * to the filename.
 *
 * \note
 * The valid location of the filename is the Content-Disposition,
 * but it has been saved in the 'name' sub-field of the Content-Type
 * field and some tools only check that field.
 *
 * \param[in,out] attachment_headers  The headers to be checked for
 *                                    a filename.
 */
void copy_filename_to_content_type(email::header_map_t & attachment_headers)
{
    if(attachment_headers.find(get_name(name_t::SNAP_NAME_CORE_CONTENT_DISPOSITION)) != attachment_headers.end()
    && attachment_headers.find(get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)) != attachment_headers.end())
    {
        // both fields are defined, copy the filename as required
        QString content_disposition(attachment_headers[get_name(name_t::SNAP_NAME_CORE_CONTENT_DISPOSITION)]);
        QString content_type(attachment_headers[get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)]);

        http_strings::WeightedHttpString content_disposition_subfields(content_disposition);
        http_strings::WeightedHttpString content_type_subfields(content_type);

        http_strings::WeightedHttpString::part_t::vector_t & content_disposition_parts(content_disposition_subfields.get_parts());
        http_strings::WeightedHttpString::part_t::vector_t & content_type_parts(content_type_subfields.get_parts());

        if(content_disposition_parts.size() > 0
        && content_type_parts.size() > 0)
        {
            // we only use part 1 (there should not be more than one though)
            //
            QString const filename(content_disposition_parts[0].get_parameter("filename"));
            if(!filename.isEmpty())
            {
                // okay, we found the filename in the Content-Disposition,
                // copy that to the Content-Type
                //
                // Note: we always force the name parameter so if it was
                //       already defined, we make sure it is the same as
                //       in the Content-Disposition field
                //
                content_type_parts[0].add_parameter("name", filename);
                attachment_headers[get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)] = content_type_subfields.to_string();
            }
            else
            {
                QString const name(content_type_parts[0].get_parameter("name"));
                if(!name.isEmpty())
                {
                    // Somehow the filename is defined in the Content-Type field
                    // but not in the Content-Disposition...
                    //
                    // copy it to the Content-Disposition too (where it should be)
                    //
                    content_disposition_parts[0].add_parameter("filename", name);
                    attachment_headers[get_name(name_t::SNAP_NAME_CORE_CONTENT_DISPOSITION)] = content_disposition_subfields.to_string();
                }
            }
        }
    }
}


}
// no name namespace




//////////////////////
// EMAIL ATTACHMENT //
//////////////////////


/** \brief Initialize an email attachment object.
 *
 * You can create an email attachment object, initializes it, and then
 * add it to an email object. The number of attachments is not limited
 * although you should remember that most mail servers limit the total
 * size of an email. It may be 5, 10 or 20Mb, but if you go over, the
 * email will fail.
 */
email::attachment::attachment()
{
}


/** \brief Clean up an email attachment.
 *
 * This function is here primarily to have a clean virtual table.
 */
email::attachment::~attachment()
{
}


/** \brief The content of the binary file to attach to this email.
 *
 * This function is used to attach one binary file to the email.
 *
 * If you know the MIME type of the data, it is smart to define it when
 * calling this function so that way you avoid asking the magic library
 * for it. This will save time as the magic library is much slower and
 * if you are positive about the type, it will be correct whereas the
 * magic library could return an invalid value.
 *
 * Also, if this is a file attachment, make sure to add a
 * `Content-Disposition` header to define the filename and
 * modification date as in:
 *
 * \code
 *   Content-Disposition: attachment; filename=my-attachment.pdf;
 *     modification-date="Tue, 29 Sep 2015 16:12:15 -0800";
 * \endcode
 *
 * See the set_content_disposition() function to easily add this
 * field.
 *
 * \note
 * The mime_type can be set to the empty string (`QString()`) to let
 * the system generate the MIME type automatically using the
 * get_mime_type() function.
 *
 * \param[in] data  The data to attach to this email.
 * \param[in] mime_type  The MIME type of the data if known,
 *                       otherwise leave empty.
 *
 * \sa add_header()
 * \sa get_mime_type()
 * \sa set_content_disposition()
 */
void email::attachment::set_data(QByteArray const & data, QString mime_type)
{
    f_data = data;

    // if user did not define the MIME type then ask the magic library
    if(mime_type.isEmpty())
    {
        mime_type = get_mime_type(f_data);
    }
    f_headers[get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)] = mime_type;
}


/** \brief Set the email attachment using quoted printable encoding.
 *
 * In most cases, when you attach something else than just text, you want
 * to encode the data. Even text, if you do not control the length of each
 * line properly, it is likely to get cut at some random length and could
 * end up looking wrong.
 *
 * This function encodes the data using the quoted_printable::encode()
 * function and marks the data encoded in such a way.
 *
 * By default, all you have to do is pass a QByteArray and the rest works
 * on its own, although it is usually a good idea to specify the MIME type
 * if you knowit.
 *
 * The flags parameter can be used to tweak the encoding functionality.
 * The default works with most data, although it does not include the
 * binary flag. See the quoted_printable::encode() function for additional
 * information about these flags.
 *
 * \param[in] data  The data of this a attachment.
 * \param[in] mime_type  The MIME type of the data, if left empty, it will
 *            be determined on the fly.
 * \param[in] flags  A set of quoted_printable::encode() flags.
 */
void email::attachment::quoted_printable_encode_and_set_data(
                            QByteArray const & data
                          , QString mime_type
                          , int flags)
{
    std::string const encoded_data(quoted_printable::encode(data.data(), flags));

    QByteArray encoded_data_bytes(encoded_data.c_str(), static_cast<int>(encoded_data.length()));

    set_data(encoded_data_bytes, mime_type);

    add_header(get_name(name_t::SNAP_NAME_CORE_EMAIL_CONTENT_TRANSFER_ENCODING),
               get_name(name_t::SNAP_NAME_CORE_EMAIL_CONTENT_ENCODING_QUOTED_PRINTABLE));
}


/** \brief The email attachment data.
 *
 * This function retrieves the attachment data from this email attachment
 * object. This is generally UTF-8 characters when we are dealing with
 * text (HTML or plain text.)
 *
 * The data type is defined in the Content-Type header which is automatically
 * defined by the mime_type parameter of the set_data() function call. To
 * retrieve the MIME type, use the following:
 *
 * \code
 * QString mime_type(attachment->get_header(get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
 * \endcode
 *
 * \warning
 * This funtion returns the data by copy. Use with care (not repetitively?)
 *
 * \return A copy of this attachment's data.
 */
QByteArray email::attachment::get_data() const
{
    return f_data;
}


/** \brief Retrieve the value of a header.
 *
 * This function returns the value of the named header. If the header
 * is not currently defined, this function returns an empty string.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a header cannot be empty. This exception is raised if
 * \p name is empty.
 *
 * \param[in] name  A valid header name.
 *
 * \return The current value of that header or an empty string if undefined.
 */
QString email::attachment::get_header(QString const & name) const
{
    if(name.isEmpty())
    {
        throw snap_exception_invalid_parameter("email::attachment::get_header(): Cannot retrieve a header with an empty name");
    }

    auto const it(f_headers.find(name));
    if(it != f_headers.end())
    {
        return it->second;
    }

    return QString();
}


/** \brief Add the Content-Disposition field.
 *
 * Helper function to add the Content-Disposition without having to
 * generate the string of the field by hand, especially because the
 * filename needs special care if defined.
 *
 * The disposition is expected to be of type "attachment" by default.
 * You may change that by changing the last parameter to this function.
 *
 * The function also accepts a filename and a date. If the date is set
 * to zero (default) then time() is used.
 *
 * \code
 *      email e;
 *      ...
 *      email::attachment a;
 *      a.set_data(some_pdf_buffer, "application/pdf");
 *      a.set_content_disposition("your-file.pdf");
 *      e.add_attachment(a);
 *      ...
 *      // if in a server plugin, you can use post_email()
 *      sendmail::instance()->post_email(e);
 * \endcode
 *
 * \attention
 * The \p filename parameter can include a full path although only the
 * basename including all extensions are saved in the header. The path
 * is not useful on the destination computer and can even possibly be
 * a security issue in some cases.
 *
 * \warning
 * The modification_date is an int64_t type in microsecond
 * as most often used in Snap! However, emails only use dates with
 * a one second precision so the milli and micro seconds will
 * generally be ignored.
 *
 * \param[in] filename  The name of this attachment file.
 * \param[in] modification_date  The last modification date of this file.
 *                               Defaults to zero meaning use now.
 *                               Value is in microseconds.
 * \param[in] attachment_type  The type of attachment, defaults to "attachment",
 *                             which is all you need in most cases.
 */
void email::attachment::set_content_disposition(QString const & filename, int64_t modification_date, QString const & attachment_type)
{
    // TODO: make use of a WeightedHTTPString::to_string() (class to be renamed!)

    // type
    //
    if(attachment_type.isEmpty())
    {
        throw snap_exception_invalid_parameter("email::attachment::set_content_disposition(): The attachment type cannot be an empty string.");
    }
    QString content_disposition(attachment_type);
    content_disposition += ";";

    // filename (optional)
    //
    QFileInfo file_info(filename);
    QString const file_name(file_info.fileName());
    if(!file_name.isEmpty())
    {
        // the path is not going to be used (should not be at least) for
        // security reasons we think it is better not to include it at all
        //
        content_disposition += " filename=";
        content_disposition += snap_uri::urlencode(file_name);
        content_disposition += ";";
    }

    // modificate-date
    //
    if(modification_date == 0)
    {
        modification_date = time(nullptr) * 1000000;
    }
    content_disposition += " modification-date=\"";
    content_disposition += snap_child::date_to_string(modification_date, snap_child::date_format_t::DATE_FORMAT_EMAIL);
    content_disposition += "\";";

    // save the result in the headers
    //
    add_header(get_name(name_t::SNAP_NAME_CORE_CONTENT_DISPOSITION), content_disposition);
}


/** \brief Check whether a named header was defined in this attachment.
 *
 * Each specific attachment can be given a set of headers that are saved
 * at the beginning of that part in a multi-part email.
 *
 * This function is used to know whther a given header was already
 * defined or not.
 *
 * \note
 * The function returns true whether the header is properly defined or
 * is the empty string.
 *
 * \param[in] name  A valid header name.
 * \param[in] value  The value of this header.
 *
 * \sa set_data()
 */
bool email::attachment::has_header(QString const & name) const
{
    if(name.isEmpty())
    {
        throw snap_exception_invalid_parameter("email::attachment::has_header(): When check the presence of a header, the name cannot be empty.");
    }

    return f_headers.find(name) != f_headers.end();
}


/** \brief Header of this attachment.
 *
 * Each attachment can be assigned a set of headers such as the Content-Type
 * (which is automatically set by the set_data() function.)
 *
 * Headers in an attachment are similar to the headers in the main email
 * only it cannot include certain entries such as the To:, Cc:, etc.
 *
 * In most cases you want to include the filename if the attachment represents
 * a file. Plain text and HTML will generally only need the Content-Type which
 * is already set by a call to the set_data() funciton.
 *
 * Note that the name of a header is case insensitive. So the names
 * "Content-Type" and "content-type" represent the same header. Which
 * one will be used when generating the output is a non-disclosed internal
 * functionality. You probably want to use the SNAP_SENDMAIL_HEADER_...
 * names anyway (at least for those that are defined.)
 *
 * \note
 * The Content-Transfer-Encoding is managed internally and you are not
 * expected to set this value. The Content-Disposition is generally set
 * to "attachment" for files that are attached to the email.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a header cannot be empty. This exception is raised if the name
 * is empty.
 *
 * \todo
 * As we develop a functioning version of sendmail we want to add tests to
 * prevent a set of fields that we will handle internally and thus we do
 * not want users to be able to set here.
 *
 * \param[in] name  A valid header name.
 * \param[in] value  The value of this header.
 *
 * \sa set_data()
 */
void email::attachment::add_header(QString const & name, QString const & value)
{
    if(name.isEmpty())
    {
        throw snap_exception_invalid_parameter("email::attachment::add_header(): When adding a header, the name cannot be empty.");
    }

    f_headers[name] = value;
}


/** \brief Remove a header.
 *
 * This function searches for the \p name header and removes it from the
 * list of defined headers. This is different from setting the value of
 * a header to the empty string as the header continues to exist.
 *
 * \param[in] name  The name of the header to get rid of.
 */
void email::attachment::remove_header(QString const & name)
{
    auto const it(f_headers.find(name));
    if(it != f_headers.end())
    {
        f_headers.erase(it);
    }
}


/** \brief Get all the headers defined in this email attachment.
 *
 * This function returns the map of the headers defined in this email
 * attachment. This can be used to quickly scan all the headers.
 *
 * \note
 * It is important to remember that since this function returns a reference
 * to the map of headers, it may break if you call add_header() while going
 * through the references unless you make a copy.
 *
 * \return A direct and constant reference to the internal header map.
 */
email::header_map_t const & email::attachment::get_all_headers() const
{
    return f_headers;
}


/** \brief Add a related sub-attachment.
 *
 * This function lets you add a related sub-attachment to an email
 * attachment. At this time, this is only accepted on HTML attachments
 * (body) to attach files such as images, CSS, and scripts.
 *
 * \note
 * At this time we prevent you from adding related sub-attachments to
 * already related sub-attachments. Note that emails can have more levels,
 * but we limit the body of the email (very first attachment) to either
 * Text or HTML. If HTML, then the sendmail plugin takes care of
 * adding the Text version. Thus the sendmail email structure is somewhat
 * different from the resulting email.
 *
 * The possible structure of a resulting email is:
 *
 * \code
 * - multipart/mixed
 *   - multipart/alternative
 *     - text/plain
 *     - multipart/related
 *       - text/html
 *       - image/jpg (Images used in text/html)
 *       - image/png
 *       - image/gif
 *       - text/css (the CSS used by the HTML)
 *   - application/pdf (PDF attachment)
 * \endcode
 *
 * The structure of the sendmail attachment for such an email would be:
 *
 * \code
 * - HTML attachment
 *   - image/jpg
 *   - image/png
 *   - image/gif
 *   - text/css
 * - application/pdf
 * \endcode
 *
 * Also, you are much more likely to use the set_email_path() which
 * means you do not have to provide anything more than than the dynamic
 * file attachments (i.e. the application/pdf file in our example here.)
 * Everything else is taken care of by the sendmail plugin.
 *
 * \param[in] data  The attachment to add to this attachment by copy.
 */
void email::attachment::add_related(attachment const & data)
{
    // if we are a sub-attachment, we do not accept a sub-sub-attachment
    //
    if(f_is_sub_attachment)
    {
        throw email_exception_too_many_levels("email::attachment::add_related(): this attachment is already a related sub-attachment, you cannot add more levels");
    }

    // related sub-attachment limitation
    //
    if(data.get_related_count() != 0)
    {
        throw email_exception_too_many_levels("email::attachment::add_related(): you cannot add a related sub-attachment to an attachment when that related sub-attachment has itself a related sub-attachment");
    }

    // create a copy of this attachment
    //
    // note that we do not attempt to use the shared pointer, we make a
    // full copy instead, this is because some people may end up wanting
    // to modify the attachment parameter and then add anew... what will
    // have to a be a different attachment.
    //
    attachment copy(data);

    // mark this as a sub-attachment to prevent users from adding
    // sub-sub-attachments to those
    //
    copy.f_is_sub_attachment = true;

    // save the result in this attachment sub-attachments
    //
    f_sub_attachments.push_back(copy);
}


/** \brief Return the number of sub-attachments.
 *
 * Attachments can be assigned related sub-attachments. For example, an
 * HTML page can be given images, CSS files, etc.
 *
 * This function returns the number of such sub-attachments that were
 * added with the add_attachment() function. The count can be used to
 * retrieve all the sub-attachments with the get_attachment() function.
 *
 * \return The number of sub-attachments.
 *
 * \sa add_related()
 * \sa get_related()
 */
int email::attachment::get_related_count() const
{
    return f_sub_attachments.size();
}


/** \brief Get one of the related sub-attachment of this attachment.
 *
 * This function is used to retrieve the related attachments found in
 * another attachment. These are called sub-attachments.
 *
 * These attachments are viewed as related documents to the main
 * attachment. These are used with HTML at this point to add images,
 * CSS files, etc. to the HTML files.
 *
 * \warning
 * The function returns a reference to the internal object. Calling
 * add_attachment() is likely to invalidate that reference.
 *
 * \exception out_of_range
 * If the index is out of range, this exception is raised.
 *
 * \param[in] index  The attachment index.
 *
 * \return A reference to the attachment.
 *
 * \sa add_related()
 * \sa get_related_count()
 */
email::attachment & email::attachment::get_related(int index) const
{
    if(static_cast<size_t>(index) >= f_sub_attachments.size())
    {
        throw std::out_of_range("email::attachment::get_related() called with an invalid index");
    }
    return const_cast<attachment &>(f_sub_attachments[index]);
}


/** \brief Unserialize an email attachment.
 *
 * This function unserializes an email attachment that was serialized using
 * the serialize() function. This is considered an internal function as it
 * is called by the unserialize() function of the email object.
 *
 * \param[in] r  The reader used to read the input data.
 *
 * \sa serialize()
 */
void email::attachment::unserialize(QtSerialization::QReader & r)
{
    QtSerialization::QComposite comp;
    //QtSerialization::QFieldBool tag_is_sub_attachment(comp, "is_sub_attachment", f_is_sub_attachment);
    QtSerialization::QFieldTag tag_header(comp, "header", this);
    QtSerialization::QFieldTag tag_sub_attachment(comp, "sub-attachment", this);
    QString attachment_data;
    QtSerialization::QFieldString tag_data(comp, "data", attachment_data);
    r.read(comp);
    f_data = QByteArray::fromBase64(attachment_data.toUtf8().data());
}


/** \brief Read the contents one tag from the reader.
 *
 * This function reads the contents of the attachment tag. It handles
 * the attachment header fields.
 *
 * \param[in] name  The name of the tag being read.
 * \param[in] r  The reader used to read the input data.
 */
void email::attachment::readTag(QString const & name, QtSerialization::QReader & r)
{
    if(name == "header")
    {
        QtSerialization::QComposite comp;
        QString header_name;
        QtSerialization::QFieldString tag_name(comp, "name", header_name);
        QString header_value;
        QtSerialization::QFieldString tag_value(comp, "value", header_value);
        r.read(comp);
        f_headers[header_name] = header_value;
    }
    else if(name == "sub-attachment")
    {
        attachment a;
        a.unserialize(r);
        add_related(a);
    }
}


/** \brief Serialize an attachment to a writer.
 *
 * This function serialize an attachment so it can be saved in the database
 * in the form of a string.
 *
 * \param[in,out] w  The writer where the data gets saved.
 */
void email::attachment::serialize(QtSerialization::QWriter & w, bool is_sub_attachment) const
{
    QtSerialization::QWriter::QTag tag(w, is_sub_attachment ? "sub-attachment" : "attachment");
    //QtSerialization::writeTag(w, "is_sub_attachment", f_is_sub_attachment);
    for(auto const & it : f_headers)
    {
        QtSerialization::QWriter::QTag header(w, "header");
        QtSerialization::writeTag(w, "name", it.first);
        QtSerialization::writeTag(w, "value", it.second);
    }
    for(auto const & it : f_sub_attachments)
    {
        it.serialize(w, true);
    }

    // the data may be binary and thus it cannot be saved as is
    // so we encode it using base64
    //
    QtSerialization::writeTag(w, "data", f_data.toBase64().data());
}


/** \brief Compare two attachments against each others.
 *
 * This function compares two attachments against each other and returns
 * true if both are considered equal.
 *
 * \param[in] rhs  The right handside.
 *
 * \return true if both attachments are equal.
 */
bool email::attachment::operator == (attachment const & rhs) const
{
    return f_headers           == rhs.f_headers
        && f_data              == rhs.f_data
        && f_is_sub_attachment == rhs.f_is_sub_attachment
        && f_sub_attachments   == rhs.f_sub_attachments;
}






///////////
// EMAIL //
///////////


/** \brief The version used to serialize emails.
 *
 * This is the major version used when serializing emails.
 */
int const email::EMAIL_MAJOR_VERSION;


/** \brief The version used to serialize emails.
 *
 * This is the minor version used when serializing emails.
 */
int const email::EMAIL_MINOR_VERSION;


/** \brief Initialize an email object.
 *
 * This function initializes an email object making it ready to be
 * setup before processing.
 *
 * The function takes no parameter, although a certain number of
 * parameters are required and must be defined before the email
 * can be sent:
 *
 * \li From -- the name/email of the user sending this email.
 * \li To -- the name/email of the user to whom this email is being sent,
 *           there may be multiple recipients and they may be defined
 *           in Cc or Bcc as well as the To list. The To can also be
 *           defined as a list alias name in which case the backend
 *           will send the email to all the subscribers of that list.
 * \li Subject -- the subject must include something.
 * \li Content -- at least one attachment must be added as the body.
 *
 * Attachments support text emails, HTML pages, and any file (image,
 * PDF, etc.). There is no specific limit to the number of attachments
 * or the size per se, although more email systems do limit the size
 * of an email so we do enforce some limit (i.e. 25Mb).
 */
email::email()
    : f_time(time(nullptr))
{
}


/** \brief Clean up the email object.
 *
 * This function ensures that an email object is cleaned up before
 * getting freed.
 */
email::~email()
{
}


/** \brief Change whether the branding is to be shown or not.
 *
 * By default, the send() function includes a couple of branding
 * headers:
 *
 * \li X-Generated-By
 * \li X-Mailer
 *
 * Those two headers can be removed by setting the branding to false.
 *
 * By default the branding is turned on meaning that it will appear
 * in your emails. Obviously, your mail server can later overwrite
 * or remove those fields.
 *
 * \param[in] branding  The new value for the branding flag, true by default.
 */
void email::set_branding(bool branding)
{
    f_branding = branding;
}


/** \brief Retrieve the branding flag value.
 *
 * This function returns true if the branding of the Snap! system will
 * appear in the email.
 *
 * \return true if branding is on, false otherwise.
 */
bool email::get_branding() const
{
    return f_branding;
}


/** \brief Mark this email as being cumulative.
 *
 * A cumulative email is not sent immediately. Instead it is stored
 * and sent at a later time once certain thresholds are reached.
 * There are two thresholds used at this time: a time threshold, a
 * user may want to receive at most one email every few days; and
 * a count threshold, a user may want to receive an email for every
 * X events.
 *
 * Also, our system is capable to cumulate using an overwrite so
 * the receiver gets one email even if the same object was modified
 * multiple times. For example an administrator may want to know
 * when a type of pages gets modified, but he doesn't want to know
 * of each little change (i.e. the editor may change the page 5
 * times in a row as he/she finds things to tweak over and over
 * again.) The name of the \p object passed as a parameter allows
 * the mail system to cumulate using an overwrite and thus mark
 * that this information should really only be sent once (i.e.
 * instead of saying 10 times that page X changed, the mail system
 * can say it once [although we will include how many edits were
 * made as an additional piece of information.])
 *
 * Note that the user may mark all emails that he/she receives as
 * cumulative or non-cumulative so this flag is useful but it can
 * be ignored by the receivers. The priority can be used by the
 * receiver to decide what to do with an email. (i.e. send urgent
 * emails immediately.)
 *
 * \note
 * You may call the set_cumulative() function with an empty string
 * to turn off the cumulative feature for that email.
 *
 * \warning
 * This feature is not yet implemented by the sendmail plugin. Note
 * that is only for that plugin and not for the email class here
 * which has no knowledge of how to cumulate multiple emails into
 * one.
 *
 * \param[in] object  The name of the object being worked on.
 *
 * \sa get_cumulative()
 */
void email::set_cumulative(QString const & object)
{
    f_cumulative = object;
}


/** \brief Check the cumulative information.
 *
 * This function is used to retreive the cumulative information as saved
 * using the set_cumulative().
 *
 * \warning
 * This feature is not yet implemented by the sendmail plugin. Note
 * that is only for that plugin and not for the email class here
 * which has no knowledge of how to cumulate multiple emails into
 * one.
 *
 * \return A string representing the way the cumulative feature should work.
 *
 * \sa set_cumulative()
 */
QString const & email::get_cumulative() const
{
    return f_cumulative;
}


/** \brief Set the site key of the site sending this email.
 *
 * The site key is saved in the email whenever the post_email() function
 * is called. You do not have to define it, it will anyway be overwritten.
 *
 * The site key is used to check whether an email is being sent to a group
 * and that group is a mailing list. In that case we've got to have the
 * name of the mailing list defined as "\<site-key>: \<list-name>" thus we
 * need access to the site key that generates the email at the time we
 * manage the email (which is from the backend that has no clue what the
 * site key is when reached).
 *
 * \param[in] site_key  The name (key/URI) of the site being built.
 */
void email::set_site_key(QString const & site_key)
{
    f_site_key = site_key;
}


/** \brief Retrieve the site key of the site that generated this email.
 *
 * This function retrieves the value set by the set_site_key() function.
 * It returns an empty string until the post_email() function is called
 * because before that it is not set.
 *
 * The main reason for having the site key is to search the list of
 * email lists when the email gets sent to the end user.
 *
 * \return The site key of the site that generated the email.
 */
QString const & email::get_site_key() const
{
    return f_site_key;
}


/** \brief Define the path to the email in the system.
 *
 * This function sets up the path of the email subject, body, and optional
 * attachments.
 *
 * Other attachments can also be added to the email. However, when a path
 * is defined, the title and body of that page are used as the subject and
 * the body of the email.
 *
 * \note
 * At the time an email gets sent, the permissions of a page are not
 * checked.
 *
 * \warning
 * If you are not in a plugin, this feature and the post will not work for
 * you. Instead you must explicitly define the body and attach it with
 * the set_body_attachment() function. It is not required to add the
 * body attachment first, but it has to be added for the email to work
 * as expected (obviously?!)
 *
 * \param[in] email_path  The path to a page that will be used as the email subject, body, and attachments
 */
void email::set_email_path(QString const & email_path)
{
    f_email_path = email_path;
}


/** \brief Retrieve the path to the page used to generate the email.
 *
 * This email path is set to a page that represents the subject (title) and
 * body of the email. It may also have attachments linked to it.
 *
 * If the path is empty, then the email is generated using the email object
 * and its attachment, the first attachment being the body of the email.
 *
 * \return The path to the page to be used to generate the email subject and
 *         title.
 */
QString const & email::get_email_path() const
{
    return f_email_path;
}


/** \brief Set the email key.
 *
 * When a new email is posted, it is assigned a unique number used as a
 * key in different places.
 *
 * \param[in] email_key  The name (key/URI) of the site being built.
 */
void email::set_email_key(QString const & email_key)
{
    f_email_key = email_key;
}


/** \brief Retrieve the email key.
 *
 * This function retrieves the value set by the set_email_key() function.
 *
 * The email key is set when you call the post_email() function. It is a
 * random number that we also save in the email object so we can keep using
 * it as we go.
 *
 * \return The email key.
 */
QString const & email::get_email_key() const
{
    return f_email_key;
}


/** \brief Retrieve the time when the email object was created.
 *
 * This function retrieves the time when the email was first created.
 *
 * \return The time when the email object was created.
 */
time_t email::get_time() const
{
    return f_time;
}


/** \brief Save the name and email address of the sender.
 *
 * This function saves the name and address of the sender. It has to
 * be valid according to RFC 2822.
 *
 * If you call this function multiple times, only the last \p from
 * information is kept.
 *
 * \note
 * The set_from() function is the same as calling the add_header() with
 * "From" as the field name and \p from as the value. To retrieve that
 * field, you have to use the get_header() function.
 *
 * \exception sendmail_exception_invalid_argument
 * If the \p from parameter is not a valid email address (as per RCF
 * 2822) or there isn't exactly one email address in that parameter,
 * then this exception is raised.
 *
 * \param[in] from  The name and email address of the sender
 */
void email::set_from(QString const & from)
{
    // parse the email to verify that it is valid
    //
    tld_email_list emails;
    if(emails.parse(from.toUtf8().data(), 0) != TLD_RESULT_SUCCESS)
    {
        throw snap_exception_invalid_parameter(QString("email::set_from(): invalid \"From:\" email in \"%1\".").arg(from));
    }
    if(emails.count() != 1)
    {
        throw snap_exception_invalid_parameter("email::set_from(): multiple \"From:\" emails");
    }

    // save the email as the From email address
    //
    f_headers[get_name(name_t::SNAP_NAME_CORE_EMAIL_FROM)] = from;
}


/** \brief Save the names and email addresses of the receivers.
 *
 * This function saves the names and addresses of the receivers. The list
 * of receivers has to be valid according to RFC 2822.
 *
 * If you are call this function multiple times, only the last \p to
 * information is kept.
 *
 * \note
 * The set_to() function is the same as calling the add_header() with
 * "To" as the field name and \p to as the value. To retrieve that
 * field, you have to use the get_header() function.
 *
 * \warning
 * In most cases you can enter any number of receivers, however, when
 * using the email object directly, it is likely to fail if you do so.
 * The sendmail plugin knows how to handle a list of destinations, though.
 *
 * \exception snap_exception_invalid_parameter
 * If the \p to parameter is not a valid list of email addresses (as per
 * RFC 2822) or there is not at least one email address then this exception
 * is raised.
 *
 * \param[in] to  The list of names and email addresses of the receivers.
 */
void email::set_to(QString const & to)
{
    // parse the email to verify that it is valid
    //
    tld_email_list emails;
    if(emails.parse(to.toUtf8().data(), 0) != TLD_RESULT_SUCCESS)
    {
        throw snap_exception_invalid_parameter("email::set_to(): invalid \"To:\" email");
    }
    if(emails.count() < 1)
    {
        // this should never happen because the parser will instead return
        // a result other than TLD_RESULT_SUCCESS
        //
        throw snap_exception_invalid_parameter("email::set_to(): not even one \"To:\" email");
    }

    // save the email as the To email address
    //
    f_headers[get_name(name_t::SNAP_NAME_CORE_EMAIL_TO)] = to;
}


/** \brief The priority is a somewhat arbitrary value defining the email urgency.
 *
 * Many mail system define a priority but it really isn't defined in the
 * RFC 2822 so the value is not well defined.
 *
 * The priority is saved in the X-Priority header.
 *
 * \param[in] priority  The priority of this email.
 */
void email::set_priority(priority_t priority)
{
    QString name;
    switch(priority)
    {
    case priority_t::EMAIL_PRIORITY_BULK:
        name = QString::fromUtf8(get_name(name_t::SNAP_NAME_CORE_EMAIL_PRIORITY_BULK));
        break;

    case priority_t::EMAIL_PRIORITY_LOW:
        name = QString::fromUtf8(get_name(name_t::SNAP_NAME_CORE_EMAIL_PRIORITY_LOW));
        break;

    case priority_t::EMAIL_PRIORITY_NORMAL:
        name = QString::fromUtf8(get_name(name_t::SNAP_NAME_CORE_EMAIL_PRIORITY_NORMAL));
        break;

    case priority_t::EMAIL_PRIORITY_HIGH:
        name = QString::fromUtf8(get_name(name_t::SNAP_NAME_CORE_EMAIL_PRIORITY_HIGH));
        break;

    case priority_t::EMAIL_PRIORITY_URGENT:
        name = QString::fromUtf8(get_name(name_t::SNAP_NAME_CORE_EMAIL_PRIORITY_URGENT));
        break;

    default:
        throw snap_exception_invalid_parameter(QString("email::set_priority(): Unknown priority \"%1\".")
                                                        .arg(static_cast<int>(priority)));

    }

    f_headers[get_name(name_t::SNAP_NAME_CORE_EMAIL_X_PRIORITY)] = QString("%1 (%2)").arg(static_cast<int>(priority)).arg(name);
    f_headers[get_name(name_t::SNAP_NAME_CORE_EMAIL_X_MSMAIL_PRIORITY)] = name;
    f_headers[get_name(name_t::SNAP_NAME_CORE_EMAIL_IMPORTANCE)] = name;
    f_headers[get_name(name_t::SNAP_NAME_CORE_EMAIL_PRECEDENCE)] = name;
}


/** \brief Set the email subject.
 *
 * This function sets the subject of the email. Anything is permitted although
 * you should not send emails with an empty subject.
 *
 * The system takes care of encoding the subject if required. It will also trim
 * it and remove any unwanted characters (tabs, new lines, etc.)
 *
 * The subject line is also silently truncated to a reasonable size.
 *
 * Note that if the email is setup with a path to a page, the title of that
 * page is used as the default subject. If the set_subject() function is
 * called with a valid subject (not empty) then the page title is ignored.
 *
 * \note
 * The set_subject() function is the same as calling the add_header() with
 * "Subject" as the field name and \p subject as the value.
 *
 * \param[in] subject  The subject of the email.
 */
void email::set_subject(QString const & subject)
{
    f_headers[get_name(name_t::SNAP_NAME_CORE_EMAIL_SUBJECT)] = subject;
}


/** \brief Add a header to the email.
 *
 * The system takes care of most of the email headers but this function gives
 * you the possibility to add more.
 *
 * Note that the priority should instead be set with the set_priority()
 * function. This way it is more likely to work in all system that the
 * sendmail plugin supports.
 *
 * The content type should not be set. The system automatically takes care of
 * that for you including required encoding information, attachments, etc.
 *
 * The To, Cc, and Bcc fields are defined in this way. If multiple
 * destinations are defined, you must concatenate them in the
 * \p value parameter before calling this function.
 *
 * Note that the name of a header is case insensitive. So the names
 * "Content-Type" and "content-type" represent the same header. Which
 * one will be used when generating the output is a non-disclosed internal
 * functionality. You probably want to use the SNAP_SENDMAIL_HEADER_...
 * names anyway (at least for those that are defined.)
 *
 * \warning
 * Also the function is called 'add', because you may add as many headers as you
 * need, the function does NOT cumulate data within one field. Instead it
 * overwrites the content of the field. This is one way to replace an unwanted
 * value or force the content of a field for a given email.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a header cannot be empty. This exception is raised if
 * \p name is empty. The field name is also validated by the TLD library
 * and must be composed of letters, digits, the dash character, and it
 * has to start with a letter. The case is not important, however.
 * Also, if the field represents an email or a list of emails, the
 * value is also checked for validity.
 *
 * \param[in] name  A valid header name.
 * \param[in] value  The value of this header.
 */
void email::add_header(QString const & name, QString const & value)
{
    // first define a type
    //
    tld_email_field_type type(tld_email_list::email_field_type(name.toUtf8().data()));
    if(type == TLD_EMAIL_FIELD_TYPE_INVALID)
    {
        // this includes the case where the field name is empty
        throw snap_exception_invalid_parameter("email::add_header(): Invalid header name for a header name.");
    }

    // if type is not unknown, check the actual emails
    //
    // "UNKNOWN" means we don't consider the value of this header to be
    // one or more emails
    //
    if(type != TLD_EMAIL_FIELD_TYPE_UNKNOWN)
    {
        // The Bcc and alike fields may be empty
        //
        if(type != TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST_OPT
        || !value.isEmpty())
        {
            // if not unknown then we should check the field value
            // as a list of emails
            //
            tld_email_list emails;
            if(emails.parse(value.toUtf8().data(), 0) != TLD_RESULT_SUCCESS)
            {
                // TODO: this can happen if a TLD becomes obsolete and
                //       a user did not update one's email address.
                //
                throw snap_exception_invalid_parameter(QString("email::add_header(): Invalid emails in header field: \"%1: %2\"")
                                                            .arg(name)
                                                            .arg(value));
            }

            // for many fields it can have at most one mailbox
            //
            if(type == TLD_EMAIL_FIELD_TYPE_MAILBOX
            && emails.count() != 1)
            {
                throw snap_exception_invalid_parameter(QString("email::add_header(): Header field expects exactly one email in: \"%1: %2\"")
                                                            .arg(name)
                                                            .arg(value));
            }
        }
    }

    f_headers[name] = value;
}


/** \brief Remove a header.
 *
 * This function searches for the \p name header and removes it from the
 * list of defined headers. This is different from setting the value of
 * a header to the empty string as the header continues to exist.
 *
 * In most cases, you may just set a header to the empty string
 * to delete it, however, removing it is cleaner.
 *
 * \param[in] name  The name of the header to get rid of.
 */
void email::remove_header(QString const & name)
{
    auto const it(f_headers.find(name));
    if(it != f_headers.end())
    {
        f_headers.erase(it);
    }
}


/** \brief Check whether a header is defined or not.
 *
 * This function returns true if the header was defined (add_header() was
 * called at least once on that header name.)
 *
 * This function will return true even if the header was set to the empty
 * string.
 *
 * \param[in] name  The name of the header to get rid of.
 */
bool email::has_header(QString const & name) const
{
    if(name.isEmpty())
    {
        throw snap_exception_invalid_parameter("email::has_header(): Cannot check for a header with an empty name.");
    }

    return f_headers.find(name) != f_headers.end();
}


/** \brief Retrieve the value of a header.
 *
 * This function returns the value of the named header. If the header
 * is not currently defined, this function returns an empty string.
 *
 * To know whether a header is defined, you may instead call the
 * has_header() even if in most cases an empty string is very much
 * similar to an undefined header.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a header cannot be empty. This exception is raised if
 * \p name is empty.
 *
 * \param[in] name  A valid header name.
 *
 * \return The current value of that header or an empty string if undefined.
 */
QString email::get_header(QString const & name) const
{
    if(name.isEmpty())
    {
        throw snap_exception_invalid_parameter("email::get_header(): Cannot retrieve a header with an empty name.");
    }

    auto const it(f_headers.find(name));
    if(it != f_headers.end())
    {
        return it->second;
    }

    // return f_headers[name] -- this would create an entry for f_headers[name] for nothing
    return QString();
}


/** \brief Get all the headers defined in this email.
 *
 * This function returns the map of the headers defined in this email. This
 * can be used to quickly scan all the headers.
 *
 * \note
 * It is important to remember that since this function returns a reference
 * to the map of headers, it may break if you call add_header() while going
 * through the references unless you make a copy.
 *
 * \return A direct constant reference to the internal header map.
 */
email::header_map_t const & email::get_all_headers() const
{
    return f_headers;
}


/** \brief Add the body attachment to this email.
 *
 * When creating an email with a path to a page (which is close to mandatory
 * if you want to have translation and let users of your system to be able
 * to edit the email in all languages.)
 *
 * This function should be private because it should only be used internally.
 * Unfortunately, the function is used from the outside. But you've been
 * warn. Really, this is using a push_front() instead of a push_back() it
 * is otherwise the same as the add_attachment() function and you may want
 * to read that function's documentation too.
 *
 * \param[in] data  The attachment to add as the body of this email.
 *
 * \sa email::add_attachment()
 */
void email::set_body_attachment(attachment const & data)
{
    f_attachments.insert(f_attachments.begin(), data);
}


/** \brief Add an attachment to this email.
 *
 * All data appearing in the body of the email is defined using attachments.
 * This includes the normal plain text body if you use one. See the
 * attachment class for details on how to create an attachment
 * for an email.
 *
 * Note that if you want to add a plain text and an HTML version to your
 * email, these are sub-attachments to one attachment of the email defined
 * as alternatives. If only that one attachment is added to an email then
 * it won't be made a sub-attachment in the final email buffer.
 *
 * \b IMPORTANT \b NOTE: the body and subject of emails are most often defined
 * using a path to a page. This means the first attachment is to be viewed
 * as an attachment, not the main body. Also, the attachments of the page
 * are also viewed as attachments of the email and will appear before the
 * attachments added here.
 *
 * \note
 * It is important to note that the attachments are written in the email
 * in the order they are defined here. It is quite customary to add the
 * plain text first, then the HTML version, then the different files to
 * attach to the email.
 *
 * \param[in] data  The email attachment to add by copy.
 *
 * \sa email::set_body_attachment()
 */
void email::add_attachment(attachment const & data)
{
    f_attachments.push_back(data);
}


/** \brief Retrieve the number of attachments defined in this email.
 *
 * This function defines the number of attachments that were added to this
 * email. This is useful to retrieve the attachments with the
 * get_attachment() function.
 *
 * \return The number of attachments defined in this email.
 *
 * \sa add_attachment()
 * \sa get_attachment()
 */
int email::get_attachment_count() const
{
    return f_attachments.size();
}


/** \brief Retrieve the specified attachement.
 *
 * This function gives you a read/write reference to the specified
 * attachment. This is used by plugins that need to access email
 * data to filter it one way or the other (i.e. change all the tags
 * with their corresponding values.)
 *
 * The \p index parameter must be a number between 0 and
 * get_attachment_count() minus one. If no attachments were added
 * then this function cannot be called.
 *
 * \exception out_of_range
 * If the index is out of range, this exception is raised.
 *
 * \param[in] index  The index of the attachment to retrieve.
 *
 * \return A reference to the corresponding attachment.
 *
 * \sa add_attachment()
 * \sa get_attachment_count()
 */
email::attachment & email::get_attachment(int index) const
{
    if(static_cast<size_t>(index) >= f_attachments.size())
    {
        throw std::out_of_range("email::get_attachment() called with an invalid index");
    }
    return const_cast<email::attachment &>(f_attachments[index]);
}


/** \brief Add a parameter to the email.
 *
 * Whenever you create an email, you may be able to offer additional
 * parameters that are to be used as token replacement in the email.
 * For example, when creating a new user, we ask the user to verify his
 * email address. This is done by creating a session identifier and then
 * asking the user to go to the special page /verify/\<session>. That
 * way we know that the user received the email (although it may not
 * exactly be the right person...)
 *
 * The name of the parameter should be something like "users::verify",
 * i.e. it should be namespace specific to not clash with sendmail or
 * other plugins parameters.
 *
 * All parameters have case sensitive names. So sendmail and Sendmail
 * are not equal. However, all parameters should use lowercase only
 * to match conventional XML tag and attribute names.
 *
 * \warning
 * Also the function is called 'add', because you may add as many parameters
 * as you have available, the function does NOT cumulate data within one field.
 * Instead it overwrites the content of the field if set more than once. This
 * is one way to replace an unwanted value or force the content of a field
 * for a given email.
 *
 * \exception snap_exception_invalid_parameter
 * The name of a parameter cannot be empty. This exception is raised if
 * \p name is empty.
 *
 * \param[in] name  A valid parameter name.
 * \param[in] value  The value of this header.
 */
void email::add_parameter(QString const & name, QString const & value)
{
    if(name.isEmpty())
    {
        throw snap_exception_invalid_parameter("email::add_parameter(): Cannot add a parameter with an empty name.");
    }

    f_parameters[name] = value;
}


/** \brief Retrieve the value of a named parameter.
 *
 * This function returns the value of the named parameter. If the parameter
 * is not currently defined, this function returns an empty string.
 *
 * \exception snap_exception_invalid_parameter
 * The name of a parameter cannot be empty. This exception is raised if
 * \p name is empty.
 *
 * \param[in] name  A valid parameter name.
 *
 * \return The current value of that parameter or an empty string if undefined.
 */
QString email::get_parameter(QString const & name) const
{
    if(name.isEmpty())
    {
        throw snap_exception_invalid_parameter("email::get_parameter(): Cannot retrieve a parameter with an empty name.");
    }
    auto const it(f_parameters.find(name));
    if(it != f_parameters.end())
    {
        return it->second;
    }

    return QString();
}


/** \brief Get all the parameters defined in this email.
 *
 * This function returns the map of the parameters defined in this email.
 * This can be used to quickly scan all the parameters.
 *
 * \note
 * It is important to remember that since this function returns a reference
 * to the map of parameters, it may break if you call add_parameter() while
 * going through the references.
 *
 * \return A direct reference to the internal parameter map.
 */
const email::parameter_map_t & email::get_all_parameters() const
{
    return f_parameters;
}


/** \brief Unserialize an email message.
 *
 * This function unserializes an email message that was serialized using
 * the serialize() function.
 *
 * You are expected to first create an email object and then call this
 * function with the data parameter set as the string that the serialize()
 * function returned.
 *
 * You may setup some default headers such as the X-Mailer value in your
 * email object before calling this function. If such header information
 * is defined in the serialized data then it will be overwritten with
 * that data. Otherwise it will remain the same.
 *
 * The function doesn't return anything. Instead it unserializes the
 * \p data directly in this email object.
 *
 * \param[in] data  The serialized email data to transform.
 *
 * \sa serialize()
 */
void email::unserialize(QString const & data)
{
    // QBuffer takes a non-const QByteArray so we have to create a copy
    QByteArray non_const_data(data.toUtf8().data());
    QBuffer in(&non_const_data);
    in.open(QIODevice::ReadOnly);
    QtSerialization::QReader reader(in);
    QtSerialization::QComposite comp;
    QtSerialization::QFieldTag rules(comp, "email", this);
    reader.read(comp);
}


/** \brief Read the contents of one tag from the reader.
 *
 * This function reads the contents of the main email tag. It calls
 * the attachment unserialize() as required whenever an attachment
 * is found in the stream.
 *
 * \param[in] name  The name of the tag being read.
 * \param[in] r  The reader used to read the input data.
 */
void email::readTag(const QString& name, QtSerialization::QReader& r)
{
    //if(name == "email")
    //{
    //    QtSerialization::QComposite comp;
    //    QtSerialization::QFieldTag info(comp, "content", this);
    //    r.read(comp);
    //}
    if(name == "email")
    {
        QtSerialization::QComposite comp;
        QtSerialization::QFieldBool tag_branding(comp, "branding", f_branding);
        QtSerialization::QFieldString tag_cumulative(comp, "cumulative", f_cumulative);
        QtSerialization::QFieldString tag_site_key(comp, "site_key", f_site_key);
        QtSerialization::QFieldString tag_email_path(comp, "email_path", f_email_path);
        QtSerialization::QFieldString tag_email_key(comp, "email_key", f_email_key);
        QtSerialization::QFieldTag tag_header(comp, "header", this);
        QtSerialization::QFieldTag tag_attachment(comp, "attachment", this);
        QtSerialization::QFieldTag tag_parameter(comp, "parameter", this);
        r.read(comp);
    }
    else if(name == "header")
    {
        QtSerialization::QComposite comp;
        QString header_name;
        QtSerialization::QFieldString tag_name(comp, "name", header_name);
        QString header_value;
        QtSerialization::QFieldString tag_value(comp, "value", header_value);
        r.read(comp);
        f_headers[header_name] = header_value;
    }
    else if(name == "attachment")
    {
        attachment a;
        a.unserialize(r);
        add_attachment(a);
    }
    else if(name == "parameter")
    {
        QtSerialization::QComposite comp;
        QString parameter_name;
        QtSerialization::QFieldString tag_name(comp, "name", parameter_name);
        QString parameter_value;
        QtSerialization::QFieldString tag_value(comp, "value", parameter_value);
        r.read(comp);
        f_parameters[parameter_name] = parameter_value;
    }
}


/** \brief Transform the email in one string.
 *
 * This function transform the email data in one string so it can easily
 * be saved in the Cassandra database. This is done so it can be sent to
 * the recipients using the backend process preferably on a separate
 * computer (i.e. a computer that is not being accessed by your web
 * clients.)
 *
 * The unserialize() function can be used to restore an email that was
 * previously serialized with this function.
 *
 * \return The email object in the form of a string.
 *
 * \sa unserialize()
 */
QString email::serialize() const
{
    QByteArray result;
    QBuffer archive(&result);
    archive.open(QIODevice::WriteOnly);
    {
        QtSerialization::QWriter w(archive, "email", EMAIL_MAJOR_VERSION, EMAIL_MINOR_VERSION);
        QtSerialization::QWriter::QTag tag(w, "email");
        QtSerialization::writeTag(w, "branding", f_branding);
        if(!f_cumulative.isEmpty())
        {
            QtSerialization::writeTag(w, "cumulative", f_cumulative);
        }
        QtSerialization::writeTag(w, "site_key", f_site_key);
        QtSerialization::writeTag(w, "email_path", f_email_path);
        QtSerialization::writeTag(w, "email_key", f_email_key);
        for(auto const & it : f_headers)
        {
            QtSerialization::QWriter::QTag header(w, "header");
            QtSerialization::writeTag(w, "name", it.first);
            QtSerialization::writeTag(w, "value", it.second);
        }
        for(auto const & it : f_attachments)
        {
            it.serialize(w, false);
        }
        for(auto const & it : f_parameters)
        {
            QtSerialization::QWriter::QTag parameter(w, "parameter");
            QtSerialization::writeTag(w, "name", it.first);
            QtSerialization::writeTag(w, "value", it.second);
        }
        // end the writer so everything gets saved in the buffer (result)
    }

    return QString::fromUtf8(result.data());
}


/** \brief Send this email.
 *
 * This function sends  the specified email. It generates all the body
 * and attachments, etc.
 *
 * Note that the function uses callbacks in order to retrieve the body
 * and attachment from the database as the Snap! environment uses those
 * for much of the data to be sent in emails. However, it is not a
 * requirements in case you want to send an email from another server
 * than a snap_child or snap_backend.
 *
 * \exception email_exception_missing_parameter
 * If the From header or the destination email only are missing this
 * exception is raised.
 *
 * \return true if the send() command worked (note that does not mean the
 * email made it; we'll know later whether it failed if we received a
 * bounced email).
 */
bool email::send() const
{
    // verify that the `From` and `To` headers are defined
    //
    QString const from(get_header(get_name(name_t::SNAP_NAME_CORE_EMAIL_FROM)));
    QString const to(get_header(get_name(name_t::SNAP_NAME_CORE_EMAIL_TO)));

    if(from.isEmpty()
    || to.isEmpty())
    {
        throw snap_exception_missing_parameter("email::send() called without a From or a To header field defined. Make sure you call the set_from() and set_header() functions appropriately.");
    }

    // verify that we have at least one attachment
    // (the body is an attachment)
    //
    int const max_attachments(get_attachment_count());
    if(max_attachments < 1)
    {
        throw snap_exception_missing_parameter("email::send() called without at least one attachment (body).");
    }

    // we want to transform the body from HTML to text ahead of time
    //
    attachment const & body_attachment(get_attachment(0));

    // TODO: verify that the body is indeed HTML!
    //       although html2text works against plain text but that is a waste
    //
    //       also, we should offer a way for the person creating an email
    //       to specify both: a plain text body and an HTML body
    //
    QString plain_text;
    QString const body_mime_type(body_attachment.get_header(get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));

    // TODO: this test is wrong as it would match things like "text/html-special"
    //
    if(body_mime_type.mid(0, 9) == "text/html")
    {
        process p("html2text");
        p.set_mode(process::mode_t::PROCESS_MODE_INOUT);
        p.set_command("html2text");
        //p.add_argument("-help");
        p.add_argument("-nobs");
        p.add_argument("-utf8");
        p.add_argument("-style");
        p.add_argument("pretty");
        p.add_argument("-width");
        p.add_argument("70");
        std::string html_data;
        QByteArray data(body_attachment.get_data());

        // TODO: support other encoding, err if not supported
        //
        if(body_attachment.get_header(get_name(name_t::SNAP_NAME_CORE_EMAIL_CONTENT_TRANSFER_ENCODING))
                                == get_name(name_t::SNAP_NAME_CORE_EMAIL_CONTENT_ENCODING_QUOTED_PRINTABLE))
        {
            // if it was quoted-printable encoded, we have to decode
            //
            // I know, we encode in this very function and could just
            // keep a copy of the original, HOWEVER, the end user could
            // build the whole email with this encoding already in place
            // and thus we anyway would have to decode... This being said,
            // we could have that as an optimization XXX
            //
            html_data = quoted_printable::decode(data.data());
        }
        else
        {
            html_data = data.data();
        }
        p.set_input(QString::fromUtf8(html_data.c_str()));

        // conver that HTML to plain text
        //
        int const r(p.run());
        if(r == 0)
        {
            plain_text = p.get_output();
        }
        else
        {
            // no plain text, but let us know that something went wrong at least
            //
            SNAP_LOG_WARNING
                << "An error occurred while executing html2text (exit code: "
                << r
                << ")"
                << SNAP_LOG_SEND;
        }
    }

    // convert the "from" email address in a TLD email address so we can use
    // the f_email_only version for the command line "sender" parameter
    //
    tld_email_list from_list;
    if(from_list.parse(from.toUtf8().data(), 0) != TLD_RESULT_SUCCESS)
    {
        throw snap_exception_invalid_parameter(QString("email::send() called with invalid sender email address: \"%1\" (parsing failed).").arg(from));
    }
    tld_email_list::tld_email_t s;
    if(!from_list.next(s))
    {
        throw snap_exception_invalid_parameter(QString("email::send() called with invalid sender email address: \"%1\" (no email returned).").arg(from));
    }

    // convert the "to" email address in a TLD email address so we can use
    // the f_email_only version for the command line "to" parameter
    //
    tld_email_list to_list;
    if(to_list.parse(to.toUtf8().data(), 0) != TLD_RESULT_SUCCESS)
    {
        throw snap_exception_invalid_parameter(QString("email::send() called with invalid destination email address: \"%1\" (parsing failed).").arg(to));
    }
    tld_email_list::tld_email_t m;
    if(!to_list.next(m))
    {
        throw snap_exception_invalid_parameter(QString("email::send() called with invalid destination email address: \"%1\" (no email returned).").arg(to));
    }

    // create an output stream to send the email
    //
    cppprocess::process p("sendmail");
    p.set_command("sendmail");
    p.add_argument("-f");
    p.add_argument(s.f_email_only);
    p.add_argument(m.f_email_only);
    SNAP_LOG_TRACE("sendmail command: [")(p.get_command_line())("]");

    cppprocess::io_data_pipe::pointer_t in(std::make_shared<cppprocess::io_data_pipe>());
    p.set_io_input(in);

    int const start_status(p.start());
    if(start_status != 0)
    {
        SNAP_LOG_ERROR
            << "could not start process \""
            << p.get_name()
            << "\" (command line: "
            << p.get_command_line()
            << ")."
            << SNAP_LOG_SEND;
        return false;
    }

    //snap_pipe spipe(cmd, snap_pipe::mode_t::PIPE_MODE_IN);
    //std::ostream f(&spipe);

    // convert email data to text and send that to the sendmail command line
    //
    email::header_map_t headers(f_headers);
    bool const body_only(max_attachments == 1 && plain_text.isEmpty());
    QString boundary;
    if(body_only)
    {
        // if the body is by itself, then its encoding needs to be transported
        // to the main set of headers
        //
        if(body_attachment.get_header(get_name(name_t::SNAP_NAME_CORE_EMAIL_CONTENT_TRANSFER_ENCODING))
                                == get_name(name_t::SNAP_NAME_CORE_EMAIL_CONTENT_ENCODING_QUOTED_PRINTABLE))
        {
            headers[get_name(name_t::SNAP_NAME_CORE_EMAIL_CONTENT_TRANSFER_ENCODING)]
                                = get_name(name_t::SNAP_NAME_CORE_EMAIL_CONTENT_ENCODING_QUOTED_PRINTABLE);
        }
    }
    else
    {
        // boundary      := 0*69<bchars> bcharsnospace
        // bchars        := bcharsnospace / " "
        // bcharsnospace := DIGIT / ALPHA / "'" / "(" / ")" /
        //                  "+" / "_" / "," / "-" / "." /
        //                  "/" / ":" / "=" / "?"
        //
        // Note: we generate boundaries without special characters
        //       (and especially no spaces or dashes) to make it simpler
        //
        // Note: the boundary starts wity "=S" which is not a valid
        //       quoted-printable sequence of characters (on purpose)
        //
        char const allowed[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; //'()+_,./:=?";
        boundary = "=Snap.Websites=";
        for(int i(0); i < 20; ++i)
        {
            // this is just for boundaries, so rand() is more than enough
            // it just needs to not match anything in the emails
            //
            int const c(static_cast<int>(rand() % (sizeof(allowed) - 1)));
            boundary += allowed[c];
        }
        headers[get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)] = "multipart/mixed;\n  boundary=\"" + boundary + "\"";
        headers[get_name(name_t::SNAP_NAME_CORE_EMAIL_MIME_VERSION)] = "1.0";
    }

    // setup the "Date: ..." field if not already defined
    //
    if(headers.find(get_name(name_t::SNAP_NAME_CORE_DATE)) == headers.end())
    {
        // the date must be specified in English only which prevents us from
        // using the strftime()
        //
        headers[get_name(name_t::SNAP_NAME_CORE_DATE)] = snap_child::date_to_string(time(nullptr) * 1000000, snap_child::date_format_t::DATE_FORMAT_EMAIL);
    }

    // setup a default "Content-Language: ..." because in general
    // that makes things work better
    //
    if(headers.find(get_name(name_t::SNAP_NAME_CORE_CONTENT_LANGUAGE)) == headers.end())
    {
        headers[get_name(name_t::SNAP_NAME_CORE_CONTENT_LANGUAGE)] = "en-us";
    }

    for(auto const & it : headers)
    {
        // TODO: the it.second needs to be URI encoded to be valid
        //       in an email; if some characters appear that need
        //       encoding, we should err (we probably want to
        //       capture those in the add_header() though)
        //
        //f << it.first << ": " << it.second << std::endl;
        in.add_input(it.first);
        in.add_input(": ");
        in.add_input(it.second);
        in.add_input("\n");
    }

    // XXX: allow administrators to change the `branding` flag
    //
    if(f_branding)
    {
        //f << "X-Generated-By: Snap! Websites C++ v" SNAPWEBSITES_VERSION_STRING " (https://snapwebsites.org/)" << std::endl
        //  << "X-Mailer: Snap! Websites C++ v" SNAPWEBSITES_VERSION_STRING " (https://snapwebsites.org/)" << std::endl;
        in.add_input(
            "X-Generated-By: Snap! Websites C++ v" SNAPWEBSITES_VERSION_STRING " (https://snapwebsites.org/)\n"
            "X-Mailer: Snap! Websites C++ v" SNAPWEBSITES_VERSION_STRING " (https://snapwebsites.org/)\n";
    }

    // end the headers
    //
    //f << std::endl;
    in.add_input("\n");

    if(body_only)
    {
        // in this case we only have one entry, probably HTML, and thus we
        // can avoid the multi-part headers and attachments
        //
        //f << body_attachment.get_data().data() << std::endl;
        in.add_input(body_attachment.get_data().data());
        in.add_input("\n");
    }
    else
    {
        // TBD: should we make this text changeable by client?
        //
        //f << "The following are various parts of a multipart email." << std::endl
        //  << "It is likely to include a text version (first part) that you should" << std::endl
        //  << "be able to read as is." << std::endl
        //  << "It may be followed by HTML and then various attachments." << std::endl
        //  << "Please consider installing a MIME capable client to read this email." << std::endl
        //  << std::endl;
        in.add_input(
                "The following are various parts of a multipart email.\n"
                "It is likely to include a text version (first part) that you should\n"
                "be able to read as is.\n"
                "It may be followed by HTML and then various attachments.\n"
                "Please consider installing a MIME capable client to read this email.\n"
                "\n");

        int i(0);
        if(!plain_text.isEmpty())
        {
            // if we have plain text then we have alternatives
            //
            //f << "--" << boundary << std::endl
            //  << "Content-Type: multipart/alternative;" << std::endl
            //  << "  boundary=\"" << boundary << ".msg\"" << std::endl
            //  << std::endl
            //  << "--" << boundary << ".msg" << std::endl
            //  << "Content-Type: text/plain; charset=\"utf-8\"" << std::endl
            //  //<< "MIME-Version: 1.0" << std::endl -- only show this one in the main header
            //  << "Content-Transfer-Encoding: quoted-printable" << std::endl
            //  << "Content-Description: Mail message body" << std::endl
            //  << std::endl
            //  << quoted_printable::encode(plain_text.toUtf8().data(), quoted_printable::QUOTED_PRINTABLE_FLAG_NO_LONE_PERIOD) << std::endl;
            in.add_input("--");
            in.add_input(boundary);
            in.add_input("\n"
                         "Content-Type: multipart/alternative;\n"
                         "  boundary-\"");
            in.add_input(boundary);
            in.add_input(".msg\"\n"
                         "\n");
            in.add_input("--");
            in.add_input(boundary);
            in.add_input(".msg\n");
            in.add_input("\n"
                         "Content-Type: text/plain; charset=\"utf-8\"\n"
                         "Content-Transfer-Encoding: quoted-printable\n"
                         "Content-Description: Mail message body\n"
                         "\n");
            in.add_input(
                      quoted_printable::encode(
                              plain_text.toUtf8().data()
                            , quoted_printable::QUOTED_PRINTABLE_FLAG_NO_LONE_PERIOD));
            in.add_input("\n");

            // at this time, this if() should always be true
            //
            if(i < max_attachments)
            {
                // now include the HTML
                //
                //f << "--" << boundary << ".msg" << std::endl;
                in.add_input("--");
                in.add_input(boundary);
                in.add_input(".msg\n");
                for(auto const & it : body_attachment.get_all_headers())
                {
                    //f << it.first << ": " << it.second << std::endl;
                    in.add_input(it.first);
                    in.add_input(": ");
                    in.add_input(it.second);
                    in.add_input("\n");
                }
                // one empty line to end the headers
                //
                in.add_input("\n");

                // here the data in body_attachment is already encoded
                //
                //f << std::endl
                //  << body_attachment.get_data().data() << std::endl
                //  << "--" << boundary << ".msg--" << std::endl
                //  << std::endl;
                in.add_input(body_attachment.get_data().data());
                in.add_input("--");
                in.add_input(boundary);
                in.add_input(".msg--\n\n");

                // we used "attachment" 0, so print the others starting at 1
                //
                i = 1;
            }
        }

        // send the remaining attachments (possibly attachment 0 if
        // we did not have plain text)
        //
        for(; i < max_attachments; ++i)
        {
            // work on this attachment
            //
            email::attachment const & a(get_attachment(i));

            // send the boundary
            //
            //f << "--" << boundary << std::endl;
            in.add_input("--");
            in.add_input(boundary);
            in.add_input("\n");

            // send  the headers for that attachment
            //
            // we get a copy and modify it slightly by making sure that
            // the filename is defined in both the Content-Disposition
            // and the Content-Type
            //
            header_map_t attachment_headers(a.get_all_headers());
            copy_filename_to_content_type(attachment_headers);
            for(auto const & it : attachment_headers)
            {
                //f << it.first << ": " << it.second << std::endl;
                in.add_input(it.first);
                in.add_input(": ");
                in.add_input(it.second);
                in.add_input("\n");
            }
            // one empty line to end the headers
            //
            //f << std::endl;
            in.add_input("\n");

            // here the data is already encoded
            //
            //f << a.get_data().data() << std::endl;
            in.add_input(a.get_data().data());
            in.add_input("\n");
        }

        // last boundary to end them all
        //
        //f << "--" << boundary << "--" << std::endl;
        in.add_input("--");
        in.add_input(boundary);
        in.add_input("--\n");
    }

    // end the message
    //
    //f << std::endl
    //  << "." << std::endl;
    in.add_input("\n"
                 ".\n");

    // make sure the ostream gets flushed or some data could be left in
    // a cache and never written to the pipe (unlikely since we do not
    // use the cache, but future C++ versions could have a problem.)
    //
    //f.flush();

    // close pipe as soon as we are done writing to it
    // and return true if it all worked as expected
    //
    //return spipe.close_pipe() == 0;

    // TODO: this needs to be using ed::communicator so we need a callback
    //       if we want to support a similar "interface" (i.e. if the caller
    //       wants to know whether it worked)
    //
    return p.wait() == 0;
}


/** \brief Compare two email obejcts for equality.
 *
 * This function checks whether two email objects are equal.
 *
 * \param[in] rhs  The right hand side email.
 *
 * \return true if both emails are considered equal.
 */
bool email::operator == (email const & rhs) const
{
    return f_branding    == rhs.f_branding
        && f_cumulative  == rhs.f_cumulative
        && f_site_key    == rhs.f_site_key
        && f_email_path  == rhs.f_email_path
        && f_email_key   == rhs.f_email_key
        //&& f_time        == rhs.f_time -- this is pretty much never going to be equal so do not compare
        && f_headers     == rhs.f_headers
        && f_attachments == rhs.f_attachments
        && f_parameters  == rhs.f_parameters;
}


}
// snap namespace
// vim: ts=4 sw=4 et
