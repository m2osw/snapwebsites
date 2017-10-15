// Snap Websites Server -- handle various locale information such as timezone and date output, number formatting for display, etc.
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

#include "snap_locale.h"
#include "qunicodestring.h"

#include "../content/content.h"

#include <snapwebsites/log.h>
#include <snapwebsites/mkgmtime.h>
#include <snapwebsites/not_used.h>

#include <unicode/datefmt.h>
#include <unicode/errorcode.h>
#include <unicode/locid.h>
#include <unicode/smpdtfmt.h>
#include <unicode/timezone.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_START(locale, 1, 0)


/* \brief Get a fixed locale name.
 *
 * The locale plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \li Example of usage
 *
 * Convert a date using UTC as the timezone (to avoid DST side effects),
 * this assumes date_val is a time_t variable which has time set to
 * 00:00:00 and only the date is of interest to you:
 *
 * \code
 *    {
 *        locale::safe_timezone const utc_timezone("UTC");
 *
 *        QString date_str(locale::locale::instance()->format_date(date_val));
 *    }
 * \endcode
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_LOCALE_SETTINGS_LOCALE: // this is to retrieve the locale settings even when the locale_settings plugin is not installed
        return "locale_settings::locale";

    case name_t::SNAP_NAME_LOCALE_SETTINGS_PATH: // this is to retrieve the locale settings even when the locale_settings plugin is not installed
        return "admin/settings/locale";

    case name_t::SNAP_NAME_LOCALE_SETTINGS_TIMEZONE: // this is to retrieve the locale settings even when the locale_settings plugin is not installed
        return "locale_settings::timezone";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_LOCALE_...");

    }
    NOTREACHED();
}




/** \brief Safely change the timezone.
 *
 * This class ensures that a change to the current timezone gets restored
 * even when an exception occurs.
 *
 * \param[in] new_timezone  The timezone to use for a while (until this
 *                          object gets destroyed.)
 */
safe_timezone::safe_timezone(QString const new_timezone)
    : f_locale_plugin(locale::locale::instance())
    , f_old_timezone(f_locale_plugin->get_current_timezone())
{
    f_locale_plugin->set_current_timezone(new_timezone);
}


/** \brief Restore the timezone from before this object was created.
 *
 * This function makes sure the current timezone is reverted to
 * what it was before you created it.
 */
safe_timezone::~safe_timezone()
{
    f_locale_plugin->set_current_timezone(f_old_timezone);
}





/** \brief Initialize the locale plugin.
 *
 * This function is used to initialize the locale plugin object.
 */
locale::locale()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the locale plugin.
 *
 * Ensure the locale object is clean before it is gone.
 */
locale::~locale()
{
}


/** \brief Get a pointer to the locale plugin.
 *
 * This function returns an instance pointer to the locale plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the locale plugin.
 */
locale * locale::instance()
{
    return g_plugin_locale_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString locale::settings_path() const
{
    return "/admin/settings/locale";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString locale::icon() const
{
    return "/images/locale/locale-logo-64x64.png";
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString locale::description() const
{
    return "Define base locale functions to be used throughout all the"
        " plugins. It handles time and date, timezone, numbers, currency,"
        " etc.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString locale::dependencies() const
{
    return "|server|content|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t locale::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize the locale.
 *
 * This function terminates the initialization of the locale plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void locale::bootstrap(snap_child * snap)
{
    f_snap = snap;
}


/** \brief Return the list of available locales.
 *
 * This function gets all the available locales from the Locale class
 * and returns them in an array of locale information.
 *
 * \return A vector of locale_info_t structures.
 */
locale::locale_list_t const & locale::get_locale_list()
{
    if(f_locale_list.isEmpty())
    {
        // we use the DateFormat to retrieve the list of locales;
        // it is most likely the same or very similar list in all
        // the various objects offering a getAvailableLocales()
        // function... (TBD)
        int32_t count;
        const Locale *l(DateFormat::getAvailableLocales(count));
        for(int32_t i(0); i < count; ++i)
        {
            locale_info_t info;

            // Language
            {
                info.f_abbreviations.f_language = QString::fromLatin1(l[i].getLanguage());
                QUnicodeString lang;
                l[i].getDisplayLanguage(lang);
                info.f_display_names.f_language = lang;
            }

            // Variant
            {
                info.f_abbreviations.f_variant = QString::fromLatin1(l[i].getVariant());
                QUnicodeString variant;
                l[i].getDisplayVariant(variant);
                info.f_display_names.f_variant = variant;
            }

            // Country
            {
                info.f_abbreviations.f_country = QString::fromLatin1(l[i].getCountry());
                QUnicodeString country;
                l[i].getDisplayVariant(country);
                info.f_display_names.f_country = country;
            }

            // Script
            {
                info.f_abbreviations.f_script = QString::fromLatin1(l[i].getScript());
                QUnicodeString script;
                l[i].getDisplayVariant(script);
                info.f_display_names.f_script = script;
            }

            info.f_locale = QString::fromLatin1(l[i].getName());

            f_locale_list.push_back(info);
        }
    }

    return f_locale_list;
}


/** \brief Return the list of available timezones.
 *
 * We use the ICU which seems to be the best C/C++ library that
 * offers timezone and many other "Unicode" functionality.
 *
 * Also, there is a zone.tab table, and on newer systems, a
 * zone1970.tab table, with the list of all the known timezones.
 *
 * \note
 * Possible graphical JavaScript library for a graphical timezone picker
 * https://github.com/dosx/timezone-picker
 *
 * \note
 * A reference of the library ICU library can be found here:
 * /usr/include/x86_64-linux-gnu/unicode/timezone.h
 *
 * \note
 * The zone[1970].tab file is generally under /usr/share/zoneinfo
 * directory.
 *
 * \note
 * This function caches all the available timezones. So calling it multiple
 * times does not waste time.
 *
 * \return The list of timezone defined on this operating system.
 */
locale::locale::timezone_list_t const & locale::get_timezone_list()
{
    // read the file only if empty
    if(f_timezone_list.empty())
    {
        StringEnumeration * zone_list(TimeZone::createEnumeration());
        if(zone_list != nullptr)
        {
            for(;;)
            {
                // WARNING: you MUST initialize err otherwise
                //          unext() fails immediately
                UErrorCode err(U_ZERO_ERROR);
                UChar const * id(zone_list->unext(nullptr, err));
                if(id == nullptr)
                {
                    if(U_FAILURE(err))
                    {
                        ErrorCode err_code;
                        err_code.set(err);
                    }
                    break;
                }
                // TODO: The following "works great", only it does not
                //       really remove the equivalent we would want to
                //       remove; for example, it would keep Chile/EasterIsland
                //       instead of the more proper Pacific/Easter entry
                //       We may want to make use of the zone.tab file (see
                //       below) and then check against the ICU entries...
                //
                // skip equivalents, it will make the list shorter and
                // generally less confusing (i.e. both: Faroe / Faeroe)
                //UnicodeString const id_string(id);
                //UnicodeString const eq_string(TimeZone::getEquivalentID(id_string, 0));
                //if(id_string != eq_string)
                //{
                //    continue;
                //}

                QString const qid(QString::fromUtf16(id));
// TODO: add a command line one can use to list all timezones
//       (and also all locales)
//SNAP_LOG_WARNING("timezone = ")(qid);
                snap_string_list const id_segments(qid.split('/'));
                if(id_segments.size() == 2)
                {
                    timezone_info_t info;
                    info.f_timezone_name = qid;
                    info.f_continent = id_segments[0];
                    info.f_continent.replace('_', ' ');
                    info.f_city = id_segments[1];
                    info.f_city.replace('_', ' ');
                    f_timezone_list.push_back(info);
                }
            }
        }

#if 0
        // still empty?
        if(f_timezone_list.empty())
        {

// TODO: use the following as a fallback? (never tested!)
//       we could also have our own copy of the zone.tab file so if we
//       cannot find a zone file, we use our own (possibly outdated)
//       version...

            // new systems should have a zone1970.tab file instead of the
            // zone.tab; this new file is expected to have a slightly different
            // format; only it is not yet available under Ubuntu (including 14.10)
            // so we skip on that for now.
            //
            // IMPORTANT NOTE: this far, I have not see zone1970, it is new
            //                 as of June 2014 so very recent...
            QFile zone("/usr/share/zoneinfo/zone.tab");
            if(!zone.open(QIODevice::ReadOnly))
            {
                // should we just generate an error and return an empty list?
                throw snap_io_exception("server cannot find zone.tab file with timezone information.");
            }

            // load the file, each line is one entry
            // a line that starts with a '#' is ignored (i.e. comment)
            for(;;)
            {
                QByteArray raw_line(zone.readLine());
                if(zone.atEnd() && raw_line.isEmpty())
                {
                    break;
                }

                // skip comments
                if(raw_line.at(0) == '#')
                {
                    continue;
                }

                // get that in a string so we can split it
                QString const line(QString::fromUtf8(raw_line.data()));

                snap_string_list const line_segments(line.split('\t'));
                if(line_segments.size() < 3)
                {
                    continue;
                }
                timezone_info_t info;

                // 2 letter country name
                info.f_2country = line_segments[0];

                // position (lon/lat)
                QString const lon(line_segments[1].mid(0, 5));
                bool ok;
                info.f_longitude = lon.toInt(&ok, 10);
                if(!ok)
                {
                    info.f_longitude = -1;
                }
                QString const lat(line_segments[1].mid(5, 5));
                info.f_latitude = lat.toInt(&ok, 10);
                if(!ok)
                {
                    info.f_latitude = -1;
                }

                // the continent, country/state, city are separated by a slash
                info.f_timezone_name = line_segments[2];
                snap_string_list const names(info.f_timezone_name.split('/'));
                if(names.size() < 2)
                {
                    // invalid continent/state/city (TZ) entry
                    continue;
                }
                info.f_continent = names[0];
                info.f_continent.replace('_', ' ');
                if(names.size() == 3)
                {
                    info.f_country_or_state = names[1];
                    info.f_country_or_state.replace('_', ' ');
                    info.f_city = names[2];
                }
                else
                {
                    // no extra country/state name
                    info.f_city = names[1];
                }
                info.f_city.replace('_', ' ');

                // comment is optional, make sure it exists
                if(line_segments.size() >= 3)
                {
                    info.f_comment = line_segments[3];
                }

                f_timezone_list.push_back(info);
            }
        }
#endif
    }

    return f_timezone_list;
}


/** \brief Retrieve the currently setup locale.
 *
 * This function retrieves the current locale of this run.
 *
 * You may change the locale with a call to the set_locale() signal.
 *
 * If you are dealing with a date and/or time, you probably also want
 * to call the set_timezone() function.
 *
 * \return The current locale, may be empty.
 */
QString locale::get_current_locale() const
{
    return f_current_locale;
}


/** \brief Define the current locale.
 *
 * This function saves a new locale as the current locale.
 *
 * This function is semi-internal as it should only be called from
 * plugins that implement the set_locale() signal.
 *
 * \warning
 * This function does NOT setup the locale. Instead you MUST call the
 * set_locale() signal and plugins that respond to that signal call
 * the set_current_locale(). Once the signal is done, then and only
 * then is the system locale actually set.
 *
 * \param[in] new_locale  Change the locale.
 *
 * \sa set_locale()
 *
 */
void locale::set_current_locale(QString const & new_locale)
{
    f_current_locale = new_locale;
}


/** \brief Retrieve the currently setup timezone.
 *
 * This function retrieves the current timezone of this run.
 *
 * You may change the timezone with a call to the set_timezone() signal.
 *
 * If you are dealing with formatting a date and/or time, you probably
 * also want to call the set_locale() function.
 *
 * \return The current timezone, may be empty.
 *
 * \sa set_timezone()
 * \sa set_current_timezone()
 */
QString locale::get_current_timezone() const
{
    return f_current_timezone;
}


/** \brief Define the current timezone.
 *
 * This function saves a new timezone as the current timezone.
 *
 * This function is semi-internal as it should only be called from
 * plugins that implement the set_timezone() signal.
 *
 * \warning
 * This function does NOT setup the timezone. Instead you MUST call the
 * set_timezone() signal and plugins that respond to that signal call
 * the set_current_timezone(). Once the signal is done, then and only
 * then is the system timezone actually set.
 *
 * \param[in] new_timezone  Change the timezone.
 *
 * \sa set_timezone()
 */
void locale::set_current_timezone(QString const & new_timezone)
{
    f_current_timezone = new_timezone;
}


/** \brief Reset the locale current setup.
 *
 * This function should be called if the calls to the set_timezone()
 * or set_locale() may result in something different after a change
 * you made.
 *
 * For example, once a user is viewed as logged in, the
 * result changes since now we can make use of the user's data to
 * determine a timezone and locale.
 */
void locale::reset_locale()
{
    f_current_locale.clear();
    f_current_timezone.clear();
}


/** \brief Set the locale for this session.
 *
 * This function checks whether the current locale is already set. If so,
 * then the function returns false which means that we do not need to
 * run any additional signal.
 *
 * Otherwise the signal is sent to all the plugins and various plugins
 * may set the locale with the set_current_locale() function.
 *
 * \return true if the current locale is not yet set, false otherwise.
 */
bool locale::set_locale_impl()
{
    return f_current_locale.isEmpty();
}


/** \brief Set the default locale for this session.
 *
 * This function checks whether a current locale was set by the set_locale()
 * signal. If so, then it does nothing. Otherwise, it checks for the default
 * locale parameters and sets those up.
 *
 * The default locale is defined as:
 *
 * \li The user locale if the user defined such.
 * \li The website locale if the website defined such.
 * \li The internal Snap default locale (i.e. left as is).
 */
void locale::set_locale_done()
{
    if(f_current_locale.isEmpty())
    {
        // no other plugin setup the current locale, check out the
        // global defaults for this website; it should always be
        // defined
        //
        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(name_t::SNAP_NAME_LOCALE_SETTINGS_PATH));
        content::content * content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));
        QString const locale_name(revision_row->cell(get_name(name_t::SNAP_NAME_LOCALE_SETTINGS_LOCALE))->value().stringValue());
        set_current_locale(locale_name);
    }

    // if the timezone was not defined, it is an empty string which is
    // exactly what we want to pass to the child set_timezone() function
SNAP_LOG_TRACE("*** Set locale_settings::locale [")(f_current_locale)("]");
    f_snap->set_locale(f_current_locale);
}


/** \brief Setup the timzone as required.
 *
 * This function checks whether the timezone is already set for this
 * session. If it, then it returns false and no signal is sent to the
 * other plugins.
 *
 * It is possible to reset the timezone with a call to the
 * logged_in_user_ready() function. This automatically happens
 * when a user is found to be logged in.
 *
 * \return true if the current timezone is not yet defined.
 */
bool locale::set_timezone_impl()
{
    return f_current_timezone.isEmpty();
}


/** \brief Finish up with the timezone setup.
 *
 * This function is called last, after all the other plugin set_timezone()
 * signals were called. If the current timezone is still undefined when
 * this function is called, then the locale plugin defines the default
 * timezone.
 *
 * The default timezone is:
 *
 * \li The timezone of the currently logged in user if one is defined;
 * The functions checks for a SNAP_NAME_LOCALE_TIMEZONE field in the user's
 * current revision content;
 * \li The timezone of the website if one is defined under
 * admin/locale/timezone as a field named SNAP_NAME_LOCALE_TIMEZONE
 * found in the current revision of that page.
 */
void locale::set_timezone_done()
{
    if(f_current_timezone.isEmpty())
    {
        // check for a locale
        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(name_t::SNAP_NAME_LOCALE_SETTINGS_PATH));
        content::content * content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));
        QString const timezone_name(revision_row->cell(get_name(name_t::SNAP_NAME_LOCALE_SETTINGS_TIMEZONE))->value().stringValue());
        set_current_timezone(timezone_name);
    }

    // if the timezone was not defined, it is an empty string which is
    // exactly what we want to pass to the child set_timezone() function
SNAP_LOG_TRACE("*** Set locale_settings::timezone [")(f_current_timezone)("]");
    f_snap->set_timezone(f_current_timezone);
}


/** \brief Convert the specified date and time to a string date.
 *
 * This function makes use of the locale to define the resulting
 * formatted date.
 *
 * The time is in seconds. The time itself is ignored except if
 * it has an effect on the date (i.e. leap year.)
 *
 * \todo
 * Save the DateFormat so if the function is called multiple times,
 * we do not have to re-create it.
 *
 * \todo
 * Allow for milliseconds (or even microseconds) as input to be
 * more compatible with other Snap! functions.
 *
 * \param[in] d  The time to be presented to the end user in seconds.
 *
 * \return A string formatted as per the locale.
 */
QString locale::format_date(time_t d)
{
    QUnicodeString const timezone_id(f_current_timezone);
    LocalPointer<TimeZone> tz(TimeZone::createTimeZone(timezone_id)); // TODO: verify that it took properly
    Locale const l(f_current_locale.toUtf8().data()); // TODO: verify that it took properly
    LocalPointer<DateFormat> dt(DateFormat::createDateInstance(DateFormat::kDefault, l));
    dt->setTimeZone(*tz);
    UDate const udate(d * 1000LL);
    QUnicodeString u;
    dt->format(udate, u);
    return u;
}


/** \brief Convert the specified date and time to a string time.
 *
 * This function makes use of the locale to define the resulting
 * formatted time.
 *
 * The time is in seconds. The date itself is ignored (i.e. only
 * the value modulo 86400 is used). The ICU library does not support
 * leap seconds here (TBD).
 *
 * \todo
 * Save the DateFormat so if the function is called multiple times,
 * we do not have to re-create it.
 *
 * \param[in] d  The time to be presented to the end user.
 *
 * \return A string formatted as per the locale.
 */
QString locale::format_time(time_t d)
{
    QUnicodeString const timezone_id(f_current_timezone);
    LocalPointer<TimeZone> tz(TimeZone::createTimeZone(timezone_id)); // TODO: verify that it took properly
    Locale const l(f_current_locale.toUtf8().data()); // TODO: verify that it took properly
    LocalPointer<DateFormat> dt(DateFormat::createTimeInstance(DateFormat::kDefault, l));
    dt->setTimeZone(*tz);
    UDate const udate(d * 1000LL);
    QUnicodeString u;
    dt->format(udate, u);
    return u;
}


/** \brief Format a Unix date.
 *
 * This function uses the date_format string to format a unix date and
 * time in a string.
 *
 * If the date_format parameter is empty, then the default locale date
 * format is used. In that case, the string will not include the time.
 * You may also call the format_date() function directly if you know
 * that date_format is always going to be empty, however, this function
 * will call set_timezone() and set_locale() for you, whereas, the
 * plain format_date(d) function does not.
 *
 * \warning
 * This function calls the set_timezone() and may call the set_locale()
 * functions setting up the timezone and locale of the plugin.
 *
 * \param[in] d  The Unix time to convert.
 * \param[in] date_format  The format to use as defined in strftime().
 * \param[in] use_local  Whether to use the localtime() or gmtime() function.
 *     (WARNING: use_local is ignored when date_format is an empty string)
 *
 * \return The formatted date.
 *
 * \sa format_date()
 * \sa format_time()
 */
QString locale::format_date(time_t d, QString const & date_format, bool use_local)
{
    // prepare outselves if not yet ready...
    //
    set_timezone();

    if(date_format.isEmpty())
    {
        set_locale();
        return format_date(d);
    }

    // TODO: I think we can use a format string with the ICU library
    //       but right now I am copy/paste-ing some code...
    //
    struct tm t;
    if(use_local)
    {
        localtime_r(&d, &t);
    }
    else
    {
        gmtime_r(&d, &t);
    }

    char buf[256];
    strftime(buf, sizeof(buf), date_format.toUtf8().data(), &t);
    buf[sizeof(buf) / sizeof(buf[0]) - 1] = '\0'; // make sure there is a NUL
    return QString::fromUtf8(buf);
}


/** \brief Parse a date and return its Unix time representation.
 *
 * This function parses a date and returns the representation of that date
 * in a Unix time representation (i.e. milliseconds are lost.)
 *
 * The function is lenient, meaning that an input string that can be
 * parsed in a valid date is parsed as such, always.
 *
 * \note
 * The function returns -1 as the time and sets errcode to an error
 * number other than PARSE_NO_ERROR if the input string cannot be parsed
 * into what is considered a valid date.
 *
 * \warning
 * This function fails if your input string includes a date and a time.
 * Only the date gets parsed and then an error is generated.
 *
 * \param[in] date  The string representing a date.
 * \param[out] errcode  An error code or PARSE_NO_ERROR on return.
 *
 * \return The time the string represented.
 */
time_t locale::parse_date(QString const & date, parse_error_t & errcode)
{
    enum class round_t
    {
        ROUND_NO,
        ROUND_UP,
        ROUND_DOWN,
        ROUND_ROUND
    };

    errcode = parse_error_t::PARSE_NO_ERROR;

    if(date.startsWith("now"))
    {
        // in this case we use our own special code
        time_t const now(f_snap->get_start_time());
        struct tm result;
        gmtime_r(&now, &result);
        QString const d(date.mid(3));
        QChar const * s(d.unicode());
        for(;;)
        {
            while(s->isSpace())
            {
                ++s;
            }
            if(s->unicode() == L'\0')
            {
                return mkgmtime(&result);
            }
            int64_t sign(1LL);
            round_t round(round_t::ROUND_NO);
            if(s->unicode() == '+')
            {
                ++s;
            }
            else if(s->unicode() == '-')
            {
                ++s;
                sign = -1LL;
            }
            else if((s[0].unicode() == 'r' || s[0].unicode() == 'R')
                 && (s[1].unicode() == 'o' || s[1].unicode() == 'O')
                 && (s[2].unicode() == 'u' || s[2].unicode() == 'U')
                 && (s[3].unicode() == 'n' || s[3].unicode() == 'N')
                 && (s[4].unicode() == 'd' || s[4].unicode() == 'D'))
            {
                s += 5;
                if((s[0].unicode() == 'e' || s[0].unicode() == 'E')
                && (s[1].unicode() == 'd' || s[1].unicode() == 'D'))
                {
                    // allow for "rounded" instead of just "round"
                    s += 2;
                }
                while(s->isSpace())
                {
                    ++s;
                }
                if((s[0].unicode() == 'u' || s[0].unicode() == 'U')
                && (s[1].unicode() == 'p' || s[1].unicode() == 'P'))
                {
                    round = round_t::ROUND_UP;
                    s += 2;
                }
                else if((s[0].unicode() == 'd' || s[0].unicode() == 'D')
                     && (s[1].unicode() == 'o' || s[1].unicode() == 'O')
                     && (s[2].unicode() == 'w' || s[2].unicode() == 'W')
                     && (s[3].unicode() == 'n' || s[3].unicode() == 'N'))
                {
                    round = round_t::ROUND_DOWN;
                    s += 4;
                }
                else if((s[0].unicode() >= '0' && s[0].unicode() <= '9')
                     || (s[0].unicode() == 't' || s[0].unicode() == 'T'))
                {
                    // if UP or DOWN are not specified, then we assume
                    // ROUND instead (UP if value is 50% or more of count)
                    round = round_t::ROUND_ROUND;
                }
                else
                {
                    errcode = parse_error_t::PARSE_ERROR_DATE;
                    return -1;
                }
                while(s->isSpace())
                {
                    ++s;
                }
                if((s[0].unicode() == 't' || s[0].unicode() == 'T')
                && (s[1].unicode() == 'o' || s[1].unicode() == 'o'))
                {
                    // allow for "rounded" instead of just "round"
                    s += 2;
                }
            }

            while(s->isSpace())
            {
                ++s;
            }

            int64_t count(0LL);
            for(;;)
            {
                int digit(s->digitValue());
                if(digit < 0)
                {
                    break;
                }
                if(count > 1000)
                {
                    errcode = parse_error_t::PARSE_ERROR_OVERFLOW;
                    return -1;
                }
                count = count * 10 + digit;
                ++s;
            }
            if(count == 0)
            {
                errcode = parse_error_t::PARSE_ERROR_UNDERFLOW;
                return -1;
            }
            count *= sign;

            while(s->isSpace())
            {
                ++s;
            }

            if(count == 0
            && round != round_t::ROUND_NO)
            {
                // TBD: should we be returning -1 and set an error?
                //      it seems to me that in this case the programmer
                //      wants to know immediately that he has a bug
                throw locale_exception_invalid_argument("count cannot be zero with rounding capability");
            }

            // here we are interested in the following word
            // unfortunately QChar does not offer a way to compare
            // a word as is and we do not know the length...
            bool use_seconds(false);
            if((s[0].unicode() == 's' || s[0].unicode() == 'S')
            && (s[1].unicode() == 'e' || s[1].unicode() == 'E')
            && (s[2].unicode() == 'c' || s[2].unicode() == 'C')
            && (s[3].unicode() == 'o' || s[3].unicode() == 'O')
            && (s[4].unicode() == 'n' || s[4].unicode() == 'N')
            && (s[5].unicode() == 'd' || s[5].unicode() == 'D'))
            {
                s += 6;
                use_seconds = true;
            }
            else if((s[0].unicode() == 'm' || s[0].unicode() == 'M')
                 && (s[1].unicode() == 'i' || s[1].unicode() == 'I')
                 && (s[2].unicode() == 'n' || s[2].unicode() == 'N')
                 && (s[3].unicode() == 'u' || s[3].unicode() == 'U')
                 && (s[4].unicode() == 't' || s[4].unicode() == 'T')
                 && (s[5].unicode() == 'e' || s[5].unicode() == 'E'))
            {
                s += 6;
                count *= 60LL;
                use_seconds = true;
            }
            else if((s[0].unicode() == 'h' || s[0].unicode() == 'H')
                 && (s[1].unicode() == 'o' || s[1].unicode() == 'O')
                 && (s[2].unicode() == 'u' || s[2].unicode() == 'U')
                 && (s[3].unicode() == 'r' || s[3].unicode() == 'R'))
            {
                s += 4;
                count *= 3600LL;
                use_seconds = true;
            }
            else if((s[0].unicode() == 'd' || s[0].unicode() == 'D')
                 && (s[1].unicode() == 'a' || s[1].unicode() == 'A')
                 && (s[2].unicode() == 'y' || s[2].unicode() == 'Y'))
            {
                s += 3;
                count *= 86400LL;
                use_seconds = true;
            }
            else if((s[0].unicode() == 'm' || s[0].unicode() == 'M')
                 && (s[1].unicode() == 'o' || s[1].unicode() == 'O')
                 && (s[2].unicode() == 'n' || s[2].unicode() == 'N')
                 && (s[3].unicode() == 't' || s[3].unicode() == 'T')
                 && (s[4].unicode() == 'h' || s[4].unicode() == 'H'))
            {
                s += 5;
                round_t r(round);
                if(r == round_t::ROUND_ROUND)
                {
                    time_t const seconds((result.tm_mday - 1) * 86400
                                        + result.tm_hour * 3600
                                        + result.tm_min * 60
                                        + result.tm_sec);
                    time_t const total_seconds(f_snap->last_day_of_month(result.tm_mon + 1, result.tm_year + 1900));
                    if(seconds >= total_seconds / 2)
                    {
                        r = round_t::ROUND_UP;
                    }
                    else
                    {
                        r = round_t::ROUND_DOWN;
                    }
                }
                switch(r)
                {
                case round_t::ROUND_NO:
                    {
                        // here count may be negative
                        result.tm_mon += count;

                        // adjust the result fields to sensible values
                        time_t const seconds(mkgmtime(&result));
                        gmtime_r(&seconds, &result);
                    }
                    break;

                case round_t::ROUND_DOWN:
                    {
                        // MMM 1, YYYY 00:00:00
                        result.tm_sec = 0;
                        result.tm_min = 0;
                        result.tm_hour = 0;
                        result.tm_mday = 1;
                        result.tm_mon -= count - 1;

                        // adjust the result fields to sensible values
                        time_t const seconds(mkgmtime(&result));
                        gmtime_r(&seconds, &result);
                    }
                    break;

                case round_t::ROUND_UP:
                    // round down and then go to next X seconds, days, months...
                    // minus 1 second (so we clamp at the end of that X seconds,
                    // days, months...)
                    {
                        // In this case we do (month + count - 1 second)

                        // MMM 28/29/30/31, YYYY 23:59:59
                        result.tm_sec = -1;
                        result.tm_min = 0;
                        result.tm_hour = 0;
                        result.tm_mday = 1;
                        result.tm_mon += count;

                        // adjust the month
                        time_t const seconds(mkgmtime(&result));
                        gmtime_r(&seconds, &result);
                    }
                    break;

                case round_t::ROUND_ROUND:
                    throw snap_logic_exception("round_t::ROUND_ROUND cannot happen in this switch. It was replaced by round up or down in the preceeding if() block.");

                }
            }
            else if((s[0].unicode() == 'y' || s[0].unicode() == 'Y')
                 && (s[1].unicode() == 'e' || s[1].unicode() == 'E')
                 && (s[2].unicode() == 'a' || s[2].unicode() == 'A')
                 && (s[3].unicode() == 'r' || s[3].unicode() == 'R'))
            {
                s += 4;
                round_t r(round);
                if(r == round_t::ROUND_ROUND)
                {
                    if(result.tm_mon >= 6) // Jul-Dec, round UP
                    {
                        r = round_t::ROUND_UP;
                    }
                    else // Jan-Jun, round DOWN
                    {
                        r = round_t::ROUND_DOWN;
                    }
                }
                switch(r)
                {
                case round_t::ROUND_NO:
                    // here count may be negative
                    result.tm_year += count;
                    break;

                case round_t::ROUND_DOWN:
                    {
                        // Jan 1, YYYY 00:00:00
                        result.tm_sec = 0;
                        result.tm_min = 0;
                        result.tm_hour = 0;
                        result.tm_mday = 1;
                        result.tm_mon = 0;
                        result.tm_year += count - 1;
                    }
                    break;

                case round_t::ROUND_UP:
                    // round down and then go to next X seconds, days, months...
                    // minus 1 second (so we clamp at the end of that X seconds,
                    // days, months...)
                    {
                        // Dec 31, YYYY 23:59:59
                        result.tm_sec = 59;
                        result.tm_min = 59;
                        result.tm_hour = 23;
                        result.tm_mday = 31; // Dec is always 31 days
                        result.tm_mon = 11;
                        result.tm_year += count - 1;
                    }
                    break;

                case round_t::ROUND_ROUND:
                    throw snap_logic_exception("round_t::ROUND_ROUND cannot happen in this switch. It was replaced by round up or down in the preceeding if() block.");

                }
            }
            else
            {
                errcode = parse_error_t::PARSE_ERROR_DATE;
                return -1;
            }

            if(use_seconds)
            {
                switch(round)
                {
                case round_t::ROUND_NO:
                    // here count may be negative
                    result.tm_sec += count;
                    break;

                case round_t::ROUND_DOWN:
                    {
                        time_t seconds(mkgmtime(&result));
                        seconds -= seconds % count;
                        gmtime_r(&seconds, &result);
                    }
                    break;

                case round_t::ROUND_UP:
                    // round down and then go to next X seconds, days, months...
                    // minus 1 second (so we clamp at the end of that X seconds,
                    // days, months...)
                    {
                        time_t seconds(mkgmtime(&result));
                        seconds = seconds - seconds % count + count - 1;
                        gmtime_r(&seconds, &result);
                    }
                    break;

                case round_t::ROUND_ROUND:
                    // TBD: should it be  (count + 1) / 2 ?
                    {
                        time_t seconds(mkgmtime(&result));
                        if(seconds % count >= count / 2)
                        {
                            // round up, fully (i.e. no -1...)
                            seconds = seconds - seconds % count + count - 1;
                        }
                        else
                        {
                            // round down
                            seconds -= seconds % count;
                        }
                        gmtime_r(&seconds, &result);
                    }
                    break;

                }
            }

            // skip the plurial if defined
            if(s->unicode() == 's' || s->unicode() == 'S')
            {
                ++s;
            }
        }
        NOTREACHED();
    }
    else
    {
        Locale const l(f_current_locale.toUtf8().data()); // TODO: verify that it took properly
        LocalPointer<DateFormat> dt(DateFormat::createDateInstance(DateFormat::kDefault, l));

        LocalPointer<TimeZone> tz(TimeZone::createTimeZone(QUnicodeString(f_current_timezone))); // TODO: verify that it took properly
        dt->setTimeZone(*tz);

        QUnicodeString const date_format(date);
        ParsePosition pos;
        UDate const result(dt->parse(date_format, pos));

        if(pos.getIndex() != date_format.length())
        {
            // it failed (we always expect the entire string to be parsed)
            //
            // TODO: ameliorate the error code with the error code that
            //       the DateFormat generates
            //
            errcode = parse_error_t::PARSE_ERROR_DATE;
            return static_cast<time_t>(-1);
        }

        // UDate is a double in milliseconds
        // TODO: should we round the number up to one second?
        //       (we do not really expect dates with such precision
        //       at this point?)
        //
        return static_cast<time_t>(result / 1000.0);
    }
}


/** \brief Parse a time and return its Unix time representation.
 *
 * This function parses a time and returns the representation of that time
 * in a Unix time representation (i.e. milliseconds are lost.)
 *
 * The function is lenient, meaning that an input string that can be
 * parsed in a valid time is parsed as such, always.
 *
 * \note
 * At this time the ICU time parser does not work for us. It may be a
 * small problem that we could resolve in some way, but for now we
 * have our own parser. We support times defined as:
 *
 * \code
 *      HH:MM[:SS] [AM/PM]
 * \endcode
 *
 * \par
 * In other words a positive decimal number representing the hour. Note
 * that it may be just one digit. If AM or PM are used, then the number
 * must be between 1 and 12 inclusive. Otherwise it has to be between
 * 0 and 23.
 *
 * \par
 * The minutes are also mandatory and is a positive decimal number. Note
 * that it may be just one digit. Minutes are limited to a number between
 * 0 and 59 inclusive.
 *
 * \par
 * The seconds are optional, although if a colon is specified, it becomes
 * mandatory. Note that it may be just one digit. Minutes are limited to
 * a number between 0 and 60 inclusive.
 *
 * \par
 * The AM or PM may appear right after the minute or second (no space
 * required). It may be in lower or upper case. (aM and pM is always
 * acceptable.) The hour is limited to a number from 1 to 12 when
 * AM/PM is used.
 *
 * \note
 * The function returns -1 as the time and sets errcode to an error
 * number other than PARSE_NO_ERROR if the input string cannot be parsed
 * into what is considered a valid time.
 *
 * \warning
 * This function fails if your input string includes a date and a time.
 * Only the time gets parsed and then an error is generated.
 *
 * \param[in] time_str  The string representing a time.
 * \param[out] errcode  An error code or PARSE_NO_ERROR on return.
 *
 * \return The time the string represented.
 */
time_t locale::parse_time(QString const & time_str, parse_error_t & errcode)
{
    errcode = parse_error_t::PARSE_NO_ERROR;

    if(time_str.startsWith("now"))
    {
        // in this case we use our own special code
        time_t result(f_snap->get_start_time());
        QString d(time_str.mid(3));
        QChar const *s(d.unicode());
        for(;;)
        {
            while(s->isSpace())
            {
                ++s;
            }
            if(s->unicode() == L'\0')
            {
                // we are only interested in time so we shorten the input
                // to a time only and lose days, months, years
                return result % 86400;
            }
            int64_t sign(1LL);
            if(s->unicode() == '+')
            {
                ++s;
            }
            else if(s->unicode() == '-')
            {
                ++s;
                sign = -1LL;
            }

            while(s->isSpace())
            {
                ++s;
            }

            int64_t count(0LL);
            for(;;)
            {
                int const digit(s->digitValue());
                if(digit < 0)
                {
                    break;
                }
                if(count > 1000)
                {
                    errcode = parse_error_t::PARSE_ERROR_OVERFLOW;
                    return -1;
                }
                count = count * 10 + digit;
                ++s;
            }
            if(count == 0)
            {
                errcode = parse_error_t::PARSE_ERROR_UNDERFLOW;
                return -1;
            }
            count *= sign;

            while(s->isSpace())
            {
                ++s;
            }

            // here we are interested in the following word
            // unfortunately QChar does not offer a way to compare
            // a word as is and we do not know the length...
            if((s[0].unicode() == 's' || s[0].unicode() == 'S')
            && (s[1].unicode() == 'e' || s[1].unicode() == 'E')
            && (s[2].unicode() == 'c' || s[2].unicode() == 'C')
            && (s[3].unicode() == 'o' || s[3].unicode() == 'O')
            && (s[4].unicode() == 'n' || s[4].unicode() == 'N')
            && (s[5].unicode() == 'd' || s[5].unicode() == 'D'))
            {
                s += 6;
                result += count;
            }
            else if((s[0].unicode() == 'm' || s[0].unicode() == 'M')
                 && (s[1].unicode() == 'i' || s[1].unicode() == 'I')
                 && (s[2].unicode() == 'n' || s[2].unicode() == 'N')
                 && (s[3].unicode() == 'u' || s[3].unicode() == 'U')
                 && (s[4].unicode() == 't' || s[4].unicode() == 'T')
                 && (s[5].unicode() == 'e' || s[5].unicode() == 'E'))
            {
                s += 6;
                result += count * 60LL;
            }
            else if((s[0].unicode() == 'h' || s[0].unicode() == 'H')
                 && (s[1].unicode() == 'o' || s[1].unicode() == 'O')
                 && (s[2].unicode() == 'u' || s[2].unicode() == 'U')
                 && (s[3].unicode() == 'r' || s[3].unicode() == 'R'))
            {
                s += 4;
                result += count * 3600LL;
            }
            else
            {
                errcode = parse_error_t::PARSE_ERROR_DATE;
                return -1;
            }

            // skip the plurial if defined
            if(s->unicode() == 's' || s->unicode() == 'S')
            {
                ++s;
            }
        }
        NOTREACHED();
    }
    else
    {
        // Somehow the time parser always gives me an error
        // Until we can figure out what the heck is happening, I have my own
        // parser that supports the fairly worldwide standard
        //          "HH:MM[:SS] [AM/PM]"
        //
//        Locale const l(f_current_locale.toUtf8().data()); // TODO: verify that it took properly
//        LocalPointer<DateFormat> dt(DateFormat::createTimeInstance(DateFormat::kDefault, l));
//
//        LocalPointer<TimeZone> tz(TimeZone::createTimeZone(QUnicodeString(f_current_timezone))); // TODO: verify that it took properly
//        dt->setTimeZone(*tz);
//
//        QUnicodeString const date_format(time_str);
//        ParsePosition pos;
//        UDate const result(dt->parse(date_format, pos));
//
//std::cerr << "*** " << f_current_locale << " time parsed " << time_str << " -> pos " << pos.getIndex() << ", length " << date_format.length() <<  " -> " << result << "\n";
//        if(pos.getIndex() != date_format.length())
//        {
//            // it failed (we always expect the entire string to be parsed)
//            //
//            // TODO: ameliorate the error code with the error code that
//            //       the DateFormat generates
//            //
//            errcode = parse_error_t::PARSE_ERROR_DATE;
//            return static_cast<time_t>(-1);
//        }
//
//        // UDate is a double in milliseconds
//        return static_cast<time_t>(result / 1000LL);

        // our own time reader with format HH:MM[:SS] [AM/PM]
        //
        // we allow any type of spaces at the start and between the
        // time and AM/PM
        //
        time_t result(0);
        QChar const *s(time_str.unicode());

        // skip spaces at the start
        while(s->isSpace())
        {
            ++s;
        }

        // H or HH
        int hour(s->digitValue());
        {
            if(hour < 0)
            {
                errcode = parse_error_t::PARSE_ERROR_DATE;
                return static_cast<time_t>(-1);
            }
            ++s;
            int const digit2(s->digitValue());
            if(digit2 >= 0)
            {
                hour = hour * 10 + digit2;
                ++s;
            }
        }

        // hours and minutes must be separated by a colon
        if(s->unicode() != L':')
        {
            errcode = parse_error_t::PARSE_ERROR_DATE;
            return static_cast<time_t>(-1);
        }
        ++s;

        // M or MM
        {
            int minute(s->digitValue());
            if(minute < 0)
            {
                errcode = parse_error_t::PARSE_ERROR_DATE;
                return static_cast<time_t>(-1);
            }
            ++s;
            int const digit2(s->digitValue());
            if(digit2 >= 0)
            {
                minute = minute * 10 + digit2;
                ++s;
            }
            if(minute > 59)
            {
                errcode = parse_error_t::PARSE_ERROR_OVERFLOW;
                return static_cast<time_t>(-1);
            }
            result += minute * 60;
        }

        // if we have a colon there are seconds
        if(s->unicode() == L':')
        {
            int second(s->digitValue());
            if(second < 0)
            {
                errcode = parse_error_t::PARSE_ERROR_DATE;
                return static_cast<time_t>(-1);
            }
            ++s;
            int const digit2(s->digitValue());
            if(digit2 >= 0)
            {
                second = second * 10 + digit2;
                ++s;
            }
            if(second > 60) // allow adjustment second
            {
                errcode = parse_error_t::PARSE_ERROR_OVERFLOW;
                return static_cast<time_t>(-1);
            }
            result += second;
        }

        // skip spaces after the time (optional)
        while(s->isSpace())
        {
            ++s;
        }

        // see whether we have AM/PM
        int mode(0);
        if(!s->isNull())
        {
            if(s->toLower().unicode() == 'a'
            && !s[1].isNull()
            && s[1].toLower().unicode() == 'm')
            {
                mode = 1;
                s += 2;
            }
            else if(s->toLower().unicode() == 'p'
                 && !s[1].isNull()
                 && s[1].toLower().unicode() == 'm')
            {
                mode = 2;
                s += 2;
            }
            else
            {
                // followed by something other than AM or PM
                errcode = parse_error_t::PARSE_ERROR_DATE;
                return static_cast<time_t>(-1);
            }

            // skip spaces after the AM/PM
            while(s->isSpace())
            {
                ++s;
            }

            if(!s->isNull())
            {
                // AM/PM followed by something
                errcode = parse_error_t::PARSE_ERROR_DATE;
                return static_cast<time_t>(-1);
            }
        }

        switch(mode)
        {
        case 0:
            if(hour > 23)
            {
                errcode = parse_error_t::PARSE_ERROR_OVERFLOW;
                return static_cast<time_t>(-1);
            }
            break;

        case 1:
        case 2:
            if(hour < 1)
            {
                errcode = parse_error_t::PARSE_ERROR_UNDERFLOW;
                return static_cast<time_t>(-1);
            }
            if(hour > 12)
            {
                errcode = parse_error_t::PARSE_ERROR_OVERFLOW;
                return static_cast<time_t>(-1);
            }
            // in case of AM hour is used as is
            if(mode == 2)
            {
                if(hour == 12)
                {
                    hour = 0;
                }
                else
                {
                    hour += 12;
                }
            }
            break;

        default:
            throw snap_logic_exception("locale.cpp:locale::parse_time(): hour mode unexpected.");

        }

        return result + hour * 3600LL;
    }
}


//
// A reference of the library ICU library can be found here:
// /usr/include/x86_64-linux-gnu/unicode/timezone.h
// file:///usr/share/doc/icu-doc/html/index.html
//
// Many territory details by Unicode.org
// http://unicode.org/repos/cldr/trunk/common/supplemental/supplementalData.xml
//

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
