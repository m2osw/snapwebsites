// Snap Websites Server -- snap websites serving children
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

#include "snapwebsites/snap_child.h"

#include "snapwebsites/compression.h"
#include "snapwebsites/http_strings.h"
#include "snapwebsites/log.h"
#include "snapwebsites/mail_exchanger.h"
#include "snapwebsites/mkgmtime.h"
#include "snapwebsites/not_used.h"
#include "snapwebsites/qdomhelpers.h"
#include "snapwebsites/qlockfile.h"
#include "snapwebsites/snap_image.h"
#include "snapwebsites/snap_utf8.h"
#include "snapwebsites/snapwebsites.h"
#include "snapwebsites/snap_lock.h"
#include "snapwebsites/snap_magic.h"

#include <libdbproxy/exception.h>
#include <QtSerialization/QSerialization.h>
#include <libtld/tld.h>

#include <sstream>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <wait.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

#include <QDirIterator>

#include "snapwebsites/poison.h"


namespace snap
{




/** \class snap_child
 * \brief Child process class.
 *
 * This class handles child objects that process queries from the Snap
 * CGI tool.
 *
 * The children appear in the Snap Server and themselves. The server is
 * the parent that handles the lifetime of the child. The parent also
 * holds the child process identifier and it waits on the child for its
 * death.
 *
 * The child itself has its f_child_pid set to zero.
 *
 * Some of the functions will react with an error if called from the
 * wrong process (i.e. parent calling a child process function and vice
 * versa.)
 */

/** \fn int64_t get_start_date() const
 * \brief Retrieve the date when the child process started.
 *
 * This function returns the date, in micro seconds (seconds x 1,000,000)
 * when the child was forked from the server.
 *
 * In some situation, it may be useful to reset this time to the clock.
 * In most cases this is done in backends. This is done by calling
 * init_start_date().
 *
 * \sa init_start_date()
 * \sa get_start_time()
 */

/** \fn time_t get_start_time() const
 * \brief Retrieve the date when the child process started in seconds.
 *
 * This function returns the date in seconds (same as a Unix date)
 * when the child was forked from the server.
 *
 * This is the same date as get_start_date() would returned, divided
 * by 1 million (rounded down).
 *
 * \sa get_start_date()
 */

namespace
{


/** \brief Retrieve the current thread identifier.
 *
 * This function retrieves the current thread identifier.
 *
 * \return The thread identifier, which is a pid_t specific to each thread
 *         of a process.
 */
pid_t gettid()
{
    return syscall(SYS_gettid);
}


// list of plugins that we cannot do without
char const * g_minimum_plugins[] =
{
    "attachment",
    "content",
    "editor",
    "filter",
    "form", // this one will be removed completely (phased out)
    "info",
    "javascript",
    "layout",
    "links",
    "list",
    "listener",
    "locale",
    "menu",
    "messages",
    "mimetype", // this should not be required
    "output",
    "password",
    "path",
    "permissions",
    "sendmail",
    "server_access",
    "sessions",
    "taxonomy",
    "users",
    "users_ui"
};

char const * g_week_day_name[] =
{
    "Sunday", "Monday", "Tuesday", "Wedneday", "Thursday", "Friday", "Saturday"
};
int const g_week_day_length[] = { 6, 6, 7, 8, 8, 6, 8 }; // strlen() of g_week_day_name's
char const * g_month_name[] =
{
    "January", "February", "Marsh", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
int const g_month_length[] = { 7, 8, 5, 5, 3, 4, 4, 6, 9, 7, 8, 8 }; // strlen() of g_month_name
int const g_month_days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
signed char const g_timezone_adjust[26] =
{
    /* A */ -1,
    /* B */ -2,
    /* C */ -3,
    /* D */ -4,
    /* E */ -5,
    /* F */ -6,
    /* G */ -7,
    /* H */ -8,
    /* I */ -9,
    /* J */ 0, // not used
    /* K */ -10,
    /* L */ -11,
    /* M */ -12,
    /* N */ 1,
    /* O */ 2,
    /* P */ 3,
    /* Q */ 4,
    /* R */ 5,
    /* S */ 6,
    /* T */ 7,
    /* U */ 8,
    /* V */ 9,
    /* W */ 10,
    /* X */ 11,
    /* Y */ 12,
    /* Z */ 0, // Zulu time is zero
};



// valid language names
// a full language definition is xx_YY where xx is the two letter name
// of a language and YY is a two letter name of a country; we also support
// 3 letter language names, and full country names (for now)
snap_child::language_name_t const g_language_names[] =
{
    {
        "Abkhaz",
        u8"\u0430\u04A7\u0441\u0443\u0430 \u0431\u044B\u0437\u0448\u04D9\u0430, \u0430\u04A7\u0441\u0448\u04D9\u0430",
        { 'a', 'b', '\0' },
        ",abk,abks,"
    },
    {
        "Afar",
        u8"Afaraf",
        { 'a', 'a', '\0' },
        ",aar,aars,"
    },
    {
        "Afrikaans",
        u8"Afrikaans",
        { 'a', 'f', '\0' },
        ",afr,afrs,"
    },
    {
        "Akan",
        u8"Akan",
        { 'a', 'k', '\0' },
        ",aka,"
    },
    {
        "Albanian",
        u8"gjuha shqipe",
        { 's', 'q', '\0' },
        ",sqi,alb,"
    },
    {
        "Amharic",
        u8"\u12A0\u121B\u122D\u129B",
        { 'a', 'm', '\0' },
        ",amh,"
    },
    {
        "Arabic",
        u8"\u0627\u0644\u0639\u0631\u0628\u064A\u0629",
        { 'a', 'r', '\0' },
        ",ara,"
    },
    {
        "Aragonese",
        u8"aragon\u00E9s",
        { 'a', 'n', '\0' },
        ",arg,"
    },
    {
        "Armenian",
        u8"\u0540\u0561\u0575\u0565\u0580\u0565\u0576",
        { 'h', 'y', '\0' },
        ",hye,arm,"
    },
    {
        "Assamese",
        u8"\u0985\u09B8\u09AE\u09C0\u09AF\u09BC\u09BE",
        { 'a', 's', '\0' },
        ",asm,"
    },
    {
        "Avaric",
        u8"\u0430\u0432\u0430\u0440 \u043C\u0430\u0446\u04C0, \u043C\u0430\u0433\u04C0\u0430\u0440\u0443\u043B \u043C\u0430\u0446\u04C0",
        { 'a', 'v', '\0' },
        ",ava,"
    },
    {
        "Avestan",
        u8"avesta",
        { 'a', 'e', '\0' },
        ",ave,"
    },
    {
        "Aymara",
        u8"aymar aru",
        { 'a', 'y', '\0' },
        ",aym,"
    },
    {
        "Azerbaijani",
        u8"az\u0259rbaycan dili",
        { 'a', 'z', '\0' },
        ",aze,"
    },
    {
        "Bambara",
        u8"bamanankan",
        { 'b', 'm', '\0' },
        ",bam,"
    },
    {
        "Bashkir",
        u8"\u0431\u0430\u0448\u04A1\u043E\u0440\u0442 \u0442\u0435\u043B\u0435",
        { 'b', 'a', '\0' },
        ",bak,"
    },
    {
        "Basque",
        u8"euskara",
        { 'e', 'u', '\0' },
        ",eus,baq,"
    },
    {
        "Belarusian",
        u8"\u0431\u0435\u043B\u0430\u0440\u0443\u0441\u043A\u0430\u044F \u043C\u043E\u0432\u0430",
        { 'b', 'e', '\0' },
        ",bel,"
    },
    {
        "Bengali",
        u8"\u09AC\u09BE\u0982\u09B2\u09BE",
        { 'b', 'n', '\0' },
        ",ben,"
    },
    {
        "Bihari",
        u8"\u092D\u094B\u091C\u092A\u0941\u0930\u0940",
        { 'b', 'h', '\0' },
        ",bih,"
    },
    {
        "Bislama",
        u8"Bislama",
        { 'b', 'i', '\0' },
        ",bis,"
    },
    {
        "Bosnian",
        u8"bosanski jezik",
        { 'b', 's', '\0' },
        ",bos,boss,"
    },
    {
        "Breton",
        u8"brezhoneg",
        { 'b', 'r', '\0' },
        ",bre,"
    },
    {
        "Bulgarian",
        u8"\u0431\u044A\u043B\u0433\u0430\u0440\u0441\u043A\u0438 \u0435\u0437\u0438\u043A",
        { 'b', 'g', '\0' },
        ",bul,buls,"
    },
    {
        "Burmese",
        u8"\u1017\u1019\u102C\u1005\u102C",
        { 'm', 'y', '\0' },
        ",mya,bur,"
    },
    {
        "Catalan; Valencian",
        u8"catal\u00E0, valanci\u00E0",
        { 'c', 'a', '\0' },
        ",cat,"
    },
    {
        "Chamorro",
        u8"Chamoru",
        { 'c', 'h', '\0' },
        ",cha,"
    },
    {
        "Chechen",
        u8"\u043D\u043E\u0445\u0447\u0438\u0439\u043D \u043C\u043E\u0442\u0442",
        { 'c', 'e', '\0' },
        ",che,"
    },
    {
        "Chichewa; Chewa; Nyanja",
        u8"chiChe\u0175a, chinyanja",
        { 'n', 'y', '\0' },
        ",nya,"
    },
    {
        "Chinese",
        u8"\u4E2D\u6587 (Zh\u014Dngw\u00E9n), \u6C49\u8BED, \u6F22\u8A9E",
        { 'z', 'h', '\0' },
        ",zho,chi,"
    },
    {
        "Chuvash",
        u8"\u0447\u04D1\u0432\u0430\u0448 \u0447\u04D7\u043B\u0445\u0438",
        { 'c', 'v', '\0' },
        ",chv,"
    },
    {
        "Cornish",
        u8"Kernewek",
        { 'k', 'w', '\0' },
        ",cor,"
    },
    {
        "Corsican",
        u8"corsu, lingua corsa",
        { 'c', 'o', '\0' },
        ",cos,"
    },
    {
        "Cree",
        u8"\u14C0\u1426\u1403\u152D\u140D\u140F\u1423",
        { 'c', 'r', '\0' },
        ",cre,"
    },
    {
        "Croatian",
        u8"hrvatski jezik",
        { 'h', 'r', '\0' },
        ",hrv,"
    },
    {
        "Czech",
        u8"\u010De\u0161tina, \u010Desk\u00FD jazyk",
        { 'c', 's', '\0' },
        ",ces,cze,"
    },
    {
        "Danish",
        u8"dansk",
        { 'd', 'a', '\0' },
        ",dan,"
    },
    {
        "Divehi; Dhivehi; Maldivian",
        u8"\u078B\u07A8\u0788\u07AC\u0780\u07A8",
        { 'd', 'v', '\0' },
        ",div,"
    },
    {
        "Dutch",
        u8"Nederlands, Vlaams",
        { 'n', 'l', '\0' },
        ",nld,dut,"
    },
    {
        "Dzongkha",
        u8"\u0F62\u0FAB\u0F7C\u0F44\u0F0B\u0F41",
        { 'd', 'z', '\0' },
        ",dzo,"
    },
    {
        "English",
        u8"English",
        { 'e', 'n', '\0' },
        ",eng,engs,"
    },
    {
        "Esperanto",
        u8"Esperanto",
        { 'e', 'o', '\0' },
        ",epo,"
    },
    {
        "Estonian",
        u8"eesti, eesti keel",
        { 'e', 't', '\0' },
        ",est,"
    },
    {
        "Ewe",
        u8"E\u028Begbe",
        { 'e', 'e', '\0' },
        ",ewe,"
    },
    {
        "Faroese",
        u8"f\u00F8royskt",
        { 'f', 'o', '\0' },
        ",fao,"
    },
    {
        "Fijian",
        u8"vosa Vakaviti",
        { 'f', 'j', '\0' },
        ",fij,"
    },
    {
        "Finnish",
        u8"suomi, suomen kieli",
        { 'f', 'i', '\0' },
        ",fin,"
    },
    {
        "French",
        u8"fran\u00E7ais, langue fran\u00E7aise",
        { 'f', 'r', '\0' },
        ",fra,fras,"
    },
    {
        "Fula; Fulah; Pulaar; Pular",
        u8"Fulfulde, Pulaar, Pular",
        { 'f', 'f', '\0' },
        ",ful,"
    },
    {
        "Galician",
        u8"galego",
        { 'g', 'l', '\0' },
        ",glg,"
    },
    {
        "Georgian",
        u8"\u10E5\u10D0\u10E0\u10D7\u10E3\u10DA\u10D8",
        { 'k', 'a', '\0' },
        ",kat,geo,"
    },
    {
        "German",
        u8"Deutsch",
        { 'd', 'e', '\0' },
        ",deu,ger,deus,"
    },
    {
        "Greek, Modern",
        u8"\u03B5\u03BB\u03BB\u03B7\u03BD\u03B9\u03BA\u03AC",
        { 'e', 'l', '\0' },
        ",ell,gre,ells,"
    },
    {
        u8"Guaran\u00ED",
        u8"Ava\u00F1e'\u1EBD",
        { 'g', 'n', '\0' },
        ",grn,"
    },
    {
        "Gujarati",
        u8"\u0A97\u0AC1\u0A9C\u0AB0\u0ABE\u0AA4\u0AC0",
        { 'g', 'u', '\0' },
        ",guj,"
    },
    {
        "Haitian; Haitian Creole",
        u8"Krey\u00F2l ayisyen",
        { 'h', 't', '\0' },
        ",hat,"
    },
    {
        "Hausa",
        u8"Hause, \u0647\u064E\u0648\u064F\u0633\u064E",
        { 'h', 'a', '\0' },
        ",hau,"
    },
    {
        "Hebrew (modern)",
        u8"\u05E2\u05D1\u05E8\u05D9\u05EA",
        { 'h', 'e', '\0' },
        ",heb,"
    },
    {
        "Herero",
        u8"Otjiherero",
        { 'h', 'z', '\0' },
        ",her,"
    },
    {
        "Hindi",
        u8"\u0939\u093F\u0928\u094D\u0926\u0940, \u0939\u093F\u0902\u0926\u0940",
        { 'h', 'i', '\0' },
        ",hin,"
    },
    {
        "Hiri Motu",
        u8"Hiri Motu",
        { 'h', 'o', '\0' },
        ",hmo,"
    },
    {
        "Hungarian",
        u8"magyar",
        { 'h', 'u', '\0' },
        ",hun,"
    },
    {
        "Interlingua",
        u8"Interlingua",
        { 'i', 'a', '\0' },
        ",ina,"
    },
    {
        "Indonesian",
        u8"Bahasa Indonesia",
        { 'i', 'd', '\0' },
        ",ind,"
    },
    {
        "Interlingue",
        u8"Interlingue",
        { 'i', 'e', '\0' },
        ",ile,"
    },
    {
        "Irish",
        u8"Gaeilge",
        { 'g', 'a', '\0' },
        ",gle,"
    },
    {
        "Igbo",
        u8"As\u1E85s\u1E85 Igbo",
        { 'i', 'g', '\0' },
        ",ibo,"
    },
    {
        "Inupiaq",
        u8"I\u00F1upiaq, I\u00F1upiatun",
        { 'i', 'k', '\0' },
        ",ipk,"
    },
    {
        "Ido",
        u8"Ido",
        { 'i', 'o', '\0' },
        ",ido,"
    },
    {
        "Icelandic",
        u8"\u00CDslenska",
        { 'i', 's', '\0' },
        ",isl,ice,"
    },
    {
        "Italian",
        u8"italiano",
        { 'i', 't', '\0' },
        ",ita,itas,"
    },
    {
        "Inuktitut",
        u8"\u1403\u14C4\u1483\u144E\u1450\u1466",
        { 'i', 'u', '\0' },
        ",iku,"
    },
    {
        "Japanese",
        u8"\u65E5\u672C\u8A9E (\u306B\u307B\u3093\u3054)",
        { 'j', 'a', '\0' },
        ",jpn,"
    },
    {
        "Javanese",
        u8"basa Jawa",
        { 'j', 'v', '\0' },
        ",jav,"
    },
    {
        "Kalaallisut, Greenlandic",
        u8"kalaallisut, kalaallit oqaasii",
        { 'k', 'l', '\0' },
        ",kal,"
    },
    {
        "Kannada",
        u8"\u0C95\u0CA8\u0CCD\u0CA8\u0CA1",
        { 'k', 'n', '\0' },
        ",kan,"
    },
    {
        "Kanuri",
        u8"Kanuri",
        { 'k', 'r', '\0' },
        ",kau,"
    },
    {
        "Kashmiri",
        u8"\u0915\u0936\u094D\u092E\u0940\u0930\u0940, \u0643\u0634\u0645\u064A\u0631\u064A",
        { 'k', 's', '\0' },
        ",kas,"
    },
    {
        "Kazakh",
        u8"\u049B\u0430\u0437\u0430\u049B \u0442\u0456\u043B\u0456",
        { 'k', 'k', '\0' },
        ",kaz,"
    },
    {
        "Khmer",
        u8"\u1781\u17D2\u1798\u17C2\u179A, \u1781\u17C1\u1798\u179A\u1797\u17B6\u179F\u17B6, \u1797\u17B6\u179F\u17B6\u1781\u17D2\u1798\u17C2\u179A",
        { 'k', 'm', '\0' },
        ",khm,"
    },
    {
        "Kikuyu, Gikuyu",
        u8"G\u0129k\u0169y\u0169",
        { 'k', 'i', '\0' },
        ",kik,"
    },
    {
        "Kinyarwanda",
        u8"Ikinyarwanda",
        { 'r', 'w', '\0' },
        ",kin,"
    },
    {
        "Kyrgyz",
        u8"\u041A\u044B\u0440\u0433\u044B\u0437\u0447\u0430, \u041A\u044B\u0440\u0433\u044B\u0437 \u0442\u0438\u043B\u0438",
        { 'k', 'y', '\0' },
        ",kir,"
    },
    {
        "Komi",
        u8"\u043A\u043E\u043C\u0438 \u043A\u044B\u0432",
        { 'k', 'v', '\0' },
        ",kom,"
    },
    {
        "Kongo",
        u8"KiKongo",
        { 'k', 'g', '\0' },
        ",kon,"
    },
    {
        "Korean",
        u8"\uD55C\uAD6D\uC5B4 (\u97D3\u570B\u8A9E), \uC870\uC120\uC5B4 (\u671D\u9BAE\u8A9E)",
        { 'k', 'o', '\0' },
        ",kor,"
    },
    {
        "Kurdish",
        u8"Kurd\u00EE, \u0643\u0648\u0631\u062F\u06CC",
        { 'k', 'u', '\0' },
        ",kur,"
    },
    {
        "Kwanyama, Kuanyama",
        u8"Kuanyama",
        { 'k', 'j', '\0' },
        ",kua,"
    },
    {
        "Latin",
        u8"latine, lingua latina",
        { 'l', 'a', '\0' },
        ",lat,lats,"
    },
    {
        "Luxembourgish, Letzeburgesch",
        u8"L\u00EBtzebuergesch",
        { 'l', 'b', '\0' },
        ",ltz,"
    },
    {
        "Ganda",
        u8"Luganda",
        { 'l', 'g', '\0' },
        ",lug,"
    },
    {
        "Limburgish, Limburgan, Limburger",
        u8"Limburgs",
        { 'l', 'i', '\0' },
        ",lim,"
    },
    {
        "Lingala",
        u8"Ling\u00E1la",
        { 'l', 'n', '\0' },
        ",lin,"
    },
    {
        "Lao",
        u8"\u0E9E\u0EB2\u0EAA\u0EB2\u0EA5\u0EB2\u0EA7",
        { 'l', 'o', '\0' },
        ",lao,"
    },
    {
        "Lithuanian",
        u8"lietuvi\u0173 kalba",
        { 'l', 't', '\0' },
        ",lit,"
    },
    {
        "Luba-Katanga",
        u8"Tshiluba",
        { 'l', 'u', '\0' },
        ",lub,"
    },
    {
        "Latvian",
        u8"latvie\u0161u valoda",
        { 'l', 'v', '\0' },
        ",lav,"
    },
    {
        "Manx",
        u8"Gaelg, Gailck",
        { 'g', 'v', '\0' },
        ",glv,"
    },
    {
        "Macedonian",
        u8"\u043C\u0430\u043A\u0435\u0434\u043E\u043D\u0441\u043A\u0438 \u0458\u0430\u0437\u0438\u043A",
        { 'm', 'k', '\0' },
        ",mkd,mac,"
    },
    {
        "Malagasy",
        u8"fiteny malagasy",
        { 'm', 'g', '\0' },
        ",mlg,"
    },
    {
        "Malay",
        u8"bahasa Melayu, \u0628\u0647\u0627\u0633 \u0645\u0644\u0627\u064A\u0648",
        { 'm', 's', '\0' },
        ",msa,may,"
    },
    {
        "Malayalam",
        u8"\u0D2E\u0D32\u0D2F\u0D3E\u0D33\u0D02",
        { 'm', 'l', '\0' },
        ",mal,"
    },
    {
        "Maltese",
        u8"Malti",
        { 'm', 't', '\0' },
        ",mlt,"
    },
    {
        u8"M\u0101ori",
        u8"te reo M\u0101ori",
        { 'm', 'i', '\0' },
        ",mri,mao,"
    },
    {
        u8"Marathi (Mar\u0101\u1E6Dh\u012B)",
        u8"\u092E\u0930\u093E\u0920\u0940",
        { 'm', 'r', '\0' },
        ",mar,"
    },
    {
        "Marshallese",
        u8"Kajin M\u0327aje\u013C",
        { 'm', 'h', '\0' },
        ",mah,"
    },
    {
        "Mongolian",
        u8"\u043C\u043E\u043D\u0433\u043E\u043B",
        { 'm', 'n', '\0' },
        ",mon,"
    },
    {
        "Nauru",
        u8"Ekakair\u0169 Naoero",
        { 'n', 'a', '\0' },
        ",nau,"
    },
    {
        "Navajo, Navaho",
        u8"Din\u00E9 bizaad, Din\u00E9k\u02BCeh\u01F0\u00ED",
        { 'n', 'v', '\0' },
        ",nav,"
    },
    {
        "Norwegian Bokm\u00E5l",
        u8"Norsk bokm\u00E5l",
        { 'n', 'b', '\0' },
        ",nob,"
    },
    {
        "North Ndebele",
        u8"isiNdebele",
        { 'n', 'd', '\0' },
        ",nde,"
    },
    {
        "Nepali",
        u8"\u0928\u0947\u092A\u093E\u0932\u0940",
        { 'n', 'e', '\0' },
        ",nep,"
    },
    {
        "Ndonga",
        u8"Owambo",
        { 'n', 'g', '\0' },
        ",ndo,"
    },
    {
        "Norwegian Nynorsk",
        u8"Norsk nynorsk",
        { 'n', 'n', '\0' },
        ",nno,"
    },
    {
        "Norwegian",
        u8"Norsk",
        { 'n', 'o', '\0' },
        ",nor,"
    },
    {
        "Nuosu",
        u8"\uA188\uA320\uA4BF Nuosuhxop",
        { 'i', 'i', '\0' },
        ",iii,"
    },
    {
        "South Ndebele",
        u8"isiNdebele",
        { 'n', 'r', '\0' },
        ",nbl,"
    },
    {
        "Occitan",
        u8"occitan, lenga d'\u00F2c",
        { 'o', 'c', '\0' },
        ",oci,"
    },
    {
        "Ojibwe, Ojibwa",
        u8"\u140A\u14C2\u1511\u14C8\u142F\u14A7\u140E\u14D0",
        { 'o', 'j', '\0' },
        ",oji,"
    },
    {
        "Old Church Slavonic, Church Slavic, Church Slavonic, Old Bulgarian, Old Slavonic",
        u8"\u0469\u0437\u044B\u043A\u044A \u0441\u043B\u043E\u0432\u0463\u043D\u044C\u0441\u043A\u044A",
        { 'c', 'u', '\0' },
        ",chu,"
    },
    {
        "Oromo",
        u8"Afaan Oromoo",
        { 'o', 'm', '\0' },
        ",orm,"
    },
    {
        "Oriya",
        u8"\u0B13\u0B21\u0B3C\u0B3F\u0B06",
        { 'o', 'r', '\0' },
        ",ori,"
    },
    {
        "Ossetian, Ossetic",
        u8"\u0438\u0440\u043E\u043D \u00E6\u0432\u0437\u0430\u0433",
        { 'o', 's', '\0' },
        ",oss,"
    },
    {
        "Panjabi, Punjabi",
        u8"\u0A2A\u0A70\u0A1C\u0A3E\u0A2C\u0A40, \u067E\u0646\u062C\u0627\u0628\u06CC",
        { 'p', 'a', '\0' },
        ",pan,"
    },
    {
        "P\u0101li",
        u8"\u092A\u093E\u0934\u093F",
        { 'p', 'i', '\0' },
        ",pli,"
    },
    {
        "Persian (Farsi)",
        u8"\u0641\u0627\u0631\u0633\u06CC",
        { 'f', 'a', '\0' },
        ",fas,per,"
    },
    {
        "Polish",
        u8"j\u0119zyk polski, polszczyzna",
        { 'p', 'l', '\0' },
        ",pol,pols,"
    },
    {
        "Pashto, Pushto",
        u8"\u067E\u069A\u062A\u0648",
        { 'p', 's', '\0' },
        ",pus,"
    },
    {
        "Portuguese",
        u8"portugu\u00EAs",
        { 'p', 't', '\0' },
        ",por,"
    },
    {
        "Quechua",
        u8"Runa Simi, Kichwa",
        { 'q', 'u', '\0' },
        ",que,"
    },
    {
        "Romansh",
        u8"rumantsch grischun",
        { 'r', 'm', '\0' },
        ",roh,"
    },
    {
        "Kirundi",
        u8"Ikirundi",
        { 'r', 'n', '\0' },
        ",run,"
    },
    {
        "Romanian",
        u8"limba rom\u00E2n\u0103",
        { 'r', 'o', '\0' },
        ",ron,rum,"
    },
    {
        "Russian",
        u8"\u0440\u0443\u0441\u0441\u043A\u0438\u0439 \u044F\u0437\u044B\u043A",
        { 'r', 'u', '\0' },
        ",rus,"
    },
    {
        "Sanskrit (Sa\u1E41sk\u1E5Bta)",
        u8"\u0938\u0902\u0938\u094D\u0915\u0943\u0924\u092E\u094D",
        { 's', 'a', '\0' },
        ",san,"
    },
    {
        "Sardinian",
        u8"sardu",
        { 's', 'c', '\0' },
        ",srd,"
    },
    {
        "Sindhi",
        u8"\u0938\u093F\u0928\u094D\u0927\u0940, \u0633\u0646\u068C\u064A\u060C \u0633\u0646\u062F\u06BE\u06CC",
        { 's', 'd', '\0' },
        ",snd,"
    },
    {
        "Northern Sami",
        u8"Davvis\u00E1megiella",
        { 's', 'e', '\0' },
        ",sme,"
    },
    {
        "Samoan",
        u8"gagana fa'a Samoa",
        { 's', 'm', '\0' },
        ",smo,"
    },
    {
        "Sango",
        u8"y\u00E2ng\u00E2 t\u00EE s\u00E3ng\u00F6",
        { 's', 'g', '\0' },
        ",sag,"
    },
    {
        "Serbian",
        u8"\u0441\u0440\u043F\u0441\u043A\u0438 \u0458\u0435\u0437\u0438\u043A",
        { 's', 'r', '\0' },
        ",srp,"
    },
    {
        "Scottish Gaelic; Gaelic",
        u8"G\u00E0idhlig",
        { 'g', 'd', '\0' },
        ",gla,"
    },
    {
        "Shona",
        u8"chiShona",
        { 's', 'n', '\0' },
        ",sna,"
    },
    {
        "Sinhala, Sinhalese",
        u8"\u0DC3\u0DD2\u0D82\u0DC4\u0DBD",
        { 's', 'i', '\0' },
        ",sin,"
    },
    {
        "Slovak",
        u8"sloven\u010Dina, slovensk\u00FD jazyk",
        { 's', 'k', '\0' },
        ",slk,slo,"
    },
    {
        "Slovene",
        u8"slovenski jezik, sloven\u0161\u010Dina",
        { 's', 'l', '\0' },
        ",slv,"
    },
    {
        "Somali",
        u8"Soomaaliga, af Soomaali",
        { 's', 'o', '\0' },
        ",som,"
    },
    {
        "Southern Sotho",
        u8"Sesotho",
        { 's', 't', '\0' },
        ",sot,"
    },
    {
        "South Azerbaijani",
        u8"\u062A\u0648\u0631\u06A9\u062C\u0647",
        { 'a', 'z', '\0' },
        ",azb,"
    },
    {
        "Spanish; Castilian",
        u8"espa\u00F1ol, castellano",
        { 'e', 's', '\0' },
        ",spa,"
    },
    {
        "Sundanese",
        u8"Basa Sunda",
        { 's', 'u', '\0' },
        ",sun,"
    },
    {
        "Swahili",
        u8"Kiswahili",
        { 's', 'w', '\0' },
        ",swa,"
    },
    {
        "Swati",
        u8"SiSwati",
        { 's', 's', '\0' },
        ",ssw,"
    },
    {
        "Swedish",
        u8"Svenska",
        { 's', 'v', '\0' },
        ",swe,"
    },
    {
        "Tamil",
        u8"\u0BA4\u0BAE\u0BBF\u0BB4\u0BCD",
        { 't', 'a', '\0' },
        ",tam,"
    },
    {
        "Telugu",
        u8"\u0C24\u0C46\u0C32\u0C41\u0C17\u0C41",
        { 't', 'e', '\0' },
        ",tel,"
    },
    {
        "Tajik",
        u8"\u0442\u043E\u04B7\u0438\u043A\u04E3, to\u011Fik\u012B, \u062A\u0627\u062C\u06CC\u06A9\u06CC",
        { 't', 'g', '\0' },
        ",tgk,"
    },
    {
        "Thai",
        u8"\u0E44\u0E17\u0E22",
        { 't', 'h', '\0' },
        ",tha,"
    },
    {
        "Tigrinya",
        u8"\u1275\u130D\u122D\u129B",
        { 't', 'i', '\0' },
        ",tir,"
    },
    {
        "Tibetan Standard, Tibetan, Central",
        u8"\u0F56\u0F7C\u0F51\u0F0B\u0F61\u0F72\u0F42",
        { 'b', 'o', '\0' },
        ",bod,tib,"
    },
    {
        "Turkmen",
        u8"T\u00FCrkmen, \u0422\u04AF\u0440\u043A\u043C\u0435\u043D",
        { 't', 'k', '\0' },
        ",tuk,"
    },
    {
        "Tagalog",
        u8"Wikang Tagalog, \u170F\u1712\u1703\u1705\u1714 \u1706\u1704\u170E\u1713\u1704\u1714",
        { 't', 'l', '\0' },
        ",tgl,"
    },
    {
        "Tswana",
        u8"Setswana",
        { 't', 'n', '\0' },
        ",tsn,"
    },
    {
        "Tonga (Tonga Islands)",
        u8"faka Tonga",
        { 't', 'o', '\0' },
        ",ton,"
    },
    {
        "Turkish",
        u8"T\u00FCrk\u00E7e",
        { 't', 'r', '\0' },
        ",tur,"
    },
    {
        "Tsonga",
        u8"Xitsonga",
        { 't', 's', '\0' },
        ",tso,"
    },
    {
        "Tatar",
        u8"\u0442\u0430\u0442\u0430\u0440 \u0442\u0435\u043B\u0435, tatar tele",
        { 't', 't', '\0' },
        ",tat,"
    },
    {
        "Twi",
        u8"Twi",
        { 't', 'w', '\0' },
        ",twi,"
    },
    {
        "Tahitian",
        u8"Reo Tahiti",
        { 't', 'y', '\0' },
        ",tah,"
    },
    {
        "Uyghur, Uighur",
        u8"Uy\u01A3urq\u0259, \u0626\u06C7\u064A\u063A\u06C7\u0631\u0686\u06D5",
        { 'u', 'g', '\0' },
        ",uig,"
    },
    {
        "Ukrainian",
        u8"\u0443\u043A\u0440\u0430\u0457\u043D\u0441\u044C\u043A\u0430 \u043C\u043E\u0432\u0430",
        { 'u', 'k', '\0' },
        ",ukr,"
    },
    {
        "Urdu",
        u8"\u0627\u0631\u062F\u0648",
        { 'u', 'r', '\0' },
        ",urd,"
    },
    {
        "Uzbek",
        u8"O\u2018zbek, \u040E\u0437\u0431\u0435\u043A, \u0623\u06C7\u0632\u0628\u06D0\u0643",
        { 'u', 'z', '\0' },
        ",uzb,"
    },
    {
        "Venda",
        u8"Tshiven\u1E13a",
        { 'v', 'e', '\0' },
        ",ven,"
    },
    {
        "Vietnamese",
        u8"Ti\u1EBFng Vi\u1EC7t",
        { 'v', 'i', '\0' },
        ",vie,"
    },
    {
        "Volap\u00FCk",
        u8"Volap\u00FCk",
        { 'v', 'o', '\0' },
        ",vol,"
    },
    {
        "Walloon",
        u8"walon",
        { 'w', 'a', '\0' },
        ",wln,"
    },
    {
        "Welsh",
        u8"Cymraeg",
        { 'c', 'y', '\0' },
        ",cym,wel,"
    },
    {
        "Wolof",
        u8"Wollof",
        { 'w', 'o', '\0' },
        ",wol,"
    },
    {
        "Western Frisian",
        u8"Frysk",
        { 'f', 'y', '\0' },
        ",fry,"
    },
    {
        "Xhosa",
        u8"isiXhosa",
        { 'x', 'o', '\0' },
        ",xho,"
    },
    {
        "Undefined",
        u8"undefined",
        { 'x', 'x', '\0' },
        ",,"
    },
    {
        "Yiddish",
        u8"\u05D9\u05D9\u05B4\u05D3\u05D9\u05E9",
        { 'y', 'i', '\0' },
        ",yid,"
    },
    {
        "Yoruba",
        u8"Yor\u00F9b\u00E1",
        { 'y', 'o', '\0' },
        ",yor,"
    },
    {
        "Zhuang, Chuang",
        u8"Sa\u026F cue\u014B\u0185, Saw cuengh",
        { 'z', 'a', '\0' },
        ",zha,"
    },
    {
        "Zulu",
        u8"isiZulu",
        { 'z', 'u', '\0' },
        ",zul,"
    },
    {
        nullptr,
        nullptr,
        { '\0', '\0', '\0' },
        nullptr
    }
};

// this is the list of 2 letter country names
snap_child::country_name_t const g_country_names[] =
{
    {
        { 'A', 'D', '\0' },
        "Andorra"
    },
    {
        { 'A', 'E', '\0' },
        "United Arab Emirates"
    },
    {
        { 'A', 'F', '\0' },
        "Afghanistan"
    },
    {
        { 'A', 'G', '\0' },
        "Antigua and Barbuda"
    },
    {
        { 'A', 'I', '\0' },
        "Anguilla"
    },
    {
        { 'A', 'L', '\0' },
        "Albania"
    },
    {
        { 'A', 'M', '\0' },
        "Armenia"
    },
    {
        { 'A', 'O', '\0' },
        "Angola"
    },
    {
        { 'A', 'Q', '\0' },
        "Antarctica"
    },
    {
        { 'A', 'R', '\0' },
        "Argentina"
    },
    {
        { 'A', 'S', '\0' },
        "American Samoa"
    },
    {
        { 'A', 'T', '\0' },
        "Austria"
    },
    {
        { 'A', 'U', '\0' },
        "Australia"
    },
    {
        { 'A', 'W', '\0' },
        "Aruba"
    },
    {
        { 'A', 'X', '\0' },
        u8"\00C5land Islands"
    },
    {
        { 'A', 'Z', '\0' },
        "Azerbaijan"
    },
    {
        { 'B', 'A', '\0' },
        "Bosnia and Herzegovina"
    },
    {
        { 'B', 'B', '\0' },
        "Barbados"
    },
    {
        { 'B', 'D', '\0' },
        "Bangladesh"
    },
    {
        { 'B', 'E', '\0' },
        "Belgium"
    },
    {
        { 'B', 'F', '\0' },
        "Burkina Faso"
    },
    {
        { 'B', 'G', '\0' },
        "Bulgaria"
    },
    {
        { 'B', 'H', '\0' },
        "Bahrain"
    },
    {
        { 'B', 'I', '\0' },
        "Burundi"
    },
    {
        { 'B', 'J', '\0' },
        "Benin"
    },
    {
        { 'B', 'L', '\0' },
        u8"Saint Barth\u00E9lemy"
    },
    {
        { 'B', 'M', '\0' },
        "Bermuda"
    },
    {
        { 'B', 'N', '\0' },
        "Brunei Darussalam"
    },
    {
        { 'B', 'O', '\0' },
        "Bolivia, Plurinational State of"
    },
    {
        { 'B', 'Q', '\0' },
        "Bonaire, Sint Eustatius and Saba"
    },
    {
        { 'B', 'R', '\0' },
        "Brazil"
    },
    {
        { 'B', 'S', '\0' },
        "Bahamas"
    },
    {
        { 'B', 'T', '\0' },
        "Bhutan"
    },
    {
        { 'B', 'V', '\0' },
        "Bouvet Island"
    },
    {
        { 'B', 'W', '\0' },
        "Botswana"
    },
    {
        { 'B', 'Y', '\0' },
        "Belarus"
    },
    {
        { 'B', 'Z', '\0' },
        "Belize"
    },
    {
        { 'C', 'A', '\0' },
        "Canada"
    },
    {
        { 'C', 'C', '\0' },
        "Cocos (Keeling) Islands"
    },
    {
        { 'C', 'D', '\0' },
        "Congo, the Democratic Republic of the"
    },
    {
        { 'C', 'F', '\0' },
        "Central African Republic"
    },
    {
        { 'C', 'G', '\0' },
        "Congo"
    },
    {
        { 'C', 'H', '\0' },
        "Switzerland"
    },
    {
        { 'C', 'I', '\0' },
        "C\u00F4te d'Ivoire"
    },
    {
        { 'C', 'K', '\0' },
        "Cook Islands"
    },
    {
        { 'C', 'L', '\0' },
        "Chile"
    },
    {
        { 'C', 'M', '\0' },
        "Cameroon"
    },
    {
        { 'C', 'N', '\0' },
        "China"
    },
    {
        { 'C', 'O', '\0' },
        "Colombia"
    },
    {
        { 'C', 'R', '\0' },
        "Costa Rica"
    },
    {
        { 'C', 'U', '\0' },
        "Cuba"
    },
    {
        { 'C', 'V', '\0' },
        "Cape Verde"
    },
    {
        { 'C', 'W', '\0' },
        "Cura\u00E7ao"
    },
    {
        { 'C', 'X', '\0' },
        "Christmas Island"
    },
    {
        { 'X', 'Y', '\0' },
        "Cyprus"
    },
    {
        { 'C', 'Z', '\0' },
        "Czech Republic"
    },
    {
        { 'D', 'E', '\0' },
        "Germany"
    },
    {
        { 'D', 'J', '\0' },
        "Djibouti"
    },
    {
        { 'D', 'K', '\0' },
        "Denmark"
    },
    {
        { 'D', 'M', '\0' },
        "Dominica"
    },
    {
        { 'D', 'O', '\0' },
        "Dominican Republic"
    },
    {
        { 'D', 'Z', '\0' },
        "Algeria"
    },
    {
        { 'E', 'C', '\0' },
        "Ecuador"
    },
    {
        { 'E', 'E', '\0' },
        "Estonia"
    },
    {
        { 'E', 'G', '\0' },
        "Egypt"
    },
    {
        { 'E', 'H', '\0' },
        "Western Sahara"
    },
    {
        { 'E', 'R', '\0' },
        "Eritrea"
    },
    {
        { 'E', 'S', '\0' },
        "Spain"
    },
    {
        { 'E', 'T', '\0' },
        "Ethiopia"
    },
    {
        { 'F', 'I', '\0' },
        "Finland"
    },
    {
        { 'F', 'J', '\0' },
        "Fiji"
    },
    {
        { 'F', 'K', '\0' },
        "Falkland Islands (Malvinas)"
    },
    {
        { 'F', 'M', '\0' },
        "Micronesia, Federated States of"
    },
    {
        { 'F', 'O', '\0' },
        "Faroe Islands"
    },
    {
        { 'F', 'R', '\0' },
        "France"
    },
    {
        { 'G', 'A', '\0' },
        "Gabon"
    },
    {
        { 'G', 'B', '\0' },
        "United Kingdom"
    },
    {
        { 'G', 'D', '\0' },
        "Grenada"
    },
    {
        { 'G', 'E', '\0' },
        "Georgia"
    },
    {
        { 'G', 'F', '\0' },
        "French Guiana"
    },
    {
        { 'G', 'G', '\0' },
        "Guernsey"
    },
    {
        { 'G', 'H', '\0' },
        "Ghana"
    },
    {
        { 'G', 'I', '\0' },
        "Gibraltar"
    },
    {
        { 'G', 'L', '\0' },
        "Greenland"
    },
    {
        { 'G', 'M', '\0' },
        "Gambia"
    },
    {
        { 'G', 'N', '\0' },
        "Guinea"
    },
    {
        { 'G', 'P', '\0' },
        "Guadeloupe"
    },
    {
        { 'G', 'Q', '\0' },
        "Equatorial Guinea"
    },
    {
        { 'G', 'R', '\0' },
        "Greece"
    },
    {
        { 'G', 'S', '\0' },
        "South Georgia and the South Sandwich Islands"
    },
    {
        { 'G', 'T', '\0' },
        "Guatemala"
    },
    {
        { 'G', 'U', '\0' },
        "Guam"
    },
    {
        { 'G', 'W', '\0' },
        "Guinea-Bissau"
    },
    {
        { 'G', 'Y', '\0' },
        "Guyana"
    },
    {
        { 'H', 'K', '\0' },
        "Hong Kong"
    },
    {
        { 'H', 'M', '\0' },
        "Heard Island and McDonald Islands"
    },
    {
        { 'H', 'N', '\0' },
        "Honduras"
    },
    {
        { 'H', 'R', '\0' },
        "Croatia"
    },
    {
        { 'H', 'T', '\0' },
        "Haiti"
    },
    {
        { 'H', 'U', '\0' },
        "Hungary"
    },
    {
        { 'I', 'D', '\0' },
        "Indonesia"
    },
    {
        { 'I', 'E', '\0' },
        "Ireland"
    },
    {
        { 'I', 'L', '\0' },
        "Israel"
    },
    {
        { 'I', 'M', '\0' },
        "Isle of Man"
    },
    {
        { 'I', 'N', '\0' },
        "India"
    },
    {
        { 'I', 'O', '\0' },
        "British Indian Ocean Territory"
    },
    {
        { 'I', 'Q', '\0' },
        "Iraq"
    },
    {
        { 'I', 'R', '\0' },
        "Iran, Islamic Republic of"
    },
    {
        { 'I', 'S', '\0' },
        "Iceland"
    },
    {
        { 'I', 'T', '\0' },
        "Italy"
    },
    {
        { 'J', 'E', '\0' },
        "Jersey"
    },
    {
        { 'J', 'M', '\0' },
        "Jamaica"
    },
    {
        { 'J', 'O', '\0' },
        "Jordan"
    },
    {
        { 'J', 'P', '\0' },
        "Japan"
    },
    {
        { 'K', 'E', '\0' },
        "Kenya"
    },
    {
        { 'K', 'G', '\0' },
        "Kyrgyzstan"
    },
    {
        { 'K', 'H', '\0' },
        "Cambodia"
    },
    {
        { 'K', 'I', '\0' },
        "Kiribati"
    },
    {
        { 'K', 'M', '\0' },
        "Comoros"
    },
    {
        { 'K', 'N', '\0' },
        "Saint Kitts and Nevis"
    },
    {
        { 'K', 'P', '\0' },
        "Korea, Democratic People's Republic of"
    },
    {
        { 'K', 'R', '\0' },
        "Korea, Republic of"
    },
    {
        { 'K', 'W', '\0' },
        "Kuwait"
    },
    {
        { 'K', 'Y', '\0' },
        "Cayman Islands"
    },
    {
        { 'K', 'Z', '\0' },
        "Kazakhstan"
    },
    {
        { 'L', 'A', '\0' },
        "Lao People's Democratic Republic"
    },
    {
        { 'L', 'B', '\0' },
        "Lebanon"
    },
    {
        { 'L', 'C', '\0' },
        "Saint Lucia"
    },
    {
        { 'L', 'I', '\0' },
        "Liechtenstein"
    },
    {
        { 'L', 'K', '\0' },
        "Sri Lanka"
    },
    {
        { 'L', 'R', '\0' },
        "Liberia"
    },
    {
        { 'L', 'S', '\0' },
        "Lesotho"
    },
    {
        { 'L', 'T', '\0' },
        "Lithuania"
    },
    {
        { 'L', 'U', '\0' },
        "Luxembourg"
    },
    {
        { 'L', 'V', '\0' },
        "Latvia"
    },
    {
        { 'L', 'Y', '\0' },
        "Libya"
    },
    {
        { 'M', 'A', '\0' },
        "Morocco"
    },
    {
        { 'M', 'C', '\0' },
        "Monaco"
    },
    {
        { 'M', 'D', '\0' },
        "Moldova, Republic of"
    },
    {
        { 'M', 'E', '\0' },
        "Montenegro"
    },
    {
        { 'M', 'F', '\0' },
        "Saint Martin (French part)"
    },
    {
        { 'M', 'G', '\0' },
        "Madagascar"
    },
    {
        { 'M', 'H', '\0' },
        "Marshall Islands"
    },
    {
        { 'M', 'K', '\0' },
        "Macedonia, the former Yugoslav Republic of"
    },
    {
        { 'M', 'L', '\0' },
        "Mali"
    },
    {
        { 'M', 'M', '\0' },
        "Myanmar"
    },
    {
        { 'M', 'N', '\0' },
        "Mongolia"
    },
    {
        { 'M', 'O', '\0' },
        "Macao"
    },
    {
        { 'M', 'P', '\0' },
        "Northern Mariana Islands"
    },
    {
        { 'M', 'Q', '\0' },
        "Martinique"
    },
    {
        { 'M', 'R', '\0' },
        "Mauritania"
    },
    {
        { 'M', 'D', '\0' },
        "Montserrat"
    },
    {
        { 'M', 'T', '\0' },
        "Malta"
    },
    {
        { 'M', 'U', '\0' },
        "Mauritius"
    },
    {
        { 'M', 'V', '\0' },
        "Maldives"
    },
    {
        { 'M', 'W', '\0' },
        "Malawi"
    },
    {
        { 'M', 'X', '\0' },
        "Mexico"
    },
    {
        { 'M', 'Y', '\0' },
        "Malaysia"
    },
    {
        { 'M', 'Z', '\0' },
        "Mozambique"
    },
    {
        { 'N', 'A', '\0' },
        "Namibia"
    },
    {
        { 'N', 'C', '\0' },
        "New Caledonia"
    },
    {
        { 'N', 'E', '\0' },
        "Niger"
    },
    {
        { 'N', 'F', '\0' },
        "Norfolk Island"
    },
    {
        { 'N', 'G', '\0' },
        "Nigeria"
    },
    {
        { 'N', 'I', '\0' },
        "Nicaragua"
    },
    {
        { 'N', 'L', '\0' },
        "Netherlands"
    },
    {
        { 'N', 'O', '\0' },
        "Norway"
    },
    {
        { 'N', 'P', '\0' },
        "Nepal"
    },
    {
        { 'N', 'R', '\0' },
        "Nauru"
    },
    {
        { 'N', 'U', '\0' },
        "Niue"
    },
    {
        { 'N', 'Z', '\0' },
        "New Zealand"
    },
    {
        { 'O', 'M', '\0' },
        "Oman"
    },
    {
        { 'P', 'A', '\0' },
        "Panama"
    },
    {
        { 'P', 'E', '\0' },
        "Peru"
    },
    {
        { 'P', 'F', '\0' },
        "French Polynesia"
    },
    {
        { 'P', 'G', '\0' },
        "Papua New Guinea"
    },
    {
        { 'P', 'H', '\0' },
        "Philippines"
    },
    {
        { 'P', 'K', '\0' },
        "Pakistan"
    },
    {
        { 'P', 'L', '\0' },
        "Poland"
    },
    {
        { 'P', 'M', '\0' },
        "Saint Pierre and Miquelon"
    },
    {
        { 'P', 'N', '\0' },
        "Pitcairn"
    },
    {
        { 'P', 'R', '\0' },
        "Puerto Rico"
    },
    {
        { 'P', 'S', '\0' },
        "Palestine, State of"
    },
    {
        { 'P', 'T', '\0' },
        "Portugal"
    },
    {
        { 'P', 'W', '\0' },
        "Palau"
    },
    {
        { 'P', 'Y', '\0' },
        "Paraguay"
    },
    {
        { 'Q', 'A', '\0' },
        "Qatar"
    },
    {
        { 'R', 'E', '\0' },
        "R\u00E9union"
    },
    {
        { 'R', 'O', '\0' },
        "Romania"
    },
    {
        { 'R', 'S', '\0' },
        "Serbia"
    },
    {
        { 'R', 'U', '\0' },
        "Russian Federation"
    },
    {
        { 'R', 'W', '\0' },
        "Rwanda"
    },
    {
        { 'S', 'A', '\0' },
        "Saudi Arabia"
    },
    {
        { 'S', 'B', '\0' },
        "Solomon Islands"
    },
    {
        { 'S', 'C', '\0' },
        "Seychelles"
    },
    {
        { 'S', 'D', '\0' },
        "Sudan"
    },
    {
        { 'S', 'E', '\0' },
        "Sweden"
    },
    {
        { 'S', 'G', '\0' },
        "Singapore"
    },
    {
        { 'S', 'H', '\0' },
        "Saint Helena, Ascension and Tristan da Cunha"
    },
    {
        { 'S', 'I', '\0' },
        "Slovenia"
    },
    {
        { 'S', 'J', '\0' },
        "Svalbard and Jan Mayen"
    },
    {
        { 'S', 'K', '\0' },
        "Slovakia"
    },
    {
        { 'S', 'L', '\0' },
        "Sierra Leone"
    },
    {
        { 'S', 'M', '\0' },
        "San Marino"
    },
    {
        { 'S', 'N', '\0' },
        "Senegal"
    },
    {
        { 'S', 'O', '\0' },
        "Somalia"
    },
    {
        { 'S', 'R', '\0' },
        "Suriname"
    },
    {
        { 'S', 'S', '\0' },
        "South Sudan"
    },
    {
        { 'S', 'T', '\0' },
        "Sao Tome and Principe"
    },
    {
        { 'S', 'V', '\0' },
        "El Salvador"
    },
    {
        { 'S', 'X', '\0' },
        "Sint Maarten (Dutch part)"
    },
    {
        { 'S', 'Y', '\0' },
        "Syrian Arab Republic"
    },
    {
        { 'S', 'Z', '\0' },
        "Swaziland"
    },
    {
        { 'T', 'C', '\0' },
        "Turks and Caicos Islands"
    },
    {
        { 'T', 'D', '\0' },
        "Chad"
    },
    {
        { 'T', 'F', '\0' },
        "French Southern Territories"
    },
    {
        { 'T', 'G', '\0' },
        "Togo"
    },
    {
        { 'T', 'H', '\0' },
        "Thailand"
    },
    {
        { 'T', 'J', '\0' },
        "Tajikistan"
    },
    {
        { 'T', 'K', '\0' },
        "Tokelau"
    },
    {
        { 'T', 'L', '\0' },
        "Timor-Leste"
    },
    {
        { 'T', 'M', '\0' },
        "Turkmenistan"
    },
    {
        { 'T', 'N', '\0' },
        "Tunisia"
    },
    {
        { 'T', 'O', '\0' },
        "Tonga"
    },
    {
        { 'T', 'R', '\0' },
        "Turkey"
    },
    {
        { 'T', 'T', '\0' },
        "Trinidad and Tobago"
    },
    {
        { 'T', 'V', '\0' },
        "Tuvalu"
    },
    {
        { 'T', 'W', '\0' },
        "Taiwan, Province of China"
    },
    {
        { 'T', 'Z', '\0' },
        "Tanzania, United Republic of"
    },
    {
        { 'U', 'A', '\0' },
        "Ukraine"
    },
    {
        { 'U', 'G', '\0' },
        "Uganda"
    },
    {
        { 'U', 'M', '\0' },
        "United States Minor Outlying Islands"
    },
    {
        { 'U', 'S', '\0' },
        "United States"
    },
    {
        { 'U', 'Y', '\0' },
        "Uruguay"
    },
    {
        { 'U', 'Z', '\0' },
        "Uzbekistan"
    },
    {
        { 'V', 'A', '\0' },
        "Holy See (Vatican City State)"
    },
    {
        { 'V', 'C', '\0' },
        "Saint Vincent and the Grenadines"
    },
    {
        { 'V', 'E', '\0' },
        "Venezuela, Bolivarian Republic of"
    },
    {
        { 'V', 'G', '\0' },
        "Virgin Islands, British"
    },
    {
        { 'V', 'I', '\0' },
        "Virgin Islands, U.S."
    },
    {
        { 'V', 'N', '\0' },
        "Viet Nam"
    },
    {
        { 'V', 'U', '\0' },
        "Vanuatu"
    },
    {
        { 'W', 'F', '\0' },
        "Wallis and Futuna"
    },
    {
        { 'W', 'S', '\0' },
        "Samoa"
    },
    {
        { 'Y', 'E', '\0' },
        "Yemen"
    },
    {
        { 'Y', 'T', '\0' },
        "Mayotte"
    },
    {
        { 'Z', 'A', '\0' },
        "South Africa"
    },
    {
        { 'Z', 'M', '\0' },
        "Zambia"
    },
    {
        { 'Z', 'W', '\0' },
        "Zimbabwe"
    },
    {
        { 'Z', 'Z', '\0' },
        "Unknown" // unknown, invalid, undefined
    },
    {
        { '\0', '\0', '\0' },
        nullptr
    }
};


} // no name namespace



/** \brief Get a composed version of this locale.
 *
 * The locale can be empty, just a language name, or a language
 * named, an underscore, and a country name.
 *
 * \return The composed locale.
 */
QString snap_child::locale_info_t::get_composed() const
{
    if(f_country.isEmpty())
    {
        return f_language;
    }

    return QString("%1_%2").arg(f_language).arg(f_country);
}



/** \brief Save the file data in the post_file_t object.
 *
 * This function makes a copy of the data in the post_file_t
 * object. Note that since we're using a QByteArray, the
 * real copy is done only on write and since we do not modify
 * the buffer, it should only copy the buffer pointer, not
 * the content.
 *
 * The function also computes the "real" MIME type using the
 * magic library.
 *
 * \param[in] data  The data of the file to save in this object.
 *
 * \sa snap::get_mime_type();
 */
void snap_child::post_file_t::set_data(QByteArray const& data)
{
    f_data = data;
    f_size = data.size();

    // namespace required otherwise we'd call the get_mime_type
    // function of the post_file_t class!
    f_mime_type = snap::get_mime_type(data);

//printf("mime: %s\n", f_mime_type.toUtf8().data());
}


/** \brief Retrieve the basename.
 *
 * This function returns the filename without any path.
 * The extension is not removed.
 *
 * \return The filename without a path
 */
QString snap_child::post_file_t::get_basename() const
{
    int const last_slash(f_filename.lastIndexOf('/'));
    if(last_slash >= 0)
    {
        return f_filename.mid(last_slash + 1);
    }
    return f_filename;
}


/** \brief Get the size of the buffer.
 *
 * This function retrieves the real size of the data buffer.
 * When loading an attachment from Cassandra, it is possible to
 * ask the system to not load the data to save time (i.e. if you
 * are just looking into showing a list of attachments and you
 * do not need the actual data...) In that case the load_attachment()
 * function of the content plugin only sets the size and no data.
 *
 * This function makes sure that the correct size gets returned.
 *
 * \return The size of the data.
 */
int snap_child::post_file_t::get_size() const
{
    int size(f_data.size());
    if(size == 0)
    {
        size = f_size;
    }
    return size;
}


/** \brief Initialize a child process.
 *
 * This function initializes a child process object. At this point it
 * marked as a parent process child instance (i.e. calling child process
 * functions will generate an error.)
 *
 * Whenever the parent Snap Server receives a connect from the Snap CGI
 * tool, it calls the process() function which creates the child and starts
 * processing the TCP request.
 *
 * Note that at this point there is not communication between the parent
 * and child processes other than the child process death that the parent
 * acknowledge at some point.
 */
snap_child::snap_child(server_pointer_t s)
    : f_server(s)
    //, f_child_pid(0) -- auto-init
    //, f_socket(-1) -- auto-init
    , f_start_date(0)
    , f_messenger_runner(this)
    , f_messenger_thread("background messenger connection thread", &f_messenger_runner)
{
}


/** \brief Clean up a child process.
 *
 * This function cleans up a child process. For the parent this means waiting
 * on the child, assuming that the child process was successfully started.
 * This also means that the function may block until the child dies...
 */
snap_child::~snap_child()
{
    // detach or wait till it dies?
    if(f_child_pid > 0)
    {
        int status;
        wait(&status);
    }
    //else
    //{ // this is the child process deleting itself
    //    ...
    //    if(f_socket != -1)
    //    {
    //        // this is automatic anyway (we're in Unix)
    //        // and if not already cleared, we've got more serious problems
    //        // (see the process() function for more info)
    //        close(f_socket);
    //    }
    //}
}

/** \brief Get the current date.
 *
 * PLEASE consider use get_start_date() instead of the current date.
 * 99.999% of the time, you do NOT want to use the current date or
 * all the dates you will manipulate will be different.
 *
 * In some really rare circumstance, you may want to make use of the
 * "exact" current date instead of the start date. This function
 * can be used in those rare cases.
 *
 * This function does not modify the start date of the current process.
 *
 * \sa init_start_date()
 * \sa get_start_date()
 * \sa get_start_time()
 */
int64_t snap_child::get_current_date()
{
    struct timeval tv;
    if(gettimeofday(&tv, nullptr) != 0)
    {
        int const err(errno);
        SNAP_LOG_FATAL("gettimeofday() failed with errno: ")(err);
        throw std::runtime_error("gettimeofday() failed");
    }
    return static_cast<int64_t>(tv.tv_sec) * static_cast<int64_t>(1000000)
         + static_cast<int64_t>(tv.tv_usec);
}


/** \brief Reset the start date to now.
 *
 * This function is called by the processing functions to reset the
 * start date. This is important because child objects may be reused
 * multiple times instead of allocated and deallocated by the server.
 *
 * Also, if you are writing a backend process, it should call this function
 * on each loop to make sure it gets updated. This is quite important because
 * you cannot be in control of all the functions that are going to be called
 * from your process and some of them may actually be using this value
 * without you having control over it.
 */
void snap_child::init_start_date()
{
    f_start_date = get_current_date();
}


/** \brief Change the current timezone to the newly specified timezone.
 *
 * \warning
 * This is a low level function to set a timezone. You are actually
 * expected to call the locale::set_timezone() event instead. That way
 * you will be setup with the right timezone as expected by the various
 * plugins.
 *
 * We force the server to run using the UTC timezone. This function can
 * be used to change the timezone to a user timezone.
 *
 * To restore the timezone to the default server timezone, you may call this
 * function with an empty string.
 *
 * Source:
 * http://stackoverflow.com/questions/10378093/c-api-to-return-timestring-in-specified-time-zone/10378513#10378513
 *
 * \param[in] timezone  Name of the timezone as understood by tzset(3).
 */
void snap_child::set_timezone(QString const& timezone)
{
    // force new timezone
    setenv("TZ", timezone.toUtf8().data(), 1);
    tzset();
}


/** \brief Change the current locale to the newly specified locale.
 *
 * \warning
 * You want to look into the locale plugin instead. This function
 * currently does nothing because it is very sensitive to the name
 * of the locale. See the locale::set_locale() function instead.
 *
 * We force the server to run using the C locale. This function can
 * be used to change the locale to a user locale or the website locale.
 *
 * To restore the locale to the default server locale, you may call this
 * function with an empty string.
 *
 * \note
 * The locale is not set to the empty string. This would set the locale
 * to the current $LANG parameter and since we have no control over that
 * parameter, we do not use it. It could otherwise have side effects over
 * our server which we do not want to have. Instead we use "C" as the
 * default locale. It may not do what you expected, but we can be sure to
 * control what happens in that situation.
 *
 * Source:
 * http://stackoverflow.com/questions/10378093/c-api-to-return-timestring-in-specified-time-zone/10378513#10378513
 *
 * \todo
 * It looks like we cannot use this one unless we compile all the locales
 * when the ICU library does not require such (TBD). Also the basic locale
 * requires the encoding which we could force to UTF-8, but we cannot
 * assume that every server would have UTF-8... So at this point this
 * function does nothing. You are expected to call locale functions
 * instead in order to get the correct string formatting behavior.
 *
 * \param[in] locale  Name of the locale as understood by setlocale(3).
 */
void snap_child::set_locale(QString const & locale)
{
    NOTUSED(locale);
    // force new locale
    // Note: empty ("") is like using $LANG to setup the locale
//std::cerr << "***\n*** set_locale(" << locale << ")\n***\n";
    //if(locale.isEmpty())
    //{
    //    // use "C" as the default (which is different from "" which
    //    // uses $LANG as the locale and since that could be tainted
    //    // or have unwanted side effects, we do not use it.)
    //    if(std::setlocale(LC_ALL, "C.UTF-8"))
    //    {
    //        // it looks like C.UTF-8 is available, use that instead
    //        // so we avoid downgrading to just an ANSII console
    //        std::locale::global(std::locale("C.UTF-8"));
    //    }
    //    else
    //    {
    //        std::locale::global(std::locale("C"));
    //    }
    //}
    //else
    //{
    //    // this function throws if the locale() is not available...
    //    std::locale::global(std::locale(locale.toUtf8().data()));
    //}
}


void snap_child::connect_messenger()
{
    auto server( f_server.lock() );
    f_communicator = snap_communicator::instance();

    QString communicator_addr("127.0.0.1");
    int communicator_port(4040);
    tcp_client_server::get_addr_port
            ( QString::fromUtf8(server->get_parameters()("snapcommunicator", "local_listen").c_str())
            , communicator_addr
            , communicator_port
            , "tcp"
            );

    // Create a new child_messenger object
    //
    f_messenger.reset(new child_messenger(this, communicator_addr.toUtf8().data(), communicator_port ));
    f_messenger->set_name("child_messenger");
    f_messenger->set_priority(50);

    // Add it into the instance list.
    //
    f_communicator->add_connection( f_messenger );

    // Add this to the logging facility so we can broadcast logs to snaplog via snapcommunicator.
    //
    server->configure_messenger_logging(
        std::static_pointer_cast<snap_communicator::snap_tcp_client_permanent_message_connection>( f_messenger )
    );

    if(!f_messenger_thread.start())
    {
        SNAP_LOG_ERROR("The thread used to run the background connection process did not start.");
    }
}


void snap_child::stop_messenger()
{
    if(f_communicator)
    {
        if(f_messenger)
        {
            // since we have the messenger, we should unregister
            // but the truth is that the following would just cache
            // the message and exepect the `snap_communicator::run()`
            // to process the message at leisure...
            //
            // but since we are exiting, we ignore that step at the
            // moment...
            //
            //snap::snap_communicator_message unregister_snapchild;
            //unregister_snapchild.set_command("UNREGISTER");
            //unregister_snapchild.add_parameter("service", f_service_name);
            //send_message(unregister_snapchild);

            // This causes the background thread to stop.
            //
            f_messenger->mark_done();
            f_communicator->remove_connection( f_messenger );
            f_messenger.reset();
        }

        f_communicator.reset();
    }

    f_messenger_thread.stop();
}


/** \brief Initialize the child_messenger connection.
 *
 * This function initializes the child_messenger connection. It saves
 * a pointer to the main Snap! server so it can react appropriately
 * whenever a message is received.
 *
 * \param[in] s           A pointer to the server so we can send messages there.
 * \param[in] addr        The address of the snapcommunicator server.
 * \param[in] port        The port of the snapcommunicator server.
 * \param[in] use_thread  If set to true, a thread is being used to connect to the server.
 */
snap_child::child_messenger::child_messenger(snap_child * s, std::string const & addr, int port )
    : snap_tcp_client_permanent_message_connection
      (   addr
        , port
        , tcp_client_server::bio_client::mode_t::MODE_PLAIN
        , snap_communicator::snap_tcp_client_permanent_message_connection::DEFAULT_PAUSE_BEFORE_RECONNECTING
        , true /*use_thread*/
      )
    , f_child(s)
    , f_service_name( QString("snap_child_%1").arg(gettid()) )
{
}


/** \brief Process a message we just received.
 *
 * This function is called whenever the snapcommunicator received and
 * decided to forward a message to us.
 *
 * \param[in] message  The message we just recieved.
 */
void snap_child::child_messenger::process_message(snap_communicator_message const & message)
{
    QString const command(message.get_command());
    if(command == "HELP")
    {
        // snapcommunicator wants us to tell it what commands
        // we accept
        snap_communicator_message commands_message;
        commands_message.set_command("COMMANDS");
        commands_message.add_parameter("list", "HELP,QUITTING,READY,STOP,UNKNOWN");
        send_message(commands_message);
        return;
    }
    else if(command == "QUITTING")
    {
        SNAP_LOG_WARNING("We received the QUITTING command.");
        f_child->stop_messenger();
        return;
    }
    else if(command == "READY")
    {
        // the REGISTER worked, wait for the HELP message
        return;
    }
    else if(command == "STOP")
    {
        SNAP_LOG_WARNING("we received the STOP command.");
        f_child->stop_messenger();
        return;
    }
    else if(command == "UNKNOWN")
    {
        // we sent a command that Snap! Communicator did not understand
        //
        SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
        return;
    }
}


/** \brief Process was just connected.
 *
 * This callback happens whenever a new connection is established.
 * It sends a REGISTER command to the snapcommunicator. The READY
 * reply will be received when process_message() gets called. At
 * that point we are fully registered.
 *
 * This callback happens first so if we lose our connection to
 * the snapcommunicator server, it will re-register the snapserver
 * again as expected.
 */
void snap_child::child_messenger::process_connected()
{
    snap_tcp_client_permanent_message_connection::process_connected();

    snap::snap_communicator_message register_snapchild;
    register_snapchild.set_command("REGISTER");
    register_snapchild.add_parameter("service", f_service_name);
    register_snapchild.add_parameter("version", snap::snap_communicator::VERSION);
    send_message(register_snapchild);
}


snap_child::messenger_runner::messenger_runner( snap_child* sc )
    : snap_runner("background snap_tcp_client_permanent_message_connection for asynchroneous connections")
    , f_child(sc)
{
}


void snap_child::messenger_runner::run()
{
    f_child->f_communicator->run();
}


/** \brief Fork the child process.
 *
 * Use this method to fork child processes. If SNAP_NO_FORK is on,
 * then respect the "--nofork" command line option.
 *
 * The main process should not be running any threads. At this point
 * we only check and post a warning if the number of threads is not
 * exactly 1.
 *
 * In case the snap logger uses threads (i.e. when using a SocketAppender)
 * we stop the logger before calling fork() and then we re-establish the
 * logger on return, whether the fork() worked or not.
 *
 * The return value is exactly the same as what the fork() function would
 * otherwise return.
 *
 * \exception snap_logic_exception
 * This exception is raised if the server weak pointer cannot be locked.
 *
 * \return The child PID, 0 for the child process, -1 if an error occurs.
 */
pid_t snap_child::fork_child()
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("snap_child::fork_child(): server pointer is nullptr");
    }

#ifdef SNAP_NO_FORK
    if( server->nofork() )
    {
        // simulate a working fork, we return as the child
        return 0;
    }
#endif

    // generate a warning about having more than one thread at this point
    // (it is a huge potential for a crash or lock up, hence the test)
    // See: http://snapwebsites.org/journal/2015/06/using-threads-server-uses-fork-they-dont-mix-well
    // We send the log after we re-established it (See below)
    size_t const count(server->thread_count());

    pid_t const parent_pid(getpid());

    pid_t const p(fork());

    if(p != 0)
    {
        if(count != 1)
        {
            SNAP_LOG_WARNING("snap_child::fork_child(): The number of threads before the fork() to create a snap_child is ")(count)(" when it should be 1.");
        }
    }
    else
    {
        // auto-kill child if parent dies
        //
        // TODO: ameliorate by using a "catch SIGHUP" in children via
        //       the signal class of snapcommunicator
        //
        //       ameliorate by using a SIGUSR1 or SIGUSR2 and implement
        //       a way for a possible clean exit (i.e. if child is still
        //       working, give it a chance, then after X seconds, still
        //       force a kill)
        //
        try
        {
            prctl(PR_SET_PDEATHSIG, SIGHUP);

            // Since we are in the child instance, we don't want the same object as was running in the server.
            // So we need to create a new snapcommunicator connection, register it, and add it
            // into the logging facility.
            //
            // At this point we are marking this call off because it breaks
            // the entire child environment. We'll have to find a way to
            // just send messages to snapcommunicator without the need
            // for the complicated messages going back and forth or pile
            // up all the messages and have them handled at the end only
            // (i.e. no threads need in that case...)
            //
            //connect_messenger();

            // always reconfigure the logger in the child
            logging::reconfigure();

            SNAP_LOG_TRACE("snap_child::fork_child() just hooked up logging!");

            // it could be that the prctrl() was made after the true parent died...
            // so we have to test the PID of our parent
            //
            if(getppid() != parent_pid)
            {
                SNAP_LOG_FATAL("snap_child::fork_child() lost parent too soon and did not receive SIGHUP; quit immediately.");
                exit(1);
                NOTREACHED();
            }
        }
        catch( snap_exception const & except )
        {
            SNAP_LOG_FATAL("snap_child::fork_child(): snap_exception caught: ")(except.what());
            exit(1);
            NOTREACHED();
        }
        catch( std::exception const & std_except )
        {
            // the snap_logic_exception is not a snap_exception
            // and other libraries may generate other exceptions
            // (i.e. libtld, C++ cassandra driver...)
            SNAP_LOG_FATAL("snap_child::fork_child(): std::exception caught: ")(std_except.what());
            exit(1);
            NOTREACHED();
        }
        catch( ... )
        {
            SNAP_LOG_FATAL("snap_child::fork_child(): unknown exception caught!");
            exit(1);
            NOTREACHED();
        }
    }

    return p;
}


void snap_child::output_session_log( QString const& what )
{
    QString const method(snapenv(get_name(name_t::SNAP_NAME_CORE_REQUEST_METHOD)));
    QString const agent(snapenv(get_name(name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
    QString const ip(snapenv(get_name(name_t::SNAP_NAME_CORE_REMOTE_ADDR)));
    SNAP_LOG_INFO("------------------------------------ ")(ip)
            (" ")(what)(" snap_child session (")(method)(" ")(f_uri.get_uri())(") with ")(agent);
}


/** \brief Process a request from the Snap CGI tool.
 *
 * The process function accepts a socket that was just connected.
 * Only the parent (Snap Server) can call this function. Assuming
 * that (1) the parent is calling and (2) that this snap_child
 * object is not already in use, then the function forks a new
 * process (the child).
 *
 * The parent acknowledge by saving the new process identifier
 * and closing its copy of the TCP socket.
 *
 * If the fork() call fails (returning -1) then the parent process
 * writes an HTTP error in the socket (503 Service Unavailable).
 *
 * \param[in] client  The client connection to attach to this child.
 *
 * \return true if the child process was successfully created.
 */
bool snap_child::process(tcp_client_server::bio_client::pointer_t client)
{
    if(f_is_child)
    {
        // this is a bug! die() on the spot
        // (here we ARE in the child process!)
        die(
            http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE,
            "Server Bug",
            "Your Snap! server detected a serious problem. Please check your logs for more information.",
            "snap_child::process() was called from a child process.");
        return false;
    }

    if(f_child_pid != 0)
    {
        // this is a bug!
        // WARNING: At this point we CANNOT call the die() function
        //          (we are not the child and have the wrong socket)
        SNAP_LOG_FATAL("BUG: snap_child::process() called when the process is still in use.");
        return false;
    }

    // to avoid the fork use --nofork on the command line
    //
    pid_t const p(fork_child());
    if(p != 0)
    {
        // parent process
        if(p == -1)
        {
            // WARNING: At this point we CANNOT call the die() function
            //          (we are not the child and have the wrong socket)
            SNAP_LOG_FATAL("snap_child::process() could not create child process, dropping connection.");
            return false;
        }

        // save the process identifier since it worked
        f_child_pid = p;

        // socket is now the responsibility of the child process
        // the accept() call in the parent will close it though
        return true;
    }

    f_client = client;

    try
    {
        f_ready = false;

        init_start_date();

        // child process
        f_is_child = true;

        read_environment();         // environment to QMap<>
        setup_uri();                // the raw URI

        // keep that one in release so we can at least know of all
        // the hits to the server
        output_session_log( "new" );

        // now we connect to the DB
        // move all possible work that does not required the DB before
        // this line so we avoid a network connection altogether
        NOTUSED(connect_cassandra(true));  // since we pass 'true', the returned value will always be true

        canonicalize_domain();      // using the URI, find the domain core::rules and start the canonicalization process
        canonicalize_website();     // using the canonicalized domain, find the website core::rules and continue the canonicalization process

        // check whether this website has a redirect and apply it if necessary
        // (not a full 301, just show site B instead of site A)
        site_redirect();

        // start the plugins and there initialization
        snap_string_list list_of_plugins(init_plugins(true));

        // run updates if any
        if(f_is_being_initialized)
        {
            update_plugins(list_of_plugins);
        }
        else
        {
            // TODO: to be ameliorated big time:
            //     a) we should present a really nice page and not a die()
            //     b) we should have a state held by snaplock which will
            //        be way faster than the database (but we have to
            //        reinitialize that state on a reboot...)
            //
            libdbproxy::value const state(get_site_parameter(get_name(name_t::SNAP_NAME_CORE_SITE_STATE)));
            if(state.nullValue())
            {
                // after the very first installation, it is always defined
                //
                die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE, "",
                    "The website is being initialized. Please try again in a moment. Thank you.",
                    "The site state is undefined. It is not yet initialized.");
                NOTREACHED();
            }
            if(state.stringValue() != "ready")
            {
                // at this point, we expect "ready" or "updating"
                //
                die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE, "",
                    "The website is being updated. Please try again in a moment. Thank you.",
                    QString("The site state is \"%1\", expected \"ready\"."
                           " It either was not yet initialized or it is being"
                           " updated right now (or the update process crashed?)")
                                    .arg(state.stringValue()));
                NOTREACHED();

                // double protection, this statement is not reachable
                return false;
            }
        }

        canonicalize_options();    // find the language, branch, and revision specified by the user

        // finally, "execute" the page being accessed
        execute();

        // we could delete ourselves but really only the socket is an
        // object that needs to get cleaned up properly and it is done
        // in the exit() function.
        //delete this;
        output_session_log("done");
        exit(0);
        NOTREACHED();
    }
    catch( snap_exception const & except )
    {
        SNAP_LOG_FATAL("snap_child::process(): snap_exception caught: ")(except.what());
    }
    catch( libexcept::exception_t const & e )
    {
        SNAP_LOG_FATAL("snap_child::process(): libexcept::exception_t caught: ")(e.what());
        for( auto const & stack_string : e.get_stack_trace() )
        {
            SNAP_LOG_ERROR("libexcept(): backtrace=")( stack_string );
        }
    }
    catch( libdbproxy::exception const & e )
    {
        SNAP_LOG_FATAL("snap_child::process(): libdbproxy::exception caught: ")(e.what());
        for( auto const & stack_string : e.get_stack_trace() )
        {
            SNAP_LOG_ERROR("exception(): backtrace=")( stack_string );
        }
    }
    catch( std::exception const & std_except )
    {
        // the snap_logic_exception is not a snap_exception
        // and other libraries may generate other exceptions
        // (i.e. libtld, C++ cassandra driver...)
        SNAP_LOG_FATAL("snap_child::process(): std::exception caught: ")(std_except.what());
    }
    catch( ... )
    {
        SNAP_LOG_FATAL("snap_child::process(): unknown exception caught!");
    }

    exit(1);
    NOTREACHED();

    // compiler expects a return
    return false;
}


/** \brief Retrieve the child process identifier.
 *
 * Once a child process started, it gets assigned a pid_t value. In
 * the parent process, this value can be retrieved using this function.
 * In the child process, just use getpid().
 *
 * \return This snap_child process identifier.
 */
pid_t snap_child::get_child_pid() const
{
    return f_child_pid;
}


/** \brief Kill running process.
 *
 * Use this method to stop a child process. It first sends the SIGTERM
 * message, then if the status doesn't change after a brief interval,
 * SIGKILL is used.
 *
 * \sa check_status()
 */
void snap_child::kill()
{
    if( f_child_pid == 0 )
    {
        // Child is not running
        //
        return;
    }

    ::kill( f_child_pid, SIGTERM );
    int timeout = 5;
    while( check_status() == status_t::SNAP_CHILD_STATUS_RUNNING )
    {
        sleep( 1 );
        if( --timeout <= 0 )
        {
            ::kill( f_child_pid, SIGKILL );
            break;
        }
    }
}


/** \brief Check the status of the child process.
 *
 * This function checks whether the child is still running or not.
 * The function returns the current status such as running,
 * or ready (to process a request.)
 *
 * The child process is not expected to call this function. It knows
 * it is running if it can anyway.
 *
 * The parent uses the wait() instruction to check the current status
 * if the process is running (f_child_pid is not zero.)
 *
 * \return The status of the child.
 */
snap_child::status_t snap_child::check_status()
{
    if(f_is_child)
    {
        // XXX -- call die() instead
        SNAP_LOG_FATAL("snap_child::check_status() was called from the child process.");
        return status_t::SNAP_CHILD_STATUS_RUNNING;
    }

    if(f_child_pid != 0)
    {
        int status;
        pid_t const r = waitpid(f_child_pid, &status, WNOHANG);
        if(r == static_cast<pid_t>(-1))
        {
            int const e(errno);
            SNAP_LOG_FATAL("a waitpid() returned an error (")(e)(" -- ")(strerror(e))(")");
        }
        else if(r == f_child_pid)
        {
            // the status of our child changed
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            if(WIFEXITED(status))
            {
                // stopped with exit() or return in main()
                // TODO: log info if exit() != 0?
            }
            else if(WIFSIGNALED(status))
            {
                // stopped because of a signal
                SNAP_LOG_ERROR("child process ")(f_child_pid)(" exited after it received signal #")(WTERMSIG(status));
            }
            // else -- other statuses are ignored for now
#pragma GCC diagnostic pop

            // mark that child as done
            f_child_pid = 0;
        }
        else if(r != 0)
        {
            // TODO: throw instead?
            SNAP_LOG_ERROR("waitpid() returned ")(r)(", we expected -1, 0 or ")(f_child_pid)(" instead.");
        }
    }

    return f_child_pid == 0 ? status_t::SNAP_CHILD_STATUS_READY : status_t::SNAP_CHILD_STATUS_RUNNING;
}


/** \brief Read the command and eventually the environment sent by snap.cgi
 *
 * The socket starts with a one line command. The command may be followed
 * by additional entries such as the Apache environment when the Snap CGI
 * connects to us.
 *
 * When the environment is defined, it is saved in a map so all the other
 * functions can later retrieve those values from the child. Note that at
 * this point the script does not tweak that data.
 *
 * To make sure that the entire environment is sent, snap.cgi starts the
 * feed with "\#START\\n" and terminate it with "\#END\\n".
 *
 * Note that unless we are receiving the Apache environment from the snap.cgi
 * tool, we do NOT return. This is important because when returning we start
 * generating a web page which is not what we want for the other instructions
 * such as the \#INFO.
 *
 * \section commands Understood Commands
 *
 * \li \#START
 *
 * Start passing the environment to the server.
 *
 * \li \#INFO
 *
 * Request for information about the server. The result is an environment
 * like variable/value pairs. Mainly versions are returned in that buffer.
 * Use the \#STATS for statistics information.
 *
 * \li \#STATS
 *
 * Request for statistics about this server instance. The result is an
 * environment like variable/value pairs. This command generates values
 * such as the total number of requests received, the number of children
 * currently running, etc.
 */
void snap_child::read_environment()
{
    class read_env
    {
    public:
        read_env(snap_child * snap, tcp_client_server::bio_client::pointer_t client, environment_map_t & env, environment_map_t & browser_cookies, environment_map_t & post, post_file_map_t & files)
            : f_snap(snap)
            , f_client(client)
            //, f_unget('\0') -- auto-init
            //, f_running(true) -- auto-init
            //, f_started(false) -- auto-init
            , f_env(env)
            , f_browser_cookies(browser_cookies)
            //, f_name("") -- auto-init
            //, f_value("") -- auto-init
            //, f_has_post(false) -- auto-init
            , f_post(post)
            , f_files(files)
            //, f_post_first() -- auto-init
            //, f_post_header() -- auto-init
            //, f_post_line() -- auto-init
            //, f_post_content() -- auto-init
            //, f_boundary() -- auto-init
            //, f_end_boundary() -- auto-init
            //, f_post_environment() -- auto-init
            //, f_post_index(0) -- auto-init
        {
        }

        void die(QString const & details) const
        {
            f_snap->die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE, "",
                "Unstable network connection",
                QString("an error occurred while reading the environment from the socket in the server child process (%1).").arg(details));
            NOTREACHED();
        }

        char getc() const
        {
            char c;

            //if(f_unget != '\0')
            //{
            //    c = f_unget;
            //    f_unget = '\0';
            //    return c;
            //}

            // this read blocks, so we read just 1 char. because we
            // want to stop calling read() as soon as possible (otherwise
            // we would be blocked here forever)
            if(f_client->read(&c, 1) != 1)
            {
                int const e(errno);
                die(QString("I/O error, errno: %1").arg(e));
                NOTREACHED();
            }
            return c;
        }

        //void ungetc(char c)
        //{
        //    if(f_unget != '\0')
        //    {
        //        die("too many unget() called");
        //        NOTREACHED();
        //    }
        //    f_unget = c;
        //}

        void start_process()
        {
            // #INFO
            if(f_name == "#INFO")
            {
                f_snap->snap_info();
                NOTREACHED();
            }

            // #STATS
            if(f_name == "#STATS")
            {
                f_snap->snap_statistics();
                NOTREACHED();
            }

            // #INIT
            if(f_name == "#INIT")
            {
                // user wants to initialize a website
                f_snap->mark_for_initialization();
            }
            else
            {
                // #START
                if(f_name != "#START")
                {
                    die("#START or other supported command missing.");
                    NOTREACHED();
                }
            }
            // TODO add support for a version: #START=1.2 or #INIT=1.2
            //      so that way the server can cleanly "break" if the
            //      snap.cgi or other client version is not compatible

            f_started = true;
            f_name.clear();
            f_value.clear();
        }

        void process_post_variable()
        {
            // here we have a set of post environment variables (header)
            // and the f_post_content which represents the value of the field
            //
            // Content-Disposition: form-data; name="field-name"
            // Content-Type: image/gif
            // Content-Transfer-Encoding: binary
            //
            // The Content-Type cannot be used with plain variables. We
            // distinguish plain variables from files as the
            // Content-Disposition includes a filename="..." parameter.
            //
            if(!f_post_environment.contains("CONTENT-DISPOSITION"))
            {
                die("multipart posts must have a Content-Disposition header to be considered valid.");
                NOTREACHED();
            }
            // TODO: verify and if necessary fix this as the ';' could I think
            //       appear in a string; looking at the docs, I'm not too sure
            //       but it looks like we would need to support the
            //       extended-value and extended-other-values as defined in
            //       http://tools.ietf.org/html/rfc2184
            snap_string_list disposition(f_post_environment["CONTENT-DISPOSITION"].split(";"));
            if(disposition.size() < 2)
            {
                die(QString("multipart posts Content-Disposition must at least include \"form-data\" and a name parameter, \"%1\" is not valid.")
                            .arg(f_post_environment["CONTENT-DISPOSITION"]));
                NOTREACHED();
            }
            if(disposition[0].trimmed() != "form-data")
            {
                // not happy if we don't get form-data parts
                die(QString("multipart posts Content-Disposition must be a \"form-data\", \"%1\" is not valid.")
                            .arg(f_post_environment["CONTENT-DISPOSITION"]));
                NOTREACHED();
            }
            // retrieve all the parameters, then keep those we want to keep
            int const max_strings(disposition.size());
            environment_map_t params;
            for(int i(1); i < max_strings; ++i)
            {
                // each parameter is name=<value>
                snap_string_list nv(disposition[i].split('='));
                if(nv.size() != 2)
                {
                    die(QString("parameter %1 in this multipart posts Content-Disposition does not include an equal character so \"%1\" is not valid.")
                                .arg(i)
                                .arg(f_post_environment["CONTENT-DISPOSITION"]));
                    NOTREACHED();
                }
                nv[0] = nv[0].trimmed().toLower(); // case insensitive
                nv[1] = nv[1].trimmed();
                if(nv[1].startsWith("\"")
                && nv[1].endsWith("\""))
                {
                    nv[1] = nv[1].mid(1, nv[1].length() - 2);
                }
                params[nv[0]] = nv[1];
            }
            if(!params.contains("name"))
            {
                die(QString("multipart posts Content-Disposition must include a name=\"...\" parameter, \"%1\" is not valid.")
                            .arg(f_post_environment["CONTENT-DISPOSITION"]));
                NOTREACHED();
            }
            // remove the ending \r\n
            if(f_post_content.right(2) == "\r\n")
            {
                f_post_content.resize(f_post_content.size() - 2);
            }
            else if(f_post_content.right(1) == "\n"
                 || f_post_content.right(1) == "\r")
            {
                f_post_content.resize(f_post_content.size() - 1);
            }
            f_name = params["name"];
            if(params.contains("filename"))
            {
                // make sure the filename is unique otherwise we'd overwrite
                // the previous file with the same name...
                //
                QString filename(params["filename"]);

                // this is a file so we want to save it in the f_file and
                // not in the f_post although we do create an f_post entry
                // with the filename
                //
                if(f_post.contains(f_name))
                {
                    die(QString("multipart post variable \"%1\" defined twice")
                                .arg(f_name));
                    NOTREACHED();
                }
                f_post[f_name] = filename;

                // an empty filename means no file was uploaded (which
                // is fine for optional fields or fields that are already
                // attached to a file anyway.)
                //
                if(!filename.isEmpty())
                {
                    post_file_t & file(f_files[f_name]);
                    file.set_name(f_name);
                    file.set_filename(filename);
                    ++f_post_index; // 1-based
                    file.set_index(f_post_index);
                    file.set_data(f_post_content);
                    if(params.contains("creation-date"))
                    {
                        file.set_creation_time(string_to_date(params["creation-date"]));
                    }
                    if(params.contains("modification-date"))
                    {
                        file.set_modification_time(string_to_date(params["modification-date"]));
                    }
                    // Content-Type is actually expected on this side
                    if(f_post_environment.contains("CONTENT-TYPE"))
                    {
                        file.set_original_mime_type(f_post_environment["CONTENT-TYPE"]);
                    }
                    //if(params.contains("description"))
                    //{
                    //    file.set_description(string_to_date(params["description"]));
                    //}
                    // for images also get the dimensions (width x height)
                    // note that some images are not detected properly by the
                    // magic library so we ignore the MIME type here
                    snap_image info;
                    if(info.get_info(f_post_content))
                    {
                        if(info.get_size() > 0)
                        {
                            smart_snap_image_buffer_t buffer(info.get_buffer(0));
                            file.set_image_width(buffer->get_width());
                            file.set_image_height(buffer->get_height());
                            file.set_mime_type(buffer->get_mime_type());
                        }
                    }
#ifdef DEBUG
SNAP_LOG_TRACE() << " f_files[\"" << f_name << "\"] = \"...\" (Filename: \"" << filename
    << "\" MIME: " << file.get_mime_type() << ", size: " << f_post_content.size() << ")";
#endif
                }
            }
            else
            {
                // this is a simple parameter
                if(f_post_environment.contains("CONTENT-TYPE"))
                {
                    // XXX accept a few valid types? it should not be necessary...
                    // the character encoding is defined as the form, page,
                    // or UTF-8 encoding; Content-Type not permitted here!
                    die(QString("multipart posts Content-Type is not allowed with simple parameters."));
                    NOTREACHED();
                }
                // TODO verify that the content of a post just needs to be
                //      decoded or whether it already is UTF-8 as required
                //      to be saved in f_post
                if(f_post.contains(f_name))
                {
                    die(QString("multipart post variable \"%1\" defined twice")
                                .arg(f_name));
                    NOTREACHED();
                }
                // append a '\0' so we can call is_valid_utf8()
                char const nul('\0');
                f_post_content.append(nul);
                if(!is_valid_utf8(f_post_content.data()))
                {
                    f_snap->die(http_code_t::HTTP_CODE_BAD_REQUEST, "Invalid Form Content",
                        "Your form includes characters that are not compatible with the UTF-8 encoding. Try to avoid special characters and try again. If you are using Internet Explorer, know that older versions may not be compatible with international characters.",
                        "is_valid_utf8() returned false against the user's content");
                    NOTREACHED();
                }
                // make sure to view the input as UTF-8 characters
                f_post[f_name] = QString::fromUtf8(f_post_content.data(), f_post_content.size() - 1); //snap_uri::urldecode(f_post_content, true);?
#ifdef DEBUG
//SNAP_LOG_TRACE() << " f_post[\"" << f_name << "\"] = \"" << f_post_content.data() << "\"\n";
#endif
            }
        }

        bool process_post_line()
        {
            // found a marker?
            if(f_post_line.length() >= f_boundary.length())
            {
                if(f_post_line == f_end_boundary)
                {
                    if(f_post_first)
                    {
                        die("got end boundary without a start");
                        NOTREACHED();
                    }
                    process_post_variable();
                    return true;
                }

                if(f_post_line == f_boundary)
                {
                    // got the first boundary yet?
                    if(f_post_first)
                    {
                        // we got the first boundary
                        f_post_first = false;
                        return false;
                    }
                    process_post_variable();

                    // on next line, we are reading a new header
                    f_post_header = true;

                    // we are done with those in this iteration
                    f_post_environment.clear();
                    f_post_content.clear();
                    return false;
                }
            }

            if(f_post_first)
            {
                die("the first POST boundary is missing.");
                NOTREACHED();
            }

            if(f_post_header)
            {
                if(f_post_line.isEmpty() || (f_post_line.size() == 1 && f_post_line.at(0) == '\r'))
                {
                    // end of the header
                    f_post_header = false;
                    return false;
                }
                char const nul('\0');
                f_post_line.append(nul);
                if(!is_valid_ascii(f_post_line.data()))
                {
                    f_snap->die(http_code_t::HTTP_CODE_BAD_REQUEST, "Invalid Form Content",
                        "Your multi-part form header includes characters that are not compatible with the ASCII encoding.",
                        "is_valid_ascii() returned false against a line of the user's multipart form header");
                    NOTREACHED();
                }

                // we got a header (Blah: value)
                QString const line(f_post_line);
#ifdef DEBUG
//SNAP_LOG_TRACE(" ++ header line [\n")(line.trimmed())("\n] ")(line.size());
#endif
                if(isspace(line.at(0).unicode()))
                {
                    // continuation of the previous header, concatenate
                    f_post_environment[f_name] += " " + line.trimmed();
                }
                else
                {
                    // new header
                    int const p(line.indexOf(':'));
                    if(p == -1)
                    {
                        die("invalid header variable name/value pair, no ':' found.");
                        NOTREACHED();
                    }
                    // render name case insensitive
                    f_name = line.mid(0, p).trimmed().toUpper();
                    // TODO: verify that f_name is a valid header name
                    f_post_environment[f_name] = line.mid(p + 1).trimmed();
                }
            }
            else
            {
                // this is content for the current variable
                f_post_content += f_post_line;
                f_post_content += '\n'; // the '\n' was not added to f_post_line
            }

            return false;
        }

        void process_post()
        {
            // one POST per request!
            if(f_has_post)
            {
                die("at most 1 #POST is accepted in the environment.");
                NOTREACHED();
            }
            f_has_post = true;

            if(!f_env.contains("CONTENT_TYPE")
            || !f_env["CONTENT_TYPE"].startsWith("multipart/form-data"))
            {
                // standard post, just return and let the main loop
                // handle the name/value pairs
                return;
            }

            // multi-part posts require special handling
            // (i.e. these are not simple VAR=VALUE)
            //
            // the POST is going to be multiple lines with
            // \r characters included! We read then all
            // up to the closing boundary
            //
            // Example of such a variable:
            // CONTENT_TYPE=multipart/form-data; boundary=---------5767747
            //
            // IMPORTANT NOTE:
            // Sub-parts are NOT supported in HTML POST messages. This is
            // clearly mentioned in HTML5 documentations:
            // http://www.w3.org/html/wg/drafts/html/master/forms.html#multipart-form-data

            // 1. Get the main boundary from the CONTENT_TYPE
            snap_string_list content_info(f_env["CONTENT_TYPE"].split(';'));
            QString boundary;
            int const max_strings(content_info.size());
            for(int i(1); i < max_strings; ++i)
            {
                QString param(content_info[i].trimmed());
                if(param.startsWith("boundary="))
                {
                    boundary = param.mid(9).trimmed();
                    break;
                }
            }
            if(boundary.isEmpty())
            {
                die("multipart POST does not include a valid boundary.");
                NOTREACHED();
            }
            f_boundary = ("--" + boundary).toLatin1();
            f_end_boundary = f_boundary;
            f_end_boundary.append("--\r", 3);
            f_boundary += '\r';
            //f_post_first = true; -- function cannot be called more than once
            //f_post_header = true;

            for(;;)
            {
                char const c(getc());
                if(c == '\n')
                {
                    if(process_post_line())
                    {
                        return;
                    }
                    f_post_line.clear();
                }
                else
                {
                    f_post_line += c;
                }
            }
        }

        void process_line()
        {
            // not started yet? check low level commands then
            if(!f_started)
            {
                start_process();
                return;
            }

            // got to the end?
            if(f_name == "#END")
            {
                f_running = false;
                return;
            }

            // got a POST?
            if(f_name == "#POST")
            {
                process_post();
                return;
            }

            if(f_name.isEmpty())
            {
                die("empty lines are not accepted in the child environment.");
                NOTREACHED();
            }
            if(f_has_post)
            {
                f_post[f_name] = snap_uri::urldecode(f_value, true);
#ifdef DEBUG
//SNAP_LOG_TRACE("(simple) f_post[\"")(f_name)("\"] = \"")(f_value)("\" (\"")(f_post[f_name])("\");");
#endif
            }
            else
            {
                if(f_name == "HTTP_COOKIE")
                {
                    // special case for cookies
                    snap_string_list cookies(f_value.split(';', QString::SkipEmptyParts));
#ifdef DEBUG
//SNAP_LOG_TRACE(" HTTP_COOKIE = [\"")(f_value)("\"]");
#endif
                    int const max_strings(cookies.size());
                    for(int i(0); i < max_strings; ++i)
                    {
                        QString const name_value(cookies[i]);
                        snap_string_list nv(name_value.trimmed().split('=', QString::SkipEmptyParts));
                        if(nv.size() == 2)
                        {
                            // XXX check with other systems to see
                            //     whether urldecode() is indeed
                            //     necessary here
                            QString const cookie_name(snap_uri::urldecode(nv[0], true));
                            QString const cookie_value(snap_uri::urldecode(nv[1], true));
//std::cerr << " f_browser_cookies[\"" << cookie_name << "\"] = \"" << cookie_value << "\"; (\"" << cookies[i] << "\")\n";
                            if(f_browser_cookies.contains(cookie_name))
                            {
                                // This happens when you have a cookie with the
                                // same name defined with multiple domains,
                                // multiple paths, multiple expiration dates...
                                //
                                // Unfortunately, we are not told which one is
                                // which when we reach this line of code, yet
                                // the last one will be the most qualified one
                                // according to most browsers and servers
                                //
                                // http://tools.ietf.org/html/rfc6265#section-5.4
                                //
                                SNAP_LOG_DEBUG(QString("cookie \"%1\" defined twice")
                                            .arg(cookie_name));
                            }
                            f_browser_cookies[cookie_name] = cookie_value;
                        }
                    }
                }
                else
                {
                    // TODO: verify that f_name is a valid header name
                    f_env[f_name] = f_value;
#ifdef DEBUG
//SNAP_LOG_TRACE(" f_env[\"")(f_name)("\"] = \"")(f_value)("\"");
#endif
                }
            }
        }

        void run()
        {
            bool reading_name(true);
            do
            {
                char const c = getc();
                if(c == '=' && reading_name)
                {
                    reading_name = false;
                }
                else if(c == '\n')
                {
#ifdef DEBUG
//SNAP_LOG_TRACE("f_name=")(f_name)(", f_value=\"")(f_value)("\" when reading the \\n");
#endif
                    process_line();

                    // clear for next line
                    f_name.clear();
                    f_value.clear();
                    reading_name = true;
                }
                else if(c == '\r')
                {
                    die("got a \\r character in the environment (not in a multi-part POST)");
                    NOTREACHED();
                }
                else if(reading_name)
                {
                    if(isspace(c))
                    {
                        die("spaces are not allowed in environment variable names");
                        NOTREACHED();
                    }
                    f_name += c;
                }
                else
                {
                    f_value += c;
                }
            }
            while(f_running);
        }

        bool has_post() const
        {
            return f_has_post;
        }

#ifdef DEBUG
        void output_debug_log()
        {
            std::stringstream ss;
            ss << "post:" << std::endl;
            for( auto const & pair : f_post.toStdMap() )
            {
                ss << pair.first << ": " << pair.second << std::endl;
            }

            //SNAP_LOG_DEBUG( ss.str().c_str() );
        }
#endif

    private:
        mutable snap_child *        f_snap = nullptr;
        tcp_client_server::bio_client::pointer_t
                                    f_client;
        //char                        f_unget = 0;
        bool                        f_running = true;
        bool                        f_started = false;

        environment_map_t &         f_env;
        environment_map_t &         f_browser_cookies;
        QString                     f_name;
        QString                     f_value;

        bool                        f_has_post = false;
        environment_map_t &         f_post;
        post_file_map_t &           f_files;
        bool                        f_post_first = true;
        bool                        f_post_header = true;
        QByteArray                  f_post_line;
        QByteArray                  f_post_content;
        QByteArray                  f_boundary;
        QByteArray                  f_end_boundary;
        environment_map_t           f_post_environment;
        uint32_t                    f_post_index = 0;
    };

    // reset the old environment
    f_env.clear();
    f_post.clear();
    f_files.clear();

#ifdef DEBUG
    SNAP_LOG_TRACE("Read environment variables including POST data.");
#endif

    read_env r(this, f_client, f_env, f_browser_cookies, f_post, f_files);
#ifdef DEBUG
    r.output_debug_log();
#endif
    r.run();
    f_has_post = r.has_post();

#ifdef DEBUG
    SNAP_LOG_TRACE("Done reading environment variables.")(f_has_post ? " (This request includes a POST)" : "");
#endif

    trace("#START\n");
}


/** \brief Mark the server for initialization.
 *
 * This special flag allows us to initialize the plugins without having
 * to actually try to access a specific website.
 */
void snap_child::mark_for_initialization()
{
    f_is_being_initialized = true;
}


/** \brief Write data to the output socket.
 *
 * This function writes data to the output socket.
 *
 * \exception runtime_error
 * This exception is thrown if the write() fails writing all the bytes. This
 * generally means the client closed the socket early.
 *
 * \param[in] data  The data pointer.
 * \param[in] size  The number of bytes to write from data.
 */
void snap_child::write(char const * data, ssize_t size)
{
    if(!f_client)
    {
        // this happens from backends that do not have snap.cgi running
        return;
    }

    if(f_client->write(data, size) != size)
    {
        SNAP_LOG_FATAL("error while sending data to a client.");
        // XXX throw? we cannot call die() because die() calls write()!
        throw std::runtime_error("error while sending data to the client");
    }
}


/** \brief Write a string to the socket.
 *
 * This function is an overload of the write() data buffer. It writes the
 * specified string using strlen() to obtain the length. The string must be
 * a valid null terminated C string.
 *
 * \param[in] str  The string to write to the socket.
 */
void snap_child::write(char const *str)
{
    write(str, strlen(str));
}


/** \brief Write a QString to the socket.
 *
 * This function is an overload that writes the QString to the socket. This
 * QString is transformed to UTF-8 before being processed.
 *
 * \param[in] str  The QString to write to the socket.
 */
void snap_child::write(QString const& str)
{
    QByteArray a(str.toUtf8());
    write(a.data(), a.size());
}


/** \brief Generate the Snap information buffer and return it.
 *
 * This function prints out information about the Snap! Server.
 * This means writing information about all the different libraries
 * in use such as their version, name, etc.
 */
void snap_child::snap_info()
{
    QString version;

    // getting started
    write("#START\n");

    // the library (server) version
    write("VERSION=" SNAPWEBSITES_VERSION_STRING "\n");

    // operating system
    version = "OS=";
#ifdef Q_OS_LINUX
    version += "Linux";
#else
#error "Unsupported operating system."
#endif
    version += "\n";
    write(version);

    // the Qt versions
    write("QT=" QT_VERSION_STR "\n");
    version = "RUNTIME_QT=";
    version += qVersion();
    version += "\n";
    write(version);

    // the libtld version
    write("LIBTLD=" LIBTLD_VERSION "\n");
    version = "RUNTIME_LIBTLD=";
    version += tld_version();
    version += "\n";
    write(version);

    // the libQtCassandra version
    version = "LIBQTCASSANDRA=";
    version += libdbproxy::LIBDBPROXY_LIBRARY_VERSION_STRING;
    version += "\n";
    write(version);
    version = "RUNTIME_LIBQTCASSANDRA=";
    version += libdbproxy::libdbproxy::version();
    version += "\n";
    write(version);

    // the libQtSerialization version
    version = "LIBQTSERIALIZATION=";
    version += QtSerialization::QT_SERIALIZATION_LIBRARY_VERSION_STRING;
    version += "\n";
    write(version);
    version = "RUNTIME_LIBQTSERIALIZATION=";
    version += QtSerialization::QLibraryVersion();
    version += "\n";
    write(version);

    // since we do not have an environment we cannot connect
    // to the Cassandra cluster...

    // done
    write("#END\n");

    exit(1);
}


/** \brief Return the current stats in name/value pairs format.
 *
 * This command returns the server statistics.
 */
void snap_child::snap_statistics()
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }

    QString s;

    // getting started
    write("#START\n");

    // the library (server) version
    write("VERSION=" SNAPWEBSITES_VERSION_STRING "\n");

    // the number of connections received by the server up until this child fork()'ed
    s = "CONNECTIONS_COUNT=";
    s += QString::number(server->connections_count());
    s += "\n";
    write(s);

    // done
    write("#END\n");

    exit(1);
}


/** \brief Setup the URI from the environment.
 *
 * This function gets the different variables from the environment
 * it just received from the snap.cgi script and builds the corresponding
 * Snap URI object with it. This will then be used to determine the
 * domain and finally the website.
 */
void snap_child::setup_uri()
{
    // PROTOCOL
    if(f_env.count("HTTPS") == 1)
    {
        if(f_env["HTTPS"] == "on")
        {
            f_uri.set_protocol("https");
        }
        else
        {
            f_uri.set_protocol("http");
        }
    }
    else
    {
        f_uri.set_protocol("http");
    }

    // HOST (domain name including all sub-domains)
    if(f_env.count("HTTP_HOST") != 1)
    {
        // this was tested in snap.cgi, but who knows who connected to us...
        //
        die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE, "",
                    "HTTP_HOST is required but not defined in your request.",
                    "HTTP_HOST was not defined in the user request");
        NOTREACHED();
    }
    QString host(f_env["HTTP_HOST"]);
    int const port_pos(host.indexOf(':'));
    if(port_pos != -1)
    {
        // remove the port information
        host = host.left(port_pos);
    }
    if(host.isEmpty())
    {
        die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE, "",
                    "HTTP_HOST is required but is empty in your request.",
                    "HTTP_HOST was defined but there was no domain name");
        NOTREACHED();
    }
    f_uri.set_domain(host);

    // PORT
    if(f_env.count("SERVER_PORT") != 1)
    {
        die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE, "",
                    "SERVER_PORT is required but not defined in your request.",
                    "SERVER_PORT was not defined in the user request");
        NOTREACHED();
    }
    f_uri.set_port(f_env["SERVER_PORT"]);

    // QUERY STRING
    if(f_env.count("QUERY_STRING") == 1)
    {
        f_uri.set_query_string(f_env["QUERY_STRING"]);
    }

    // REQUEST URI
    if(f_env.count(snap::get_name(name_t::SNAP_NAME_CORE_REQUEST_URI)) != 1)
    {
        die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE, "",
                     "REQUEST_URI is required but not defined in your request.",
                     "REQUEST_URI was not defined in the user's request");
        NOTREACHED();
    }
    // For some reasons I was thinking that the q=... was necessary to
    // find the correct REQUEST_URI -- it is only if you want to allow
    // for snap.cgi?q=... syntax in the client's agent, otherwise it
    // is totally useless; since we do not want the ugly snap.cgi syntax
    // we completely removed the need for it
    //
    // Get the path from the REQUEST_URI
    // note that it includes the query string when there is one so we must
    // make sure to remove that part if defined
    //
    QString path;
    {
        int const p = f_env[snap::get_name(name_t::SNAP_NAME_CORE_REQUEST_URI)].indexOf('?');
        if(p == -1)
        {
            path = f_env[snap::get_name(name_t::SNAP_NAME_CORE_REQUEST_URI)];
        }
        else
        {
            path = f_env[snap::get_name(name_t::SNAP_NAME_CORE_REQUEST_URI)].mid(0, p);
        }
    }

    QString extension;
    if(path != "." && path != "..")
    {
        // TODO: make the size (2048) a variable? (parameter in
        //       snapserver.conf)
        //
        // I'm not totally sure this test is really necessary,
        // but it probably won't hurt for a while (Drupal was
        // limiting to 128 in the database and that was way
        // too small, but 2048 is the longest you can use
        // with Internet Explorer)
        //
        if(path.length() > 2048)
        {
            // See SNAP-99 for more info about this limit
            //
            // TBD: maybe we should redirect instead of dying in this case?
            //
            die(http_code_t::HTTP_CODE_REQUEST_URI_TOO_LONG, "",
                         "The path of this request is too long.",
                         "We accept paths up to 2048 characters.");
            NOTREACHED();
        }
        f_uri.set_path(path);
        int limit(path.lastIndexOf('/'));
        if(limit < 0)
        {
            limit = 0;
        }
        int const ext(path.lastIndexOf('.'));
        if(ext > limit)
        {
            extension = path.mid(ext);
            // check for a compression and include that and
            // the previous extension
            if(extension == ".gz"       // gzip
            || extension == ".Z"        // Unix compress
            || extension == ".bz2")     // bzip2
            {
                // we generally expect .gz but we have to take
                // whatever extension the user added to make sure
                // we send the file in the right format
                // we will also need to use the Accept-Encoding
                // and make use of the Content-Encoding
                // TODO: make use of extension instead of Accept-Encoding
                f_uri.set_option("compression", extension);
                int const real_ext(path.lastIndexOf('.', ext - 1));
                if(real_ext > limit)
                {
                    // retrieve the extension without the compression
                    extension = path.mid(real_ext, real_ext - ext);
                }
                else
                {
                    // do not view a compression extension by itself
                    // as an extension
                    //
                    extension.clear();
                }
            }
        }
    }
    f_uri.set_option("extension", extension);

//std::cerr << "    set path to: [" << path << "]\n";
//
//std::cerr << "        original [" << f_uri.get_original_uri() << "]\n";
//std::cerr << "             uri [" << f_uri.get_uri() << "] + #! [" << f_uri.get_uri(true) << "]\n";
//std::cerr << "        protocol [" << f_uri.protocol() << "]\n";
//std::cerr << "     full domain [" << f_uri.full_domain() << "]\n";
//std::cerr << "top level domain [" << f_uri.top_level_domain() << "]\n";
//std::cerr << "          domain [" << f_uri.domain() << "]\n";
//std::cerr << "     sub-domains [" << f_uri.sub_domains() << "]\n";
//std::cerr << "            port [" << f_uri.get_port() << "]\n";
//std::cerr << "            path [" << f_uri.path() << "] \n";
//std::cerr << "    query string [" << f_uri.query_string() << "]\n";
}


/** \brief Change the main path.
 *
 * This function allows a plugin to change the main path. This is very
 * practical to allow one to redirect without changing the path the
 * user sees in his browser. Such has to be done in the
 * on_check_for_redirect() signal of your module, very early on before
 * the path gets used (except in other redirect functions).
 *
 * The path change should not modified anything else in the URI (i.e.
 * options, query string, etc.)
 *
 * \param[in] path  The new path to save in f_uri.
 */
void snap_child::set_uri_path(QString const & path)
{
    f_uri.set_path(path);
}


/** \brief Get a copy of the URI information.
 *
 * This function returns a constant reference (i.e. a read-only "copy")
 * of the URI used to access the server. This is the request we want to
 * answer.
 *
 * \return The URI reference.
 */
snap_uri const & snap_child::get_uri() const
{
    return f_uri;
}


/** \brief Retrieve the current action.
 *
 * This function is used to retrieve the action defined in the query string.
 * This value is changed with what will be used just before the permission
 * verification process starts.
 *
 * Trying to retrieve the action too soon may give you an invalid value.
 * Note that if you are generating the page contents, then you are past
 * the verification process so this action value was corrected as required.
 *
 * \return  The name of the action used to check this page.
 */
QString snap_child::get_action() const
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    return f_uri.query_option(server->get_parameter("qs_action"));
}


/** \brief Define the action.
 *
 * This function is used to save the action being used to check the
 * main page. The actions are defined in the database. These are
 * "view", "edit", "delete", "administer", etc.
 *
 * Any action can be set, the permission plugin verifies that it exists
 * and that the user has permissions before actually apply the action.
 *
 * \param[in] action  The action used to check the page.
 */
void snap_child::set_action(QString const& action)
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    f_uri.set_query_option(server->get_parameter("qs_action"), action);
}


/** \brief Check that the email is legal.
 *
 * A legal email has a legal name on the left of the @ character
 * and a valid domain and TLD. We do not accept emails that represent
 * local userbox (i.e. webmaster, root, etc. by themselves) because
 * from a website these do not really make sense.
 *
 * If the email address is considered valid, knowing that we also check
 * the Top Level Domain name (TLD), then we further send a request to
 * the domain name handling that domain to make sure we can get information
 * about the MX record. If the domain does not provide an MX record, then
 * there is no emails to be sent.
 *
 * \note
 * If max > 1 you probably want to keep allow_example_domain set to false
 * otherwise you may get verified_email_t::VERIFIED_EMAIL_MIXED as a result
 * which means you cannot know which email is an example and which is not.
 *
 * \todo
 * Determine whether email addresses with the domain name of "example"
 * are allowed. In some cases they are (i.e. we have a customers that
 * has to create some pages linked to an email and at times those are
 * not real email and thus we use `name@example.com` for those...)
 * Especially, if someone registers an account with such an email it
 * is totally useless and we should prevent it.
 *
 * \todo
 * We want to count the number of times a certain IP address tries to
 * verify an email address and fails. That way we can block them after
 * X attempts. Because really someone should not be able to fail to
 * enter their email address so many times.
 *
 * \exception users_exception_invalid_email
 * This function does not return if the email is invalid. Instead it throws.
 * We may later want to have a version that returns true if valid, false
 * otherwise. At the same time, we generate three different errors here.
 *
 * \param[in] email  The email to verify.
 * \param[in] max  The maximum number of emails supported. May be 0, usually 1.
 * \param[in] allow_example_domain  Whether the "example" domain is allowed.
 *
 * \return Whether the email is considered valid, invalid, is an example
 *         or a mix of valid and example emails.
 */
snap_child::verified_email_t snap_child::verify_email(QString const & email, size_t const max, bool allow_example_domain)
{
    // is there an actual email?
    // (we may want to remove standalone and duplicated commas too)
    if(email.isEmpty())
    {
        if(max == 0)
        {
            return verified_email_t::VERIFIED_EMAIL_EMPTY;
        }
        throw snap_child_exception_invalid_email("no email defined");
    }

    // check the email name, domain, and TLD
    tld_email_list tld_emails;
    if(tld_emails.parse(email.toUtf8().data(), 0) != TLD_RESULT_SUCCESS)
    {
        throw snap_child_exception_invalid_email("invalid user email");
    }

    // make sure there is a limit to the number of emails, if max was set
    // to 1 we expect exactly one email (we previously eliminated the
    // case where there would be none)
    //
    if(static_cast<size_t>(tld_emails.count()) > max)
    {
        throw snap_child_exception_invalid_email(QString("too many emails, excepted up to %1 got %2 instead.").arg(max).arg(tld_emails.count()));
    }

    // if the email string is not empty, then there has to be at least one
    // email otherwise the parse() function should have returned an error
    //
    if(tld_emails.count() == 0)
    {
        throw snap_child_exception_invalid_email(QString("no emails, even though \"%1\" is not empty.").arg(email));
    }

    libdbproxy::table::pointer_t mx_table(get_table(get_name(name_t::SNAP_NAME_MX)));

    verified_email_t result(verified_email_t::VERIFIED_EMAIL_UNKNOWN);

    // function to set the result to either STANDARD or MIXED
    // (we use a lambda because we use that twice)
    //
    auto set_result = [&result]()
    {
        switch(result)
        {
        case verified_email_t::VERIFIED_EMAIL_UNKNOWN:
            result = verified_email_t::VERIFIED_EMAIL_STANDARD;
        case verified_email_t::VERIFIED_EMAIL_STANDARD:
            break;

        default:
            result = verified_email_t::VERIFIED_EMAIL_MIXED;
            break;

        }
    };


    // finally, check that the MX record exists for that email address, if
    // not then we can immediately say its wrong; not that if multiple emails
    // are defined, you get one throw if any one of them is wrong...
    //
    tld_email_list::tld_email_t e;
    while(tld_emails.next(e))
    {
        if(allow_example_domain)
        {
            tld_object const d(e.f_domain.c_str());
            if(d.domain_only() == "example")
            {
                // in this case a domain named "example" is considered valid
                // (TODO the libtld should tell us whether this is true
                // or not because this does not apply to all TLDs...)
                //
                // Note: we do not save such in our database, no need.
                //
                switch(result)
                {
                case verified_email_t::VERIFIED_EMAIL_UNKNOWN:
                    SNAP_LOG_TRACE("domain \"")(e.f_domain)("\" is considered to represent an example email.");
                    result = verified_email_t::VERIFIED_EMAIL_EXAMPLE;
                case verified_email_t::VERIFIED_EMAIL_EXAMPLE:
                    break;

                default:
                    result = verified_email_t::VERIFIED_EMAIL_MIXED;
                    break;

                }
                continue;
            }
        }

        libdbproxy::row::pointer_t row(mx_table->getRow(QString::fromUtf8(e.f_domain.c_str())));
        libdbproxy::value last_checked_value(row->getCell(QString(get_name(name_t::SNAP_NAME_CORE_MX_LAST_CHECKED)))->getValue());
        if(last_checked_value.size() == sizeof(int64_t))
        {
            time_t const last_update(last_checked_value.int64Value());
            if(f_start_date < last_update + 86400LL * 1000000LL)
            {
                libdbproxy::value result_value(row->getCell(QString(get_name(name_t::SNAP_NAME_CORE_MX_RESULT)))->getValue());
                if(result_value.safeSignedCharValue())
                {
                    // considered valid without the need to check again
                    //
                    SNAP_LOG_TRACE("domain \"")(e.f_domain)("\" is considered valid (saved in mx table)");
                    set_result();
                    continue;
                }
                // this is not valid
                //
                throw snap_child_exception_invalid_email(QString("domain \"%1\" from email \"%2\" does not provide an MX record (from cache).")
                        .arg(QString::fromUtf8(e.f_domain.c_str()))
                        .arg(QString::fromUtf8(e.f_canonicalized_email.c_str())));
            }
        }
        row->getCell(QString(get_name(name_t::SNAP_NAME_CORE_MX_LAST_CHECKED)))->setValue(f_start_date);
        mail_exchangers exchangers(e.f_domain);
        if(!exchangers.domain_found())
        {
            signed char const failed(0);
            row->getCell(QString(get_name(name_t::SNAP_NAME_CORE_MX_RESULT)))->setValue(failed);
            throw snap_child_exception_invalid_email(QString("domain \"%1\" from email \"%2\" does not exist.")
                        .arg(QString::fromUtf8(e.f_domain.c_str()))
                        .arg(QString::fromUtf8(e.f_canonicalized_email.c_str())));
        }
        if(exchangers.size() == 0)
        {
            signed char const failed(0);
            row->getCell(QString(get_name(name_t::SNAP_NAME_CORE_MX_RESULT)))->setValue(failed);
            throw snap_child_exception_invalid_email(QString("domain \"%1\" from email \"%2\" does not provide an MX record.")
                        .arg(QString::fromUtf8(e.f_domain.c_str()))
                        .arg(QString::fromUtf8(e.f_canonicalized_email.c_str())));
        }
        signed char const succeeded(1);
        row->getCell(QString(get_name(name_t::SNAP_NAME_CORE_MX_RESULT)))->setValue(succeeded);

        SNAP_LOG_TRACE("domain \"")(e.f_domain)("\" was just checked and is considered valid (it has an MX record.)");

        // TODO: if we have the timeout, we could save the time when
        //       the data goes out of date (instead of using exactly
        //       1 day as we do now) although we probably should use
        //       a minimum of 1 day anyway

        // TODO: also save the MX info (once we are to make use of any of it...)

        set_result();
    }

    return result;
}


/** \brief Connect to the Cassandra database system.
 *
 * This function connects to the Cassandra database system and
 * returns only if the connection succeeds. If it fails, it logs
 * the fact and sends an error back to the user.
 *
 * By default the function calls die() on error. If you would
 * prefer to have the function return with false, then set the
 * child parameter to false.
 *
 * \param[in] child  Whether a child is calling this function.
 *
 * \return true if the connection succeeded, false if child is false
 *         and the connection failed, does not return if child is true
 *         and the connection failed.
 */
bool snap_child::connect_cassandra(bool child)
{
    // Cassandra already exists?
    if(f_cassandra)
    {
        if(!child)
        {
            SNAP_LOG_DEBUG("snap_child::connect_cassandra() already considered connected.");

            // here we return true since the Cassandra connection is
            // already in place, valid and well
            //
            return true;
        }
        die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE, "",
                "Our database is being initialized more than once.",
                "The connect_cassandra() function cannot be called more than once.");
        NOTREACHED();
    }

    // retrieve the address:port info
    //
    // WARNING: server->get_snapdbproxy_addr/port() do not work in
    //          snapbackend so we do it this way instead.
    //
    QString snapdbproxy_addr("127.0.0.1");
    int snapdbproxy_port(4042);
    snap_config config("snapdbproxy");
    tcp_client_server::get_addr_port(config["listen"], snapdbproxy_addr, snapdbproxy_port, "tcp");

    // connect to Cassandra
    f_cassandra = libdbproxy::libdbproxy::create();
    f_cassandra->setDefaultConsistencyLevel(libdbproxy::CONSISTENCY_LEVEL_QUORUM);
    bool connected(false);
    try
    {
        connected = f_cassandra->connect(snapdbproxy_addr, snapdbproxy_port);
    }
    catch(std::exception const & e)
    {
        connected = false; // make double sure this is still false
        SNAP_LOG_FATAL("Could not connect to the snapdbproxy server (")(snapdbproxy_addr)(":")(snapdbproxy_port)("). Reason: ")(e.what());
    }
    if(!connected)
    {
        // cassandra is not valid, get rid of it
        f_cassandra.reset();
        if(!child)
        {
            SNAP_LOG_WARNING("snap_child::connect_cassandra() could not connect to snapdbproxy.");
            return false;
        }
        die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE, "",
                "Our database system is temporarilly unavailable.",
                "Could not connect to Cassandra");
        NOTREACHED();
    }

// WARNING: The f_casssandra->contexts() function should not be used anymore
//          (only to check whether the context exists,) because the context
//          is normally created by snapmanager[.cgi] now.
SNAP_LOG_WARNING("snap_child::connect_cassandra() should not have to call contexts() anymore...");

    try
    {
        // select the Snap! context
        f_cassandra->getContexts();
        QString const context_name(get_name(name_t::SNAP_NAME_CONTEXT));
        f_context = f_cassandra->findContext(context_name);
        if(!f_context)
        {
            // we connected to the database, but it is not properly initialized!?
            //
            f_cassandra.reset();
            if(!child)
            {
                SNAP_LOG_WARNING("snap_child::connect_cassandra() could not read the context.");
                return false;
            }
            die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE,
                    "",
                    "Our database system does not seem to be properly installed.",
                    QString("The child process connected to Cassandra but it could not find the \"%1\" context.").arg(context_name));
            NOTREACHED();
        }
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_FATAL("Connected to snapdbproxy server, but could not gather the Cassandra metadata (")(snapdbproxy_addr)(":")(snapdbproxy_port)("). Reason: ")(e.what());

        // we connected to the database, but it is not properly initialized!?
        //
        f_context.reset();  // just in case, also reset the context (it should already be null here)
        f_cassandra.reset();
        if(!child)
        {
            SNAP_LOG_WARNING("snap_child::connect_cassandra() could not read the context metadata.");
            return false;
        }
        die(http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE,
                "",
                "Our database system is not currently available.",
                "The child process connected to Cassandra through snapdbproxy, but it could not find retrieve the context metadata.");
        NOTREACHED();
    }

    // TBD -- Is that really the right place for this?
    //        (in this way it is done once for any plugin using
    //        the snap expression system; but it looks like a hack!)
    snap_expr::expr::set_cassandra_context(f_context);

    return true;
}


/** \brief Completely disconnect from cassandra.
 *
 * Whenever we receive a NOCASSANDRA event in a backend, we want to
 * disconnect completely. We will then reconnect once we receive the
 * CASSANDRAREADY message.
 */
void snap_child::disconnect_cassandra()
{
    snap_expr::expr::set_cassandra_context(nullptr);

    // we must get rid of the site table otherwise it will hold a shared
    // pointer to the context and the context to the libdbproxy object
    //
    reset_sites_table();

    // make sure the context cache is cleared too
    //
    f_context.reset();

    f_cassandra.reset();
}


/** \brief Retrieve the pointer to a table.
 *
 * This function is generally used by plugins to retrieve tables they
 * use to manage their data.
 *
 * \exception snap_child_exception_no_cassandra
 * If this function gets called when the Cassandra context is not yet
 * defined, this exception is raised.
 *
 * \exception snap_child_exception_table_missing
 * The table must already exists or this exception is raised. Tables
 * are created by running the snapcreatetables tool before starting
 * the snapserver.
 *
 * \param[in] table_name  The name of the table to create.
 */
libdbproxy::table::pointer_t snap_child::get_table(QString const & table_name)
{
    if(f_context == nullptr)
    {
        throw snap_child_exception_no_cassandra("table \"" + table_name + "\" cannot be determined, we have no context.");
    }

    libdbproxy::table::pointer_t table(f_context->findTable(table_name));
    if(table == nullptr)
    {
        throw snap_child_exception_table_missing("table \"" + table_name + "\" does not exist.");
    }

    return table;
}


/** \brief Canonalize the domain information.
 *
 * This function uses the URI to find the domain core::rules and
 * start the canonicalization process.
 *
 * The canonicalized domain is a domain name with sub-domains that
 * are required. All the optional sub-domains will be removed.
 *
 * All the variables are saved as options in the f_uri object.
 *
 * \todo
 * The functionality of this function needs to be extracted so it becomes
 * available to others (i.e. probably moved to snap_uri.cpp) that way we
 * can write tools that show the results of this parser.
 */
void snap_child::canonicalize_domain()
{
    // retrieve domain table
    QString const table_name(get_name(name_t::SNAP_NAME_DOMAINS));
    libdbproxy::table::pointer_t table(f_context->getTable(table_name));

    // row for that domain exists?
    f_domain_key = f_uri.domain() + f_uri.top_level_domain();
    if(!table->exists(f_domain_key))
    {
        // this domain doesn't exist; i.e. that's a 404
        die(http_code_t::HTTP_CODE_NOT_FOUND, "Domain Not Found", "This website does not exist. Please check the URI and make corrections as required.", "User attempt to access \"" + f_domain_key + "\" which is not defined as a domain.");
        NOTREACHED();
    }

    // get the core::rules
    libdbproxy::value value(table->getRow(f_domain_key)->getCell(QString(get_name(name_t::SNAP_NAME_CORE_RULES)))->getValue());
    if(value.nullValue())
    {
        // Null value means an empty string or undefined column and either
        // way it's wrong here
        die(http_code_t::HTTP_CODE_NOT_FOUND, "Domain Not Found", "This website does not exist. Please check the URI and make corrections as required.", "User attempt to access domain \"" + f_domain_key + "\" which does not have a valid core::rules entry.");
        NOTREACHED();
    }

    // parse the rules to our domain structures
    domain_rules r;
    // QBuffer takes a non-const QByteArray so we have to create a copy
    QByteArray data(value.binaryValue());
    QBuffer in(&data);
    in.open(QIODevice::ReadOnly);
    QtSerialization::QReader reader(in);
    r.read(reader);

    // we add a dot because the list of variables are expected to
    // end with a dot, but only if sub_domains is not empty
    QString sub_domains(f_uri.sub_domains());
    if(!sub_domains.isEmpty())
    {
        sub_domains += ".";
    }
    int const max_rules(r.size());
    for(int i = 0; i < max_rules; ++i)
    {
        QSharedPointer<domain_info> info(r[i]);

        // build the regex (TODO: pre-compile the regex?
        // the problem is the var. name versus data parsed)
        QString re;
        int vmax(info->size());
        for(int v = 0; v < vmax; ++v)
        {
            QSharedPointer<domain_variable> var(info->get_variable(v));

            // put parameters between () so we get the data in
            // variables (options) later
            re += "(" + var->get_value() + ")";
            if(!var->get_required())
            {
                // optional sub-domain
                re += "?";
            }
        }
        QRegExp regex(re);
        if(regex.exactMatch(sub_domains))
        {
            // we found the domain!
            snap_string_list captured(regex.capturedTexts());
            QString canonicalized;

            // note captured[0] is the full matching pattern, we ignore it
            for(int v = 0; v < vmax; ++v)
            {
                QSharedPointer<domain_variable> var(info->get_variable(v));

                QString sub_domain_value(captured[v + 1]);
                // remove the last dot because in most cases we do not want it
                // in the variable even if it were defined in the regex
                if(!sub_domain_value.isEmpty() && sub_domain_value.right(1) == ".")
                {
                    sub_domain_value = sub_domain_value.left(sub_domain_value.length() - 1);
                }

                if(var->get_required())
                {
                    // required, use default if empty
                    // TODO -- test that one, it seems that && should be used, not ||
                    if(sub_domain_value.isEmpty()
                    || var->get_type() == domain_variable::DOMAIN_VARIABLE_TYPE_WEBSITE)
                    {
                        sub_domain_value = var->get_default();
                    }
                    f_uri.set_option(var->get_name(), sub_domain_value);

                    // these make up the final canonicalized domain name
                    canonicalized += snap_uri::urlencode(sub_domain_value, ".");
                }
                else if(!sub_domain_value.isEmpty())
                {
                    // optional sub-domain, set only if not empty
                    if(var->get_type() == domain_variable::DOMAIN_VARIABLE_TYPE_WEBSITE)
                    {
                        sub_domain_value = var->get_default();
                    }
                    f_uri.set_option(var->get_name(), sub_domain_value);
                }
                else
                {
                    // optional with a default, use it
                    sub_domain_value = var->get_default();
                    if(!sub_domain_value.isEmpty())
                    {
                        f_uri.set_option(var->get_name(), sub_domain_value);
                    }
                }
            }

            // now we've got the website key
            if(canonicalized.isEmpty())
            {
                f_website_key = f_domain_key;
            }
            else
            {
                f_website_key = canonicalized + "." + f_domain_key;
            }
            return;
        }
    }

    // no domain match, we're dead meat
    die(http_code_t::HTTP_CODE_NOT_FOUND, "Domain Not Found", "This website does not exist. Please check the URI and make corrections as required.", "The domain \"" + f_uri.full_domain() + "\" did not match any domain name defined in your Snap! system. Should you remove it from your DNS?");
}


/** \brief Finish the canonicalization process.
 *
 * The function reads the website core::rules and continue the parsing process
 * of the URI.
 *
 * The sub-domain and domain canonicalization was accomplished in the previous
 * process: canonicalize_domain(). This is not done again in the websites.
 *
 * This process includes the following checks:
 *
 * 1. Protocol
 * 2. Port
 * 3. Query String
 * 4. Path
 *
 * The protocol, port, and query strings are checked as they are found in the
 * website variables of the core::rules.
 *
 * The path is checked once all the variables were checked and if the protocol,
 * port, and query strings were all matching as expected. If any one of them
 * does not match then we don't need to check the path.
 *
 * \note
 * As the checks of the protocol, port, and query strings are found, we cannot
 * put them in the options just yet since if the path check fails, then
 * another entry could be the proper match and that other entry may have
 * completely different variables.
 *
 * \todo
 * The functionality of this function needs to be extracted so it becomes
 * available to others (i.e. probably moved to snap_uri.cpp) that way we
 * can write tools that show the results of this parser.
 */
void snap_child::canonicalize_website()
{
    // retrieve website table
    QString const table_name(get_name(name_t::SNAP_NAME_WEBSITES));
    libdbproxy::table::pointer_t table(f_context->getTable(table_name));

    // row for that website exists?
    if(!table->exists(f_website_key))
    {
        // this website doesn't exist; i.e. that's a 404
        die(http_code_t::HTTP_CODE_NOT_FOUND, "Website Not Found", "This website does not exist. Please check the URI and make corrections as required.", "User attempt to access \"" + f_website_key + "\" which was not defined as a website.");
        NOTREACHED();
    }

    // get the core::rules
    libdbproxy::value value(table->getRow(f_website_key)->getCell(QString(get_name(name_t::SNAP_NAME_CORE_RULES)))->getValue());
    if(value.nullValue())
    {
        // Null value means an empty string or undefined column and either
        // way it's wrong here
        die(http_code_t::HTTP_CODE_NOT_FOUND, "Website Not Found", "This website does not exist. Please check the URI and make corrections as required.", "User attempt to access website \"" + f_website_key + "\" which does not have a valid core::rules entry.");
        NOTREACHED();
    }

    // parse the rules to our website structures
    website_rules r;
    QByteArray data(value.binaryValue());
    QBuffer in(&data);
    in.open(QIODevice::ReadOnly);
    QtSerialization::QReader reader(in);
    r.read(reader);

    // we check decoded paths
    QString uri_path(f_uri.path(false));
    int const max_rules(r.size());
    for(int i = 0; i < max_rules; ++i)
    {
        QSharedPointer<website_info> info(r[i]);

        // build the regex (TODO: pre-compile the regex?
        // the problem is the var. name versus data parsed)
        QString protocol("http");
        QString port("80");
        QMap<QString, QString> query;
        QString re_path;
        int vmax(info->size());
        bool matching(true);
        for(int v = 0; matching && v < vmax; ++v)
        {
            QSharedPointer<website_variable> var(info->get_variable(v));

            // put parameters between () so we get the data in
            // variables (options) later
            QString param_value("(" + var->get_value() + ")");
            switch(var->get_part())
            {
            case website_variable::WEBSITE_VARIABLE_PART_PATH:
                re_path += param_value;
                if(!var->get_required())
                {
                    // optional sub-domain
                    re_path += "?";
                }
                break;

            case website_variable::WEBSITE_VARIABLE_PART_PORT:
            {
                QRegExp regex(param_value);
                if(!regex.exactMatch(QString("%1").arg(f_uri.get_port())))
                {
                    matching = false;
                    break;
                }
                snap_string_list captured(regex.capturedTexts());
                port = captured[1];
            }
                break;

            case website_variable::WEBSITE_VARIABLE_PART_PROTOCOL:
            {
                QRegExp regex(param_value);
                // the case of the protocol in the regex doesn't matter
                // TODO (TBD):
                // Although I'm not 100% sure this is correct, we may
                // instead want to use lower case in the source
                regex.setCaseSensitivity(Qt::CaseInsensitive);
                if(!regex.exactMatch(f_uri.protocol()))
                {
                    matching = false;
                    break;
                }
                snap_string_list captured(regex.capturedTexts());
                protocol = captured[1];
            }
                break;

            case website_variable::WEBSITE_VARIABLE_PART_QUERY:
            {
                // the query string parameters are not ordered...
                // the variable name is 1 to 1 what is expected on the URI
                QString name(var->get_name());
                if(f_uri.has_query_option(name))
                {
                    // make sure it matches first
                    QRegExp regex(param_value);
                    if(!regex.exactMatch(f_uri.query_option(name)))
                    {
                        matching = false;
                        break;
                    }
                    snap_string_list captured(regex.capturedTexts());
                    query[name] = captured[1];
                }
                else if(var->get_required())
                {
                    // if required then we want to use the default
                    query[name] = var->get_default();
                }
            }
                break;

            default:
                throw std::runtime_error("unknown part specified in website_variable::f_part");

            }
        }
        if(!matching)
        {
            // one of protocol, port, or query string failed
            // (path is checked below)
            continue;
        }
        // now check the path, if empty assume it matches and
        // also we have no extra options
        QString canonicalized_path;
        if(!re_path.isEmpty())
        {
            // match from the start, but it doesn't need to match the whole path
            QRegExp regex("^" + re_path);
            if(regex.indexIn(uri_path) != -1)
            {
                // we found the site including a path!
                // TODO: should we keep the length of the captured data and
                //       remove it from the path sent down the road?
                //       (note: if you have a path such as /blah/foo and
                //       you remove it, then what looks like /robots.txt
                //       is really /blah/foo/robots.txt which is wrong.)
                //       However, if the path is only used for options such
                //       as languages, those options should be removed from
                //       the original path.
                snap_string_list captured(regex.capturedTexts());

                // note captured[0] is the full matching pattern, we ignore it
                for(int v = 0; v < vmax; ++v)
                {
                    QSharedPointer<website_variable> var(info->get_variable(v));

                    if(var->get_part() == website_variable::WEBSITE_VARIABLE_PART_PATH)
                    {
                        QString path_value(captured[v + 1]);

                        if(var->get_required())
                        {
                            // required, use default if empty
                            if(path_value.isEmpty()
                            || var->get_type() == website_variable::WEBSITE_VARIABLE_TYPE_WEBSITE)
                            {
                                path_value = var->get_default();
                            }
                            f_uri.set_option(var->get_name(), path_value);

                            // these make up the final canonicalized domain name
                            canonicalized_path += "/" + snap_uri::urlencode(path_value, "~");
                        }
                        else if(!path_value.isEmpty())
                        {
                            // optional path, set only if not empty
                            if(var->get_type() == website_variable::WEBSITE_VARIABLE_TYPE_WEBSITE)
                            {
                                path_value = var->get_default();
                            }
                            f_uri.set_option(var->get_name(), path_value);
                        }
                        else
                        {
                            // optional with a default, use it
                            path_value = var->get_default();
                            if(!path_value.isEmpty())
                            {
                                f_uri.set_option(var->get_name(), path_value);
                            }
                        }
                    }
                }
            }
            else
            {
                matching = false;
            }
        }

        if(matching)
        {
            // now we've got the protocol, port, query strings, and paths
            // so we can build the final URI that we'll use as the site key
            QString canonicalized;
            f_uri.set_option("protocol", protocol);
            canonicalized += protocol + "://" + f_website_key;
            f_uri.set_option("port", port);
            if(port.toInt() != snap_uri::protocol_to_port(protocol))
            {
                canonicalized += ":" + port;
            }
            if(canonicalized_path.isEmpty())
            {
                canonicalized += "/";
            }
            else
            {
                canonicalized += canonicalized_path;
            }
            QString canonicalized_query;
            for(QMap<QString, QString>::const_iterator it(query.begin()); it != query.end(); ++it)
            {
                f_uri.set_query_option(it.key(), it.value());
                if(!canonicalized_query.isEmpty())
                {
                    canonicalized_query += "&";
                }
                canonicalized_query += snap_uri::urlencode(it.key()) + "=" + snap_uri::urlencode(it.value());
            }
            if(!canonicalized_query.isEmpty())
            {
                canonicalized += "?" + canonicalized_query;
            }
            // now we've got the site key
            f_site_key = canonicalized;
            f_original_site_key = f_site_key; // in case of a redirect...
            f_site_key_with_slash = f_site_key;
            if(f_site_key.right(1) != "/")
            {
                f_site_key_with_slash += "/";
            }
            return;
        }
    }

    // no website match, we're dead meat
    die(http_code_t::HTTP_CODE_NOT_FOUND, "Website Not Found", "This website does not exist. Please check the URI and make corrections as required.", "The website \"" + f_website_key + "\" did not match any website defined in your Snap! system. Should you remove it from your DNS?");
}


/** \brief Determine the language, branch, revision, and compression.
 *
 * This function takes parameters as specified by the client and determines
 * the language, branch, revision, and compression that are to be used to
 * select the data to be returned to the client.
 *
 * In most cases visitors can only see the branch and revision that are
 * marked as current. Editors can see all branches and revisions. Everyone
 * can view any language, and translators (who are editors as well) can
 * create new pages in a the languages they were assigned.
 *
 * The compression information is canonicalized here so we do not have to
 * do it over and over again. It initializes the f_compressions vector
 * with the very first entry privileged, as the favored entry. The last
 * entry may be the special value COMPRESSION_INVALID in which case the
 * 406 error (HTTP_CODE_NOT_ACCEPTABLE) should be generated.
 *
 * \todo
 * The user may select a preferred language in his account. That has
 * to be used before the browser information. However, the options
 * (domain, path, or GET variable,) still have higher priority.
 * Unfortunately, although plugins are already initialized, whether
 * the user is logged in is not yet known so we cannot check from
 * this function.
 */
void snap_child::canonicalize_options()
{
    // *** LANGUAGE / COUNTRY ***
    // first take care of the language

    // transform the language specified by the browser in an array
    http_strings::WeightedHttpString languages(snapenv(get_name(name_t::SNAP_NAME_CORE_HTTP_ACCEPT_LANGUAGE)));
    http_strings::WeightedHttpString::part_t::vector_t const & browser_languages(languages.get_parts());
    if(!browser_languages.isEmpty())
    {
        // not sorted by default, make sure it gets sorted
        //
        languages.sort_by_level();

        int const max_languages(browser_languages.size());
        for(int i(0); i < max_languages; ++i)
        {
            QString browser_lang(browser_languages[i].get_name());
            QString browser_country;
            if(verify_locale(browser_lang, browser_country, false))
            {
                // only keep those that are considered valid (they all should
                // be, but in case a hacker or strange client access our
                // systems...)
                locale_info_t l;
                l.set_language(browser_lang);
                l.set_country(browser_country);
                f_browser_locales.push_back(l);
                // added in order
            }
            // else -- browser sent us something we don't understand
        }
    }

    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    QString const qs_lang(server->get_parameter("qs_language"));
    QString lang(f_uri.query_option(qs_lang));
    QString country;

    if(!lang.isEmpty())
    {
        // user specified locale
        verify_locale(lang, country, true);
    }
    //else
    //{
    //    // the lang option in the URI is defined from:
    //    //
    //    //   1. a sub-domain name
    //    //   2. a section in the path
    //    //   3. a query string parameter
    //    //
    //    // and that's it, but we also want to support another set of
    //    // languages: the ones defined by the browser variable named:
    //    //          HTTP_ACCEPT_LANGUAGE
    //    //
    //    // the problem we have here is that the browser variable defines
    //    // a set of supported languages and all have a right to be used
    //    // in a visitor's request, example:
    //    //
    //    //   HTTP_ACCEPT_LANGUAGE=en-us,en;q=0.8,fr-fr;q=0.5,fr;q=0.3
    //    //
    //    // To resolve this issue, we do not handle the browser language
    //    // here, instead of do it in the snap_child::get_language() function,
    //    // which is important because a plugin may force the language too.
    //    //
    //    if(!f_browser_locales.isEmpty())
    //    {
    //        // language and country already broken up
    //        lang = f_browser_locales[0].get_language();
    //        country = f_browser_locales[0].get_country();
    //    }
    //    else
    //    {
    //        lang = "xx";
    //    }
    //}

    // *** BRANCH / REVISION ***
    // now take care of the branch and revision

    // current or working branch (working_branch=1)
    QString const qs_working_branch(server->get_parameter("qs_working_branch"));
    QString const working_branch_entry(f_uri.query_option(qs_working_branch));
    bool const working_branch(!working_branch_entry.isEmpty());

    // branch (branch=<branch>)
    QString const qs_branch(server->get_parameter("qs_branch"));
    QString branch(f_uri.query_option(qs_branch));

    // rev (rev=<branch>.<revision>)
    QString const qs_rev(server->get_parameter("qs_rev"));
    QString rev(f_uri.query_option(qs_rev));

    // revision (revision=<revision>)
    QString const qs_revision(server->get_parameter("qs_revision"));
    QString revision(f_uri.query_option(qs_revision));

    if(branch.isEmpty() && revision.isEmpty())
    {
        if(!rev.isEmpty())
        {
            snap_string_list r(rev.split('.'));
            branch = r[0];
            int const size(r.size());
            if(size != 1)
            {
                if(size == 2)
                {
                    branch = r[0];
                    revision = r[1];
                }
                else
                {
                    // refuse branch/revision + rev definitions together
                    die(http_code_t::HTTP_CODE_BAD_REQUEST, "Invalid Revision",
                            QString("The revision (%1) is not valid. It is expected to be a branch number, a period (.), and a revision number.").arg(rev),
                            "We found a number of period other than 1 or 2.");
                    NOTREACHED();
                }
            }
        }
        // else use the default
    }
    else if(!rev.isEmpty())
    {
        // refuse branch/revision + rev definitions together
        die(http_code_t::HTTP_CODE_BAD_REQUEST, "Invalid Revision",
                "You defined a rev parameter along a branch and/or revision, which is not supported. Remove one or the other to access your page.",
                QString("We only accept a branch+revision (%1.%2) or a rev (%3).").arg(branch).arg(revision).arg(rev));
        NOTREACHED();
    }

    snap_version::version_number_t branch_value(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED));
    snap_version::version_number_t revision_value(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED));
    if(!branch.isEmpty())
    {
        // now verify that both are formed of digits only
        bool ok;
        branch_value = branch.toInt(&ok, 10);
        if(!ok)
        {
            // if defined the branch needs to be a valid decimal number
            die(http_code_t::HTTP_CODE_BAD_REQUEST, "Invalid Branch",
                    QString("Branch number \"%1\" is invalid. Only digits are expected.").arg(branch),
                    "The user did not give us a number as the branch number.");
            NOTREACHED();
        }
    }
    if(!revision.isEmpty())
    {
        bool ok;
        revision_value = revision.toInt(&ok, 10);
        if(!ok)
        {
            // if defined the revision needs to be a valid decimal number
            die(http_code_t::HTTP_CODE_BAD_REQUEST, "Invalid Revision",
                    QString("Revision number \"%1\" is invalid. Only digits are expected.").arg(revision),
                    "The user did not give us a number as the revision number.");
            NOTREACHED();
        }
    }

    // *** COMPRESSION ***
    //
    // compression is called "Encoding" in the HTTP reference
    http_strings::WeightedHttpString encodings(snapenv("HTTP_ACCEPT_ENCODING"));

    // server defined compression is named "*" (i.e. server chooses)
    // so first we check for gzip because we support that compression
    // and that is our favorite for now (will change later with sdpy
    // support eventually...)
    compression_vector_t compressions;
    bool got_gzip(false);
    float const gzip_level(std::max(std::max(encodings.get_level("gzip"), encodings.get_level("x-gzip")), encodings.get_level("*")));
    float const deflate_level(encodings.get_level("deflate"));
    if(gzip_level > 0.0f && gzip_level >= deflate_level)
    {
        compressions.push_back(compression_t::COMPRESSION_GZIP);
        got_gzip = true;
    }

    // now check all the other encodings and add them
    http_strings::WeightedHttpString::part_t::vector_t browser_compressions(encodings.get_parts());
    std::stable_sort(browser_compressions.begin(), browser_compressions.end());
    int const max_compressions(browser_compressions.size());
    for(int i(0); i < max_compressions; ++i)
    {
        QString encoding_name(browser_compressions[i].get_name());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if(browser_compressions[i].get_level() == 0.0f)
#pragma GCC diagnostic pop
        {
            if(encoding_name == "identity")
            {
                // if you reach this entry, generate a 406!
                // this means the user will not accept uncompressed data
                compressions.push_back(compression_t::COMPRESSION_INVALID);
            }
        }
        else
        {
            if(encoding_name == "gzip"
            || encoding_name == "x-gzip"
            || encoding_name == "*")
            {
                // since we support multiple name we make use of a
                // flag to avoid adding gzip more than once
                if(!got_gzip)
                {
                    compressions.push_back(compression_t::COMPRESSION_GZIP);
                    got_gzip = true;
                }
            }
            else if(encoding_name == "deflate")
            {
                compressions.push_back(compression_t::COMPRESSION_DEFLATE);
            }
            else if(encoding_name == "bz2")
            {
                // not yet implemented though...
                compressions.push_back(compression_t::COMPRESSION_BZ2);
            }
            else if(encoding_name == "sdch")
            {
                // not yet implemented though...
                compressions.push_back(compression_t::COMPRESSION_SDCH);
            }
            else if(encoding_name == "identity")
            {
                // identity is acceptable
                compressions.push_back(compression_t::COMPRESSION_IDENTITY);
            }
            // else -- we do not support yet...
        }
    }

    // *** CACHE CONTROL ***
    cache_control_settings cache_control;
    cache_control.set_max_age(cache_control_settings::IGNORE_VALUE); // defaults to 0 which is not correct for client's cache information
    cache_control.set_cache_info(snapenv("HTTP_CACHE_CONTROL"), false);

    // *** SAVE RESULTS ***
    f_language = lang;
    f_country = country;
    //f_language_key = f_language;
    //if(!f_country.isEmpty())
    //{
    //    f_language_key += "_";
    //    f_language_key += f_country;
    //}

    // TBD: add support for long revisions? (for JS and CSS files)
    f_working_branch = working_branch;
    f_branch = branch_value;
    f_revision = revision_value;
    f_revision_key = QString("%1.%2").arg(f_branch).arg(f_revision);

    f_compressions.swap(compressions);
    f_client_cache_control = cache_control;
}


/** \brief Verify the locale defined in \p lang.
 *
 * This function cuts the \p lang string in two strings: the language
 * name and one country.
 *
 * In most cases, when you call this function the language  parameter
 * (\p lang) includes the entire locale such as: de_DE or am-BY. It is
 * not mandatory so the country can also be define in which case it
 * should not appear in the \p lang parameter.
 *
 * \param[in,out] lang  The language to be verified.
 * \param[out] country  The resulting country if defined in \p lang.
 * \param[in] generate_errors  Whether to call die() on errors.
 *
 * \return true if no errors were detected.
 */
bool snap_child::verify_locale(QString & lang, QString & country, bool generate_errors)
{
    country.clear();

    // search for a '-' or '_' separator as in:
    //
    //    fr-ca   (browser locale)
    //    en_US   (Unix environment locale)
    //
    // initialize with 'ptr - 1' so we can ++ on entry of the while loop
    QChar const * s(lang.data() - 1);
    ushort c;
    do
    {
        ++s;
        c = s->unicode();
    }
    while(c != '\0' && c != '-' && c != '_');

    // if not '\0' then we have a country specified
    if(c != '\0')
    {
        if(!country.isEmpty())
        {
            if(generate_errors)
            {
                // country should always be empty on entry
                die(http_code_t::HTTP_CODE_BAD_REQUEST, "Country Defined Twice",
                        QString("Country is defined twice.").arg(lang).arg(c),
                        "This one may be a programmer mistake. The country parameter was not an empty string on entry of this function.");
                NOTREACHED();
            }
            return false;
        }
        int const pos(static_cast<int>(s - lang.data()));
        country = lang.mid(pos + 1);
        lang = lang.left(pos);
        if(lang.isEmpty())
        {
            if(generate_errors)
            {
                // we do not accept entries such as "-br" or "_RU"
                die(http_code_t::HTTP_CODE_NOT_FOUND, "Language Not Found",
                        QString("Language in the locale specification \"%1\" is not defined which is not supported.").arg(lang),
                        "Prevented user from accessing the website with no language specified, even though he specified a language entry.");
                NOTREACHED();
            }
            return false;
        }
        if(country.isEmpty())
        {
            if(generate_errors)
            {
                // we do not accept entries such as "br-" or "ru_"
                die(http_code_t::HTTP_CODE_NOT_FOUND, "Country Not Found",
                        QString("Country in the locale specification \"%1\" is not defined. Remove '%2' if you do not want to include a country.").arg(lang).arg(c),
                        "Prevented user from accessing the website with no country specified, even though he specified a country entry.");
                NOTREACHED();
            }
            return false;
        }
    }

    if(lang.isEmpty())
    {
        // use the default
        lang = "xx";
    }
    else
    {
        if(!verify_language_name(lang))
        {
            if(generate_errors)
            {
                // language is not valid; generate an error
                die(http_code_t::HTTP_CODE_NOT_FOUND, "Language Not Found",
                        QString("Language in the locale specification \"%1\" is not currently considered a known or valid language name.").arg(lang),
                        "Prevented user from accessing the website with an invalid language.");
                NOTREACHED();
            }
            return false;
        }
    }

    // country can be empty, that is fine; in most cases we recommend users
    // to not use the country name in their language
    if(!country.isEmpty())
    {
        if(country.contains('.'))
        {
            if(generate_errors)
            {
                // charset not supported here for now
                die(http_code_t::HTTP_CODE_NOT_FOUND, "Country Not Found",
                        QString("Locale specification \"%1\" seems to include a charset which is not legal at this time.").arg(lang),
                        "Prevented user from accessing the website because a charset was specified with the language.");
                NOTREACHED();
            }
            return false;
        }
        if(!verify_country_name(country))
        {
            if(generate_errors)
            {
                // abbreviation not found
                die(http_code_t::HTTP_CODE_NOT_FOUND, "Country Not Found",
                        QString("Country in locale specification \"%1\" does not look like a valid country name.").arg(lang),
                        "Prevented user from accessing the website with an invalid country.");
                NOTREACHED();
            }
            return false;
        }
    }

    return true;
}


/** \brief Verify that a name is a language name.
 *
 * This function checks whether \p lang is a valid language name. If so
 * then the function returns true and \p lang is set to the corresponding
 * 2 letter abbreviation for that country.
 *
 * Note that calling the function with an empty string will make it
 * return false.
 *
 * \warning
 * The input string is changed even if the language is not found.
 *
 * \param[in,out] lang  The language being checked.
 *
 * \return true if the language is valid, false in all other cases.
 */
bool snap_child::verify_language_name(QString & lang)
{
    // TODO: make use of a fully optimized search with a binary search like
    //       capability (i.e. a switch on a per character basis in a
    //       sub-function generated using the language table)
    //
    lang = lang.toLower();
    if(lang.length() == 2)
    {
        // we get the unicode value even though in the loop we compare
        // against ASCII; if l0 or l1 are characters outside of the ASCII
        // character set, then the loop will fail, simply
        ushort const l0(lang[0].unicode());
        ushort const l1(lang[1].unicode());
        for(language_name_t const *l(g_language_names); l->f_short_name[0]; ++l)
        {
            if(l0 == l->f_short_name[0]
            && l1 == l->f_short_name[1])
            {
                // got it!
                return true;
            }
        }
    }
    else
    {
        // TBD: do we really want to support long names?
        QString lang_with_commas("," + lang + ",");
        QByteArray lang_with_commas_buffer(lang_with_commas.toUtf8());
        char const * lwc(lang_with_commas_buffer.data());
        for(language_name_t const * l(g_language_names); l->f_language; ++l)
        {
            // the multi-name is already semi-optimize so we test it too
            // TBD -- this should maybe not be checked at all
            if(strstr(l->f_other_names, lwc) != nullptr)
            {
                lang = l->f_short_name;
                return true;
            }
        }
    }

    // not found
    return false;
}


/** \brief Verify that a name is a country name.
 *
 * This function checks whether \p country is a valid country name. If so
 * then the function returns true and \p country is set to the corresponding
 * 2 letter abbreviation for that country.
 *
 * Note that calling the function with an empty string will make it
 * return false.
 *
 * \warning
 * The input string is changed even if the country is not found.
 *
 * \param[in,out] country  The country being checked.
 *
 * \return true if the country is valid, false in all other cases.
 */
bool snap_child::verify_country_name(QString & country)
{
    country = country.toUpper();
    if(country.length() == 2)
    {
        // check abbreviations only
        ushort const c0 = country[0].unicode();
        ushort const c1 = country[1].unicode();
        for(country_name_t const *c(g_country_names); c->f_abbreviation[0]; ++c)
        {
            if(c0 == c->f_abbreviation[0] && c1 == c->f_abbreviation[1])
            {
                // found it!
                return true;
            }
        }
    }
    else
    {
        // check full names only
        //
        // TODO: this could be fully optimized with a set of switch
        //       statements and the full country name should be checked
        //       without the part after the comma, then in full with the
        //       part after the comma put in the front of the other
        //       part; i.e. "Bolivia, Plurinational State of" should
        //       be checked as any one of the following four entries:
        //
        //           BO  (abbreviation)
        //           Bolivia
        //           Plurinational State of Bolivia
        //           Bolivia, Plurinational State of
        //
        // TBD -- I'm not so sure we really want to support full names
        for(country_name_t const *c(g_country_names); c->f_abbreviation[0]; ++c)
        {
            if(QString(c->f_name).toUpper() == country)
            {
                country = c->f_abbreviation;
                return true;
            }
        }
    }

    return false;
}


/** \brief Check whether a site needs to be redirected.
 *
 * This function verifies the site we just discovered to see whether
 * the user requested a redirect. If so, then we replace the
 * f_site_key accordingly.
 *
 * Note that this is not a 301 redirect, just an internal remap from
 * site A to site B.
 */
void snap_child::site_redirect()
{
    libdbproxy::value redirect(get_site_parameter(get_name(name_t::SNAP_NAME_CORE_REDIRECT)));
    if(redirect.nullValue())
    {
        // no redirect
        return;
    }

    // redirect now
    f_site_key = redirect.stringValue();

    // TBD -- should we also redirect the f_domain_key and f_website_key?

    // the site table is the old one, we want to switch to the new one
    reset_sites_table();
}


/** \brief Reset the site table so one can make sure to use the latest version.
 *
 * In a backend that does not restart all the time, we may need to reset
 * the site table.
 *
 * This function clears the memory cache of the existing site table, if
 * one exists in memory, and then it resets the site table pointer.
 * The next time a user calls the set_site_parameter() or get_site_parameter()
 * a new site table object is created and filled as required.
 */
void snap_child::reset_sites_table()
{
    f_sites_table.reset();
}


/** \brief Redirect the user to a new page.
 *
 * This function forcibly redirects a user to a new page. If the path
 * includes a protocol (is a full URI) then it is used as is. If
 * the path includes no protocol, the current site key is prepended.
 *
 * The HTTP code can be specified. By default, 301 is assumed because
 * that's the most prominent redirect code used. If a page is used to
 * redirect dynamically, make sure to use 302 or 303 instead. You can
 * safely use one of the following codes:
 *
 * \li HTTP_CODE_MOVED_PERMANENTLY (301)
 * \li HTTP_CODE_FOUND (302)
 * \li HTTP_CODE_SEE_OTHER (303) -- POST becomes GET
 * \li HTTP_CODE_TEMPORARY_REDIRECT (307) -- keep same method
 * \li HTTP_CODE_PERMANENT_REDIRECT (308) -- keep same method
 *
 * The path may include a query string and an anchor.
 *
 * \note
 * This function does not allow redirecting an application which used a
 * method other than GET, HEAD, and POST. Applications are expected to
 * get their URI right.
 *
 * \warning
 * The function does not return since after sending a redirect to a client
 * there is nothing more you can do. So if you need to save some data, make
 * sure to do it before this call. Note also that this function calls the
 * attach_to_session() signal so any plugin that was not really done has a
 * chance to save its data until the next connection arrives.
 *
 * \param[in] path  The full URI or the local website path to redirect the
 *                  user browser to.
 * \param[in] http_code  The code used to send the redirect.
 * \param[in] reason_brief  A brief explanation for the redirection.
 * \param[in] reason  The long version of the explanation for the redirection.
 */
void snap_child::page_redirect(QString const & path, http_code_t http_code, QString const & reason_brief, QString const & reason)
{
    if(f_site_key_with_slash.isEmpty())
    {
        die(http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR, "Initialization Mismatch",
                "An internal server error was detected while initializing the process.",
                "The server snap_child::page_redirect() function was called before the website got canonicalized.");
        NOTREACHED();
    }
    QString const method(snapenv(get_name(name_t::SNAP_NAME_CORE_REQUEST_METHOD)));
    if(method != "GET"
    && method != "POST"
    && method != "HEAD")
    {
        die(http_code_t::HTTP_CODE_FORBIDDEN, "Method Not Allowed",
                QString("Prevented a redirect when using method %1.").arg(method),
                "We do not currently support redirecting users for methods other than GET, POST, and HEAD.");
        NOTREACHED();
    }

    switch(http_code)
    {
    case http_code_t::HTTP_CODE_MOVED_PERMANENTLY:
    case http_code_t::HTTP_CODE_FOUND:
    case http_code_t::HTTP_CODE_SEE_OTHER:
    case http_code_t::HTTP_CODE_TEMPORARY_REDIRECT:
    case http_code_t::HTTP_CODE_PERMANENT_REDIRECT:
        break;

    default:
        die(http_code_t::HTTP_CODE_FORBIDDEN, "Error Code Not Allowed For Redirect",
                QString("Prevented a redirect using HTTP code %1.").arg(static_cast<int>(http_code)),
                "We limit the redirect to using 301, 302, 303, 307, and 308.");
        NOTREACHED();

    }

    if(path.contains('\n') || path.contains('\r'))
    {
        // if the path includes a \n or \r then the user could inject
        // a header which could have all sorts of effects we don't even
        // want to think about! just deny it...
        die(http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR, "Hack Prevention",
                "Server prevented a potential hack from being applied.",
                "The server snap_child::page_redirect() function was called with a path that includes \n or \r and refused processing it: \"" + path + "\"");
        NOTREACHED();
    }

    snap_uri uri;
    if(!uri.set_uri(path))
    {
        // in most cases it fails because the protocol is missing
        QString local_path(path);
        canonicalize_path(local_path);
        if(!uri.set_uri(get_site_key_with_slash() + local_path))
        {
            die(http_code_t::HTTP_CODE_ACCESS_DENIED, "Invalid URI",
                    "The server prevented a redirect because it could not understand the destination URI.",
                    "The server snap_child::page_redirect() function was called with a path that it did not like: \"" + path + "\"");
            NOTREACHED();
        }
    }

    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    server->attach_to_session();

    // redirect the user to the specified path
    QString http_name;
    define_http_name(http_code, http_name);

    set_header("Status", QString("%1 %2")
                    .arg(static_cast<int>(http_code))
                    .arg(http_name), HEADER_MODE_REDIRECT);

    snap_string_list const show_redirects(server->get_parameter("show_redirects").split(","));

    if(!show_redirects.contains("refresh-only"))
    {
        // the get_uri() returns an HTTP encoded string
        set_header("Location", uri.get_uri(), HEADER_MODE_REDIRECT);
    }

    // also the default is already text/html we force it again in case this
    // function is called after someone changed this header
    set_header(get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER), "text/html; charset=utf-8", HEADER_MODE_EVERYWHERE);

    // compute the body
    // (TBD: should we support getting the content of a page? since 99.9999% of
    // the time this content is ignored, I would say no.)
    //
    QString body;
    {
        QDomDocument doc;
        // html
        QDomElement html(doc.createElement("html"));
        doc.appendChild(html);
        // html/head
        QDomElement head(doc.createElement("head"));
        html.appendChild(head);
        // html/head/meta[@http-equiv=...][@content=...]
        QDomElement meta_locale(doc.createElement("meta"));
        head.appendChild(meta_locale);
        meta_locale.setAttribute("http-equiv", get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER));
        meta_locale.setAttribute("content", "text/html; charset=utf-8");
        // html/head/title/...
        QDomElement title(doc.createElement("title"));
        head.appendChild(title);
        QDomText title_text(doc.createTextNode(reason_brief));
        title.appendChild(title_text);
        // html/head/meta[@http-equiv=...][@content=...]
        if(show_redirects.contains("refresh-only")
        || !show_redirects.contains("no-refresh"))
        {
            QDomElement meta_refresh(doc.createElement("meta"));
            head.appendChild(meta_refresh);
            meta_refresh.setAttribute("http-equiv", "Refresh");
            int timeout(0);
            if(show_redirects.contains("one-minute"))
            {
                timeout = 60;
            }
            meta_refresh.setAttribute("content", QString("%1; url=%2").arg(timeout).arg(uri.get_uri()));
        }
        // html/head/meta[@http-equiv=...][@content=...]
        QDomElement meta_robots(doc.createElement("meta"));
        head.appendChild(meta_robots);
        meta_robots.setAttribute("name", "ROBOTS");
        meta_robots.setAttribute("content", "NOINDEX");
        // html/body
        QDomElement body_tag(doc.createElement("body"));
        html.appendChild(body_tag);

        // include an actual body?
        if(show_redirects.contains("include-body"))
        {
            // html/body/h1
            QDomElement h1(doc.createElement("h1"));
            body_tag.appendChild(h1);
            QDomText h1_text(doc.createTextNode(reason_brief));
            h1.appendChild(h1_text);
            // html/body/p
            QDomElement p(doc.createElement("p"));
            body_tag.appendChild(p);
            QDomText p_text1(doc.createTextNode(QString("%1 New location: ").arg(reason)));
            p.appendChild(p_text1);
            // html/body/a
            QDomElement a(doc.createElement("a"));
            a.setAttribute("href", uri.get_uri());
            p.appendChild(a);
            QDomText a_text(doc.createTextNode(uri.get_uri()));
            a.appendChild(a_text);
            // html/body/p (text after anchor)
            QDomText p_text2(doc.createTextNode("."));
            p.appendChild(p_text2);
        }

        // save the result
        body = doc.toString(-1);
    }

    output_result(HEADER_MODE_REDIRECT, body.toUtf8());

    // XXX should we exit with 1 in this case?
    exit(0);
    NOTREACHED();
}


/** \brief Attach variables to this session.
 *
 * Once in a while a plugin creates a form that is intermediary. In this
 * case the session variables need to be saved and this function is called.
 *
 * Note that you may want to look into not detaching the variable(s) if at
 * all possible.
 */
void snap_child::attach_to_session()
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    server->attach_to_session();
}


/** \brief Load a file.
 *
 * This signal is sent to all the plugins so one can load the file as
 * defined in the file filename.
 *
 * The function returns true if the file was loaded successfully.
 *
 * \param[in,out] file  The file that is to be loaded.
 *
 * \return true if the file was found, false otherwise.
 */
bool snap_child::load_file(post_file_t & file)
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    bool found(false);
    server->load_file(file, found);
    return found;
}


/** \brief Retrieve an environment variable.
 *
 * This function can be used to read an environment variable. It will make
 * sure, in most cases, that the variable is not tented.
 *
 * At this point only the variables defined in the HTTP request are available.
 * Any other variable name will return an empty string.
 *
 * The SERVER_PROTOCOL variable can be retrieved at any time, even before we
 * read the environment. This is done so we can call the die() function and
 * return with a valid protocol and version.
 *
 * \param[in] name  The name of the variable to retrieve.
 *
 * \return The value of the specified variable.
 */
QString snap_child::snapenv(QString const & name) const
{
    if(name == get_name(name_t::SNAP_NAME_CORE_SERVER_PROTOCOL))
    {
        // SERVER PROTOCOL
        if(false == f_fixed_server_protocol)
        {
            f_fixed_server_protocol = true;
            // Drupal does the following
            // TBD can the SERVER_PROTOCOL really be wrong?
            if(f_env.count(get_name(name_t::SNAP_NAME_CORE_SERVER_PROTOCOL)) != 1)
            {
                // if undefined, set a default protocol
                const_cast<snap_child *>(this)->f_env[get_name(name_t::SNAP_NAME_CORE_SERVER_PROTOCOL)] = "HTTP/1.0";
            }
            else
            {
                // note that HTTP/0.9 could be somewhat supported but that's
                // most certainly totally useless
                if("HTTP/1.0" != f_env.value(get_name(name_t::SNAP_NAME_CORE_SERVER_PROTOCOL))
                && "HTTP/1.1" != f_env.value(get_name(name_t::SNAP_NAME_CORE_SERVER_PROTOCOL)))
                {
                    // environment is no good!?
                    const_cast<snap_child *>(this)->f_env[get_name(name_t::SNAP_NAME_CORE_SERVER_PROTOCOL)] = "HTTP/1.0";
                }
            }
        }
        return f_env.value(get_name(name_t::SNAP_NAME_CORE_SERVER_PROTOCOL), "HTTP/1.0");
    }

    return f_env.value(name, "");
}


/** \brief Check whether a POST variable was defined.
 *
 * Check whether the named POST variable was defined. This can be useful if
 * you have some optional fields in a form. Also in some places where the
 * code does not know about all the widgets.
 *
 * Note that the functions that directly access the post environment should
 * not be used by most as the form plugin already does what is necessary.
 *
 * \param[in] name  The name of the POST variable to fetch.
 *
 * \return true if the value is defined, false otherwise.
 */
bool snap_child::postenv_exists(QString const & name) const
{
    return f_post.contains(name);
}


/** \brief Retrieve a POST variable.
 *
 * Return the content of one of the POST variables. Post variables are defined
 * only if the method used to access the site was a POST.
 *
 * \warning
 * This function returns the RAW data from a POST. You should instead use the
 * data returned by your form which will have been validated and fixed up as
 * required (decoded, etc.)
 *
 * \param[in] name  The name of the POST variable to fetch.
 * \param[in] default_value  The value to return if the POST variable is not defined.
 *
 * \return The value of that POST variable or the \p default_value.
 */
QString snap_child::postenv(QString const & name, QString const & default_value) const
{
    return f_post.value(name, default_value);
}


/** \brief Replace the value of a POST variable.
 *
 * Once in a while you may want to change a POST variable. One reason might
 * be to apply a filter on a field before it gets saved in the database. This
 * is accessible by any plugin so any one of those can do some work as
 * required.
 *
 * \param[in] name  The name of the POST variable to fetch.
 * \param[in] value  The new value for this POST variable.
 */
void snap_child::replace_postenv(QString const & name, QString const & value)
{
    f_post[name] = value;
}


/** \brief Check whether a file from the POST request is defined.
 *
 * This function is expected to be called to verify that a file was
 * indeed uploaded for the named widget.
 *
 * If this function returns false, then the postfile() function should not
 * be called.
 *
 * \param[in] name  The id (name) of the Input File widget.
 *
 * \sa postfile()
 */
bool snap_child::postfile_exists(QString const & name) const
{
    // the QMap only returns a reference if this is not constant
    //
    return f_files.contains(name) && f_files[name].get_size() != 0;
}


/** \brief Retrieve a file from the POST.
 *
 * This function can be called if this request included a POST with a
 * file attached.
 *
 * Note that the files are saved by widget identifier. This means if
 * you check a post with postenv("file") (which returns the filename),
 * then you can get the actual file with postfile("file").
 *
 * \note
 * If the file was not sent by the browser, then this function creates
 * an entry in the global table which is probably not what you want.
 * Make sure to call the postfile_exists() function first.
 *
 * \param[in] name  The id (name) of the Input File widget.
 *
 * \sa postfile_exists()
 */
const snap_child::post_file_t& snap_child::postfile(QString const & name) const
{
    // the QMap only returns a reference if this is not constant
    return const_cast<snap_child *>(this)->f_files[name];
}


/** \brief Check whether a cookie was sent to us by the browser.
 *
 * This function checks whether a cookie was defined and returned by the
 * browser. This is different from testing whether the value returned by
 * cookie() is an empty string.
 *
 * \note
 * Doing a set_cookie() does not interfere with this list of cookies
 * which represent the list of cookies the browser sent to us.
 *
 * \param[in] name  The name of the cookie to retrieve.
 *
 * \return true if the cookie was sent to us, false otherwise.
 */
bool snap_child::cookie_is_defined(QString const & name) const
{
    return f_browser_cookies.contains(name);
}


/** \brief Return the contents of a cookie.
 *
 * This function returns the contents of the named cookie.
 *
 * Note that this function is not the counterpart of the set_cookie()
 * function. The set_cookie() accepts an http_cookie object, whereas
 * this function only returns a string (because that's all we get
 * from the browser.)
 *
 * \param[in] name  The name of the cookie to retrieve.
 *
 * \return The content of the cookie, an empty string if the cookie is not defined.
 */
QString snap_child::cookie(QString const & name) const
{
    if(f_browser_cookies.contains(name))
    {
        return f_browser_cookies[name];
    }
    return QString();
}


/** \brief Make sure to clean up then exit the child process.
 *
 * This function cleans up the child and then calls the
 * server::exit() function to give the server a chance to
 * also clean up. Then it exists by calling the exit(3)
 * function of the C library.
 *
 * \param[in] code  The exit code, generally 0 or 1.
 */
void snap_child::exit(int code)
{
    // if we have a messenger, make sure to get rid of it
    //
    stop_messenger();

    // make sure the socket data is pushed to the caller
    //
    f_client.reset();

    // after we close the socket the answer is sent to the client so
    // we can take a little time to gather some statistics.
    //
    server::pointer_t server( f_server.lock() );
    if(server)
    {
        server->udp_rusage("snap_child");
    }

    server::exit(code);
    NOTREACHED();
}


/** \brief Check whether the server was started in debug mode.
 *
 * With this function any plugin can determine whether the server was
 * started with the --debug command line option and act accordingly
 * (i.e. show a certain number of debug in stdout or stderr).
 *
 * It should not be used to display debug data in the HTML output.
 * Other parameters can be used for that purpose (specifically, that
 * very plugin can make use of its own debug parameter for that because
 * having the debug turned on for all modules would be a killer.)
 *
 * \return true if the --debug (-d) command line option was used to start
 *         the server
 */
bool snap_child::is_debug() const
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    return server->is_debug();
}


/** \brief This function tells you whether the child is ready or not.
 *
 * The child prepares the database using a lot of complicated add_content()
 * calls in the content plugin. Once all those calls were made, the system
 * finally tells the world that it is ready and then it triggers the
 * execute() signal.
 *
 * The flag is false up until just before the execute() function of
 * the server gets called. It is a good way to know whether you are
 * still in the initialization process or you are already working
 * from within the path plugin (which is the one plugin that captures
 * the execute() signal).
 *
 * \return true if the snap_child object triggered the execute() signal.
 */
bool snap_child::is_ready() const
{
    return f_ready;
}


/** \brief Retrieve the library (server) version.
 *
 * Retrieve the version of the snapwebsites library.
 *
 * \return A pointer to the version of the running server library.
 */
char const * snap_child::get_running_server_version()
{
    return server::version();
}


/** \brief Check whether the plugin is considered a Core Plugin.
 *
 * Plugins can be forced defined by the administrator in the
 * snapserver.conf under the name "plugins". In that case,
 * only those specific plugins are loaded. This is quite practicle
 * if a plugin is generating problems and you cannot otherwise
 * check out your website.
 *
 * The list of plugins can be soft defined in the default_plugins
 * variable. This variable is used by new websites until the
 * user adds and removes plugins to his website by editing the
 * list of plugins via the Plugin Selector ("/admin/plugin").
 *
 * However, in all cases, the system will not work if you do not
 * have a certain number of low level plugins running such as the
 * content and users plugins. These are considered Core Plugins.
 * This function returns true whenever the specified \p name
 * represents a Core Plugin.
 *
 * \param[in] name  The name of the plugin to check.
 *
 * \return true if the plugin is a Core Plugin.
 */
bool snap_child::is_core_plugin(QString const & name) const
{
    // a special case because "server" is not listed in the g_minimum_plugins
    // list (because it is already loaded since it is an object in the library)
    //
    if(name == "server")
    {
        return true;
    }

    // TODO: make sure the table is sorted alphabetically and use
    //       a binary search
    //
    for(size_t i(0); i < sizeof(g_minimum_plugins) / sizeof(g_minimum_plugins[0]); ++i)
    {
        if(name == g_minimum_plugins[i])
        {
            return true;
        }
    }

    return false;
}


/** \brief Retrieve a server parameter.
 *
 * This function calls the get_parameter() function of the server. This
 * gives you access to all the parameters defined in the server
 * configuration file.
 *
 * This gives you access to parameters such as the qs_action and
 * the default_plugins list.
 *
 * \param[in] name  The name of the parameter to retrieve.
 */
QString snap_child::get_server_parameter(QString const & name)
{
#ifdef DEBUG
    if(name.isEmpty())
    {
        throw snap_logic_exception("get_server_parameter() called with an empty string as the name of the parameter to be retrieved");
    }
#endif
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    return server->get_parameter(name);
}


/** \brief Retrieve the path to the list data.
 *
 * This function retrieve the path to the data used by the list environment.
 * The list plugin and backends make use of this path to handle the
 * journal and database that they manage.
 *
 * By default the path is `"/var/lib/snapwebsites/list"`.
 *
 * \return The path to the list data directory.
 */
QString snap_child::get_list_data_path()
{
    QString const path(get_server_parameter(get_name(name_t::SNAP_NAME_CORE_LIST_DATA_PATH)));

    // not defined by end user, return the default value
    //
    return path.isEmpty() ? "/var/lib/snapwebsites/list" : path;
}


/** \brief Improve signature to place at the bottom of a page.
 *
 * This signal can be used to generate a signature to place at the bottom
 * of a page. In most cases, this very simple signature function is only
 * used on error pages, although plugins that generate rather generic
 * content are welcome to use it too.
 *
 * \param[in] path  The path to the page that is being generated.
 * \param[in] doc  The document holding the data.
 * \param[in,out] signature_tag  The signature tag which is to be improved.
 */
void snap_child::improve_signature(QString const & path, QDomDocument doc, QDomElement signature_tag)
{
    server::pointer_t server(f_server.lock());
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    return server->improve_signature(path, doc, signature_tag);
}


/** \brief Retreive a website wide parameter.
 *
 * This function reads a column from the sites table using the site key as
 * defined by the canonicalization process. The function cannot be called
 * before the canonicalization process ends.
 *
 * The table is opened once and remains opened so calling this function
 * many times is not a problem. Also the libQtCassandra library caches
 * all the data. Reading the same field multiple times is not a concern
 * at all.
 *
 * If the value is undefined, the result is a null value.
 *
 * \param[in] name  The name of the parameter to retrieve.
 *
 * \return The content of the row as a Cassandra value.
 */
libdbproxy::value snap_child::get_site_parameter(QString const & name)
{
    // retrieve site table if not there yet
    if(!f_sites_table)
    {
        QString const table_name(get_name(name_t::SNAP_NAME_SITES));
        libdbproxy::table::pointer_t table(f_context->findTable(table_name));
        if(!table)
        {
            // the whole table is still empty
            libdbproxy::value value;
            return value;
        }
        f_sites_table = table;
    }

    if(!f_sites_table->exists(f_site_key))
    {
        // an empty value is considered to be a null value
        libdbproxy::value value;
        return value;
    }
    libdbproxy::row::pointer_t row(f_sites_table->getRow(f_site_key));
    if(!row->exists(name))
    {
        // an empty value is considered to be a null value
        libdbproxy::value value;
        return value;
    }

    return row->getCell(name)->getValue();
}


/** \brief Save a website wide parameter.
 *
 * This function writes a column to the sites table using the site key as
 * defined by the canonicalization process. The function cannot be called
 * before the canonicalization process ends.
 *
 * The table is opened once and remains opened so calling this function
 * many times is not a problem.
 *
 * If the value was still undefined, then it is created.
 *
 * \param[in] name  The name of the parameter to save.
 * \param[in] value  The new value for this parameter.
 */
void snap_child::set_site_parameter(QString const & name, libdbproxy::value const & value)
{
    // retrieve site table if not there yet
    //
    if(!f_sites_table)
    {
        // get a pointer to the "sites" table
        //
        f_sites_table = get_table(get_name(name_t::SNAP_NAME_SITES));
    }

    f_sites_table->getRow(f_site_key)->getCell(name)->setValue(value);
}


/** \brief Retrieve a current copy of the output buffer.
 *
 * This function returns a copy of the current snap_child output buffer.
 *
 * The buffer cannot be changed using the returned buffer. Use the
 * write() functions to appened to the buffer.
 *
 * \return A copy of the output buffer.
 */
QByteArray snap_child::get_output() const
{
    return f_output.buffer();
}


/** \brief Write the buffer to the output.
 *
 * This function writes the specified buffer (array of bytes) to
 * the output of the snap child. When the execute function returns
 * from running all the plugins, the data in the buffer is sent to
 * Apache (through snap.cgi).
 *
 * This function is most often used when the process is replying with
 * data other than text (i.e. images, PDF documents, etc.)
 *
 * \param[in] data  The array of byte to append to the buffer.
 */
void snap_child::output(QByteArray const & data)
{
    f_output.write(data);
}


/** \brief Write the string to the output buffer.
 *
 * This function writes the specified string to the output buffer
 * of the snap child. When the execute function returns from running
 * all the plugins, the data in the buffer is sent to Apache.
 *
 * The data is always written in UTF-8.
 *
 * \param[in] data  The string data to append to the buffer.
 */
void snap_child::output(QString const & data)
{
    f_output.write(data.toUtf8());
}


/** \brief Write the string to the output buffer.
 *
 * This function writes the specified string to the output buffer
 * of the snap child. When the execute function returns from running
 * all the plugins, the data in the buffer is sent to Apache.
 *
 * The data is viewed as UTF-8 characters and it is sent as is to the
 * buffer.
 *
 * \param[in] data  The string data to append to the buffer.
 */
void snap_child::output(std::string const & data)
{
    f_output.write(data.c_str(), data.length());
}


/** \brief Write the string to the output buffer.
 *
 * This function writes the specified string to the output buffer
 * of the snap child. When the execute function returns from running
 * all the plugins, the data in the buffer is sent to Apache.
 *
 * The data is viewed as UTF-8 characters and it is sent as is to the
 * buffer.
 *
 * \param[in] data  The string data to append to the buffer.
 */
void snap_child::output(char const * data)
{
    f_output.write(data);
}


/** \brief Write the string to the output buffer.
 *
 * This function writes the specified string to the output buffer
 * of the snap child. When the execute function returns from running
 * all the plugins, the data in the buffer is sent to Apache.
 *
 * The data is viewed as UTF-8 characters and it is sent as is to the
 * buffer.
 *
 * \param[in] data  The string data to append to the buffer.
 */
void snap_child::output(wchar_t const * data)
{
    f_output.write(QString::fromWCharArray(data).toUtf8());
}


/** \brief Check whether someone wrote any output yet.
 *
 * This function checks whether any output was written or not.
 *
 * \return true if the output buffer is still empty.
 */
bool snap_child::empty_output() const
{
    return f_output.buffer().size() == 0;
}


/** \brief Trace in case we are initializing the website.
 *
 * While initializing (when a request was sent using #INIT instead of
 * #START) then we immediately send data back using the trace() functions.
 *
 * \param[in] data  The data to send to the listener.
 *                  Generally a printable string.
 */
void snap_child::trace(QString const & data)
{
    trace(std::string(data.toUtf8().data()));
}


/** \brief Trace in case we are initializing the website.
 *
 * While initializing (when a request was sent using #INIT instead of
 * #START) then we immediately send data back using the trace() functions.
 *
 * \param[in] data  The data to send to the listener.
 *                  Generally a printable string.
 */
void snap_child::trace(std::string const & data)
{
    if(f_is_being_initialized)
    {
        // keep a copy in the server logs too
        //
        if(data.back() == '\n')
        {
            SNAP_LOG_INFO("trace() from installation: ")(data.substr(0, data.length() - 1));
        }
        else
        {
            SNAP_LOG_INFO("trace() from installation: ")(data);
        }

        write(data.c_str(), data.size());
    }
}


/** \brief Trace in case we are initializing the website.
 *
 * While initializing (when a request was sent using #INIT instead of
 * #START) then we immediately send data back using the trace() functions.
 *
 * \param[in] data  The data to send to the listener.
 *                  Generally a printable string.
 */
void snap_child::trace(char const * data)
{
    trace(std::string(data));
}


/** \brief Generate an HTTP error and exit the child process.
 *
 * This function kills the child process after sending an HTTP
 * error message to the user and to the logger.
 *
 * The \p err_name parameter is optional in that it can be set to
 * the empty string ("") and let the die() function make use of
 * the default error message for the specified \p err_code.
 *
 * The error description message can include HTML tags to change
 * the basic format of the text (i.e. bold, italic, underline, and
 * other inline tags.) The message is printed inside a paragraph
 * tag (<p>) and thus it should not include block tags.
 * The message is expected to be UTF-8 encoded, although in general
 * it should be in English so only using ASCII.
 *
 * The \p err_details parameter is the message to write to the
 * log. It should be as detailed as possible so it makes it
 * easy to know what's wrong and eventually needs attention.
 *
 * \note
 * You can trick the description paragraph by adding a closing
 * paragraph tag (</p>) at the start and an opening paragraph
 * tag (<p>) at the end of your description.
 *
 * \warning
 * This function does NOT return. It calls exit(1) once done.
 *
 * \param[in] err_code  The error code such as 501 or 503.
 * \param[in] err_name  The name of the error such as "Service Not Available".
 * \param[in] err_description  HTML message about the problem.
 * \param[in] err_details  Server side text message with details that are logged only.
 */
void snap_child::die(http_code_t err_code, QString err_name, QString const & err_description, QString const & err_details)
{
    if(f_died)
    {
        // avoid loops
        return;
    }
    f_died = true;

    try
    {
        // define a default error name if undefined
        define_http_name(err_code, err_name);

        // log the error
        SNAP_LOG_FATAL("snap child process: ")(err_details)(" (")(static_cast<int>(err_code))(" ")(err_name)(": ")(err_description)(")");

        if(f_is_being_initialized)
        {
            // send initialization process the info about the error
            trace(QString("Error: die() called: %1 (%2 %3: %4)\n").arg(err_details).arg(static_cast<int>(err_code)).arg(err_name).arg(err_description));
            trace("#END\n");
        }
        else
        {
            // On error we do not return the HTTP protocol, only the Status field
            // it just needs to be first to make sure it works right
            //
            // IMPORTANT NOTE: we WANT the header to be set when we call
            //                 the attach_to_session() function so someone
            //                 can at least peruse it
            //
            set_header(get_name(name_t::SNAP_NAME_CORE_STATUS_HEADER),
                       QString("%1 %2").arg(static_cast<int>(err_code)).arg(err_name),
                       HEADER_MODE_ERROR);

            // Make sure that the session is re-attached
            if(f_cassandra)
            {
                server::pointer_t server( f_server.lock() );
                if(!server)
                {
                    throw snap_logic_exception("server pointer is nullptr");
                }
                server->attach_to_session();
            }

            // content type is HTML, we reset this header because it could have
            // been changed to something else and prevent the error from showing
            // up in the browser
            set_header(get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER),
                       "text/html; charset=utf8",
                       HEADER_MODE_EVERYWHERE);

            // TODO: the HTML could also come from a user defined
            //       page so that way it can get a translated
            //       message without us having to do anything
            //       (but probably only for 403 and 404 pages?)

            QString const html(error_body(err_code, err_name, err_description));

            // in case there are any cookies, send them along too
            output_result(HEADER_MODE_ERROR, html.toUtf8());
        }
    }
    catch(std::exception const & e)
    {
        // ignore all errors because at this point we must die quickly.
        SNAP_LOG_FATAL("snap_child.cpp:die(): try/catch caught an exception. What: ")(e.what());
    }
    catch(...)
    {
        // ignore all errors because at this point we must die quickly.
        SNAP_LOG_FATAL("snap_child.cpp:die(): try/catch caught an exception");
    }

    // exit with an error
    exit(1);
}


/** \brief Create the body of an HTTP error.
 *
 * This function generates an error page for an HTTP error. Thus far,
 * it is used by the die() function and an equivalent in the attachment
 * plugin.
 *
 * \param[in] err_code  The error code such as 501 or 503.
 * \param[in] err_name  The name of the error such as "Service Not Available".
 * \param[in] err_description  HTML message about the problem.
 */
QString snap_child::error_body(http_code_t err_code, QString const & err_name, QString const & err_description)
{
    QString const title(QString("%1 %2").arg(static_cast<int>(err_code)).arg(err_name));

    // html
    QDomDocument doc;
    QDomElement html(doc.createElement("html"));
    doc.appendChild(html);

    // html/head
    QDomElement head(doc.createElement("head"));
    html.appendChild(head);

    // html/head/meta[@http-equiv=...][@content=...]
    QDomElement meta_locale(doc.createElement("meta"));
    head.appendChild(meta_locale);
    meta_locale.setAttribute("http-equiv", get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER));
    meta_locale.setAttribute("content", "text/html; charset=utf-8");

    // html/head/meta[@name=...][@content=...]
    QDomElement meta_robots(doc.createElement("meta"));
    head.appendChild(meta_robots);
    meta_robots.setAttribute("name", "ROBOTS");
    meta_robots.setAttribute("content", "NOINDEX");

    // html/head/title/...
    QDomElement title_tag(doc.createElement("title"));
    head.appendChild(title_tag);
    snap_dom::append_plain_text_to_node(title_tag, title);

    // html/head/style/...
    QDomElement style_tag(doc.createElement("style"));
    head.appendChild(style_tag);
    snap_dom::append_plain_text_to_node(style_tag, "body{font-family:sans-serif}");

    // html/body
    QDomElement body_tag(doc.createElement("body"));
    html.appendChild(body_tag);

    // html/body/h1/...
    QDomElement h1_tag(doc.createElement("h1"));
    h1_tag.setAttribute("class", "error");
    body_tag.appendChild(h1_tag);
    snap_dom::append_plain_text_to_node(h1_tag, title);

    // html/body/p[@class=description]/...
    QDomElement description_tag(doc.createElement("p"));
    description_tag.setAttribute("class", "description");
    body_tag.appendChild(description_tag);
    // the description may include HTML tags
    snap_dom::insert_html_string_to_xml_doc(description_tag, err_description);

    // html/body/p[@class=signature]/...
    QDomElement signature_tag(doc.createElement("p"));
    signature_tag.setAttribute("class", "signature");
    body_tag.appendChild(signature_tag);

    // now generate the signature tag anchors
    QString const site_key(get_site_key());
    if(f_cassandra)
    {
        libdbproxy::value const site_name(get_site_parameter(get_name(name_t::SNAP_NAME_CORE_SITE_NAME)));
        QDomElement a_tag(doc.createElement("a"));
        a_tag.setAttribute("class", "home");
        a_tag.setAttribute("target", "_top");
        a_tag.setAttribute("href", site_key);
        signature_tag.appendChild(a_tag);
        snap_dom::append_plain_text_to_node(a_tag, site_name.stringValue());

        improve_signature(f_uri.path(), doc, signature_tag);
    }
    else if(!site_key.isEmpty())
    {
        QDomElement a_tag(doc.createElement("a"));
        a_tag.setAttribute("class", "home");
        a_tag.setAttribute("target", "_top");
        a_tag.setAttribute("href", site_key);
        signature_tag.appendChild(a_tag);
        snap_dom::append_plain_text_to_node(a_tag, site_key);

        improve_signature(f_uri.path(), doc, signature_tag);
    }
    // else -- no signature...

    return doc.toString(-1);
}


/** \brief Ensure that the http_name variable is not empty.
 *
 * This function sets the content of the \p http_name variable if empty. It
 * uses the \p http_code value to define a default message in \p http_name.
 *
 * If the \p http_name string is not empty then it is not modified.
 *
 * \param[in] http_code  The code used to determine the http_name value.
 * \param[in,out] http_name  The http request name to set if not already defined.
 */
void snap_child::define_http_name(http_code_t http_code, QString & http_name)
{
    if(http_name.isEmpty())
    {
        switch(http_code)
        {
        // 1xx
        case http_code_t::HTTP_CODE_CONTINUE:                               http_name = "Continue"; break;
        case http_code_t::HTTP_CODE_SWITCHING_PROTOCOLS:                    http_name = "Switching Protocols"; break;
        case http_code_t::HTTP_CODE_PROCESSING:                             http_name = "Processing"; break;

        // 2xx
        case http_code_t::HTTP_CODE_OK:                                     http_name = "OK"; break;
        case http_code_t::HTTP_CODE_CREATED:                                http_name = "Created"; break;
        case http_code_t::HTTP_CODE_ACCEPTED:                               http_name = "Accepted"; break;
        case http_code_t::HTTP_CODE_NON_AUTHORITATIVE_INFORMATION:          http_name = "Non-Authoritative Information"; break;
        case http_code_t::HTTP_CODE_NO_CONTENT:                             http_name = "No Content"; break;
        case http_code_t::HTTP_CODE_RESET_CONTENT:                          http_name = "Reset Content"; break;
        case http_code_t::HTTP_CODE_PARTIAL_CONTENT:                        http_name = "Partial Content"; break;
        case http_code_t::HTTP_CODE_MULTI_STATUS:                           http_name = "Multi-Status"; break;
        case http_code_t::HTTP_CODE_ALREADY_REPORTED:                       http_name = "Already Reported"; break;
        case http_code_t::HTTP_CODE_IM_USED:                                http_name = "Instance-Manipulation Used"; break;

        // 3xx
        case http_code_t::HTTP_CODE_MULTIPLE_CHOICE:                        http_name = "Multiple Choice"; break;
        case http_code_t::HTTP_CODE_MOVED_PERMANENTLY:                      http_name = "Moved Permanently"; break;
        case http_code_t::HTTP_CODE_FOUND:                                  http_name = "Found"; break;
        case http_code_t::HTTP_CODE_SEE_OTHER:                              http_name = "See Other"; break; // POST becomes GET
        case http_code_t::HTTP_CODE_NOT_MODIFIED:                           http_name = "Not Modified"; break;
        case http_code_t::HTTP_CODE_USE_PROXY:                              http_name = "Use Proxy"; break;
        case http_code_t::HTTP_CODE_SWITCH_PROXY:                           http_name = "Switch Proxy"; break;
        case http_code_t::HTTP_CODE_TEMPORARY_REDIRECT:                     http_name = "Temporary Redirect"; break; // keep same method
        case http_code_t::HTTP_CODE_PERMANENT_REDIRECT:                     http_name = "Permanent Redirect"; break; // keep same method

        // 4xx
        case http_code_t::HTTP_CODE_BAD_REQUEST:                            http_name = "Bad Request"; break;
        case http_code_t::HTTP_CODE_UNAUTHORIZED:                           http_name = "Unauthorized"; break;
        case http_code_t::HTTP_CODE_PAYMENT_REQUIRED:                       http_name = "Payment Required"; break;
        case http_code_t::HTTP_CODE_FORBIDDEN:                              http_name = "Forbidden"; break;
        case http_code_t::HTTP_CODE_NOT_FOUND:                              http_name = "Not Found"; break;
        case http_code_t::HTTP_CODE_METHOD_NOT_ALLOWED:                     http_name = "Method Not Allowed"; break;
        case http_code_t::HTTP_CODE_NOT_ACCEPTABLE:                         http_name = "Not Acceptable"; break;
        case http_code_t::HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED:          http_name = "Proxy Authentication Required"; break;
        case http_code_t::HTTP_CODE_REQUEST_TIMEOUT:                        http_name = "Request Timeout"; break;
        case http_code_t::HTTP_CODE_CONFLICT:                               http_name = "Conflict"; break;
        case http_code_t::HTTP_CODE_GONE:                                   http_name = "Gone"; break;
        case http_code_t::HTTP_CODE_LENGTH_REQUIRED:                        http_name = "Length Required"; break;
        case http_code_t::HTTP_CODE_PRECONDITION_FAILED:                    http_name = "Precondition Failed"; break;
        case http_code_t::HTTP_CODE_REQUEST_ENTITY_TOO_LARGE:               http_name = "Request Entity Too Large"; break;
        case http_code_t::HTTP_CODE_REQUEST_URI_TOO_LONG:                   http_name = "Request-URI Too Long"; break;
        case http_code_t::HTTP_CODE_UNSUPPORTED_MEDIA_TYPE:                 http_name = "Unsupported Media Type"; break;
        case http_code_t::HTTP_CODE_REQUESTED_RANGE_NOT_SATISFIABLE:        http_name = "Requested Range Not Satisfiable"; break;
        case http_code_t::HTTP_CODE_EXPECTATION_FAILED:                     http_name = "Expectation Failed"; break;
        case http_code_t::HTTP_CODE_I_AM_A_TEAPOT:                          http_name = "I'm a teapot"; break;
        case http_code_t::HTTP_CODE_ENHANCE_YOUR_CALM:                      http_name = "Enhance Your Calm"; break;
        //case http_code_t::HTTP_CODE_METHOD_FAILURE:                         http_name = "Method Failure"; break;
        case http_code_t::HTTP_CODE_UNPROCESSABLE_ENTITY:                   http_name = "Unprocessable Entity"; break;
        case http_code_t::HTTP_CODE_LOCKED:                                 http_name = "Locked"; break;
        case http_code_t::HTTP_CODE_FAILED_DEPENDENCY:                      http_name = "Failed Dependency"; break;
        case http_code_t::HTTP_CODE_UNORDERED_COLLECTION:                   http_name = "Unordered Collection"; break;
        case http_code_t::HTTP_CODE_UPGRADE_REQUIRED:                       http_name = "Upgrade Required"; break;
        case http_code_t::HTTP_CODE_PRECONDITION_REQUIRED:                  http_name = "Precondition Required"; break;
        case http_code_t::HTTP_CODE_TOO_MANY_REQUESTS:                      http_name = "Too Many Requests"; break;
        case http_code_t::HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE:        http_name = "Request Header Fields Too Large"; break;
        case http_code_t::HTTP_CODE_NO_RESPONSE:                            http_name = "No Response"; break;
        case http_code_t::HTTP_CODE_RETRY_WITH:                             http_name = "Retry With"; break;
        case http_code_t::HTTP_CODE_BLOCKED_BY_WINDOWS_PARENTAL_CONTROLS:   http_name = "Blocked by Windows Parental Controls"; break;
        case http_code_t::HTTP_CODE_UNAVAILABLE_FOR_LEGAL_REASONS:          http_name = "Unavailable For Legal Reasons"; break;
        //case http_code_t::HTTP_CODE_REDIRECT:                               http_name = "Redirect"; break;
        case http_code_t::HTTP_CODE_REQUEST_HEADER_TOO_LARGE:               http_name = "Request Header Too Large"; break;
        case http_code_t::HTTP_CODE_CERT_ERROR:                             http_name = "Cert Error"; break;
        case http_code_t::HTTP_CODE_NO_CERT:                                http_name = "No Cert"; break;
        case http_code_t::HTTP_CODE_HTTP_TO_HTTPS:                          http_name = "HTTP to HTTPS"; break;
        case http_code_t::HTTP_CODE_TOKEN_EXPIRED:                          http_name = "Token Expired"; break;
        case http_code_t::HTTP_CODE_CLIENT_CLOSED_REQUEST:                  http_name = "Client Closed Request"; break;
        //case http_code_t::HTTP_CODE_TOKEN_REQUIRED:                         http_name = "Token Required"; break;

        // 5xx
        case http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR:                  http_name = "Internal Server Error"; break;
        case http_code_t::HTTP_CODE_NOT_IMPLEMENTED:                        http_name = "Not Implemented"; break;
        case http_code_t::HTTP_CODE_BAD_GATEWAY:                            http_name = "Bad Gateway"; break;
        case http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE:                    http_name = "Service Unavailable"; break;
        case http_code_t::HTTP_CODE_GATEWAY_TIMEOUT:                        http_name = "Gateway Timeout"; break;
        case http_code_t::HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED:             http_name = "HTTP Version Not Supported"; break;
        case http_code_t::HTTP_CODE_VARIANTS_ALSO_NEGOTIATES:               http_name = "Variants Also Negotiates"; break;
        case http_code_t::HTTP_CODE_INSUFFICIANT_STORAGE:                   http_name = "Insufficiant Storage"; break;
        case http_code_t::HTTP_CODE_LOOP_DETECTED:                          http_name = "Loop Detected"; break;
        case http_code_t::HTTP_CODE_BANDWIDTH_LIMIT_EXCEEDED:               http_name = "Bandwidth Limit Exceeded"; break;
        case http_code_t::HTTP_CODE_NOT_EXTENDED:                           http_name = "Not Extended"; break;
        case http_code_t::HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED:        http_name = "Network Authentication Required"; break;
        case http_code_t::HTTP_CODE_ACCESS_DENIED:                          http_name = "Access Denied"; break;
        case http_code_t::HTTP_CODE_NETWORK_READ_TIMEOUT_ERROR:             http_name = "Network read timeout error"; break;
        case http_code_t::HTTP_CODE_NETWORK_CONNECT_TIMEOUT_ERROR:          http_name = "Network connect timeout error"; break;

        default: // plugins can always use other codes
            http_name = "Unknown HTTP Code";
            break;

        }
    }
}

/** \brief Set an HTTP header.
 *
 * This function sets the specified HTTP header to the specified value.
 * This function overwrites the existing value if any. To append to the
 * existing value, use the append_header() function instead. Note that
 * append only works with fields that supports lists (comma separated
 * values, etc.)
 *
 * The value is trimmed of LWS (SP, HT, CR, LF) characters on both ends.
 * Also, if the value includes CR or LF characters, it must be followed
 * by at least one SP or HT. Note that all CR are transformed to LF and
 * double LFs are replaced by one LF.
 *
 * The definition of an HTTP header is message-header as found
 * in the snippet below:
 *
 * \code
 *     OCTET          = <any 8-bit sequence of data>
 *     CHAR           = <any US-ASCII character (octets 0 - 127)>
 *     CTL            = <any US-ASCII control character
 *                      (octets 0 - 31) and DEL (127)>
 *     CR             = <US-ASCII CR, carriage return (13)>
 *     LF             = <US-ASCII LF, linefeed (10)>
 *     SP             = <US-ASCII SP, space (32)>
 *     HT             = <US-ASCII HT, horizontal-tab (9)>
 *     CRLF           = CR LF
 *     LWS            = [CRLF] 1*( SP | HT )
 *     TEXT           = <any OCTET except CTLs,
 *                      but including LWS>
 *     token          = 1*<any CHAR except CTLs or separators>
 *     separators     = "(" | ")" | "<" | ">" | "@"
 *                    | "," | ";" | ":" | "\" | <">
 *                    | "/" | "[" | "]" | "?" | "="
 *                    | "{" | "}" | SP | HT
 *     message-header = field-name ":" [ field-value ]
 *     field-name     = token
 *     field-value    = *( field-content | LWS )
 *     field-content  = <the OCTETs making up the field-value
 *                      and consisting of either *TEXT or combinations
 *                      of token, separators, and quoted-string>
 * \endcode
 *
 * To remove a header, set the value to the empty string.
 *
 * References: http://www.w3.org/Protocols/rfc2616/rfc2616-sec2.html
 * and http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
 *
 * When adding a header, it is expected to only be used when no error
 * occurs (HEADER_MODE_NO_ERROR). However, in some circumstances it
 * is useful to send additional headers with errors or redirects.
 * These headers can use a different mode so they appear in those other
 * locations.
 *
 * \note
 * The key of the f_header map is the name in lowercase. For this
 * reason we save the field name as defined by the user in the
 * value as expected in the final result (i.e. "Blah: " + value.)
 *
 * \todo
 * Added a separate function so we can add multiple HTTP Link entries.
 *
 * \param[in] name  The name of the header.
 * \param[in] value  The value to assign to that header. Set to an empty
 *                   string to remove the header.
 * \param[in] modes  Where the header will be used.
 */
void snap_child::set_header(QString const & name, QString const & value, header_mode_t modes)
{
    {
        // name cannot include controls or separators and only CHARs
        std::vector<wchar_t> ws;
        ws.resize(name.length());
        name.toWCharArray(&ws[0]);
        int p(name.length());
        while(p > 0)
        {
            --p;
            wchar_t wc(ws[p]);
            bool valid(true);
            if(wc < 0x21 || wc > 0x7E)
            {
                valid = false;
            }
            else switch(wc)
            {
            case L'(': case L')': case L'<': case L'>': case L'@':
            case L',': case L';': case L':': case L'\\': case L'"':
            case L'/': case L'[': case L']': case L'?': case L'=':
            case L'{': case L'}': // SP & HT are checked in previous if()
                valid = false;
                break;

            default:
                //valid = true; -- default to true
                break;

            }
            if(!valid)
            {
                // more or less ASCII except well defined separators
                throw snap_child_exception_invalid_header_field_name(QString("header field name \"%1\" is not valid, found unwanted character: '%2'").arg(name).arg(QChar(wc)));
            }
        }
    }

    QString v;
    {
        // value cannot include controls except LWS (\r, \n and \t)
        std::vector<wchar_t> ws;
        ws.resize(value.length());
        value.toWCharArray(&ws[0]);
        int const max_length(value.length());
        wchar_t lc(L'\0');
        for(int p(0); p < max_length; ++p)
        {
            wchar_t wc(ws[p]);
            if((wc < 0x20 || wc == 127) && wc != L'\r' && wc != L'\n' && wc != L'\t')
            {
                // refuse controls except \r, \n, \t
                throw snap_child_exception_invalid_header_value("header field value \"" + value + "\" is not valid, found unwanted character: '" + QChar(wc) + "'");
            }
            // we MUST have a space or tab after a newline
            if(wc == L'\r' || wc == L'\n')
            {
                // if p + 1 == max_length then the user supplied the ending "\r\n"
                if(p + 1 < max_length)
                {
                    if(ws[p] != L' ' && ws[p] != L'\t' && ws[p] != L'\r' && ws[p] != L'\n')
                    {
                        // missing space or tab after a "\r\n" sequence
                        // (we also accept \r or \n although empty lines are
                        // forbidden but we'll remove them anyway)
                        throw snap_child_exception_invalid_header_value("header field value \"" + value + "\" is not valid, found a \\r pr \\n not followed by a space");
                    }
                }
            }
            if(v.isEmpty() && (wc == L' ' || wc == L'\t' || wc == L'\r' || wc == L'\n'))
            {
                // trim on the left (that's easy and fast to do here)
                continue;
            }
            if(wc == L'\r')
            {
                wc = L'\n';
            }
            if(lc == L'\n' && wc == L'\n')
            {
                // do not double '\n' (happens when user sends us "\r\n")
                continue;
            }
            v += QChar(wc);
            lc = wc;
        }
        // remove ending spaces which would otherwise cause problems in
        // the HTTP header
        while(!v.isEmpty())
        {
            QChar c(v.right(1)[0]);
            // we skip the '\r' because those were removed anyway
            if(c != ' ' && c != '\t' /*&& c != '\r'*/ && c != '\n')
            {
                break;
            }
            v.remove(v.length() - 1, 1);
        }
    }

    if(v.isEmpty())
    {
        f_header.remove(name.toLower());
    }
    else
    {
        // Note that even the Status needs to be a field
        // because we are using Apache and they expect such
        http_header_t header;
        header.f_header = name + ": " + v;
        header.f_modes = modes;
        f_header[name.toLower()] = header;
    }
}


/** \brief Check whether a header is defined.
 *
 * This function searches for the specified name in the list of
 * headers and returns true if it finds it.
 *
 * \warning
 * Cookies are headers, but these are managed using the cookie manager
 * which offers functions such as set_cookie(), cookie_is_defined(),
 * and cookie().
 *
 * \warning
 * Links are headers, but these are managed using the link manager
 * which offers functions such as set_link(), link_is_defined(),
 * and link().
 *
 * \param[in] name  Name of the header to check for.
 *
 * \return false if the header was not defined yet, true otherwise.
 */
bool snap_child::has_header(QString const & name) const
{
    return f_header.find(name.toLower()) != f_header.end();
}


/** \brief Retrieve the current value of the given header.
 *
 * This function returns the value of the specified header, if
 * it exists. You may want to first call has_header() to know
 * whether the header exists. It is not an error to get a header
 * that was not yet defined, you get an empty string as a result.
 *
 * \note
 * We only return the value of the header even though the
 * header field name is included in the f_header value,
 * we simply skip that information.
 *
 * \param[in] name  The name of the header to query.
 *
 * \return The value of this header, "" if undefined.
 */
QString snap_child::get_header(QString const & name) const
{
    header_map_t::const_iterator it(f_header.find(name.toLower()));
    if(it == f_header.end())
    {
        // it is not defined
        return "";
    }

    // return the value without the field
    return it->f_header.mid(name.length() + 2);
}


/** \brief Output the HTTP headers.
 *
 * This function prints the HTTP headers to the output.
 *
 * The headers are defined with a mode (a set of flags really) which
 * can be used to tell the server when such and such header is to
 * be output.
 *
 * Note that the Cookies headers are never printed by this function.
 *
 * \note
 * Headers are NOT encoded in UTF-8, we output them as Latin1, this is
 * VERY important; headers are checked at the time you do the set_header
 * to ensure that only Latin1 characters are used.
 *
 * \todo
 * Any header that a path other than the default (see the die() and
 * page_redirect() functions) uses should not be printed by this
 * function. At this point there is no real protection against that
 * yet it should be protected. An idea is for us to change all those
 * functions to use the set_header() first, then call this function
 * because that way the set_header() will have overwritten whatever
 * other plugins would have defined there.
 *
 * \param[in] modes  Print the headers for these modes.
 */
void snap_child::output_headers(header_mode_t modes)
{
    // The Cache-Control information are not output until here because
    // we have a couple of settings (page and server) and it is just
    // way to complicated to recompute the correct caches each time
    set_cache_control();

    // Output the status first (we may want to order the HTTP header
    // fields by type and output them ordered by type as defined in
    // the HTTP reference chapter 4.2)
    if(has_header(get_name(name_t::SNAP_NAME_CORE_STATUS_HEADER)) && (f_header["status"].f_modes & modes) != 0)
    {
        // If status is defined, it should not be 200
        write((f_header["status"].f_header + "\n").toLatin1().data());
    }

    // Now output all the other headers except the cookies
    for(header_map_t::const_iterator it(f_header.begin());
                                     it != f_header.end();
                                     ++it)
    {
        if((it.value().f_modes & modes) != 0 && it.key() != "status")
        {
            write((it.value().f_header + "\n").toLatin1().data());
        }
    }

    // Output the links
    //
    output_http_links(modes);

    // Finally output the cookies
    //
    output_cookies();

    // Done with the headers
    write("\n");
}


/** \brief Add the HTTP link to this snap_child.
 *
 * This function saves the specified link to the snap_child object.
 *
 * If the link was already defined, then it gets overwritten by the
 * new link. (i.e. the "rel" parameter is used to distinguish between
 * each link and only one can exist with a given name.)
 *
 * \param[in] link  The link to be added to this snap_child.
 */
void snap_child::add_http_link(http_link const & link)
{
    f_http_links[link.get_name()] = link;
}


/** \brief Check whether a link is defined.
 *
 * This function searches the list of links defined in snap_child, if
 * defined, the function returns true.
 *
 * \param[in] name  The name of the link (i.e. the "rel" parameter.)
 *
 * \return true if the snap_child is defined.
 */
bool snap_child::http_link_is_defined(std::string const & name)
{
    return f_http_links.find(name) != f_http_links.end();
}


/** \brief Get an HTTP link.
 *
 * This function returns a constant reference to the named HTTP link.
 *
 * If the link does not exist, then the function throws. To avoid the
 * throw, use the http_link_is_defined() first and if false avoid
 * calling this function.
 *
 * \param[in] name  The name of the link to retrieve a reference to.
 *
 * \return A reference to the named link.
 */
http_link const & snap_child::get_http_link(std::string const & name)
{
    auto const & l(f_http_links.find(name));
    if(l == f_http_links.end())
    {
        throw snap_child_exception_invalid_header_field_name(QString("link \"%1\" is not defined, you could check first using http_link_is_defined()").arg(QString::fromUtf8(name.c_str())));
    }
    return l->second;
}


/** \brief Transform the HTTP links to a header.
 *
 * This function generates the "Link: ..." header from the various
 * links plugins added to snap_child.
 *
 * The data is directly send to the output with the write() function.
 *
 * \param[in] modes  Print the headers for these modes.
 */
void snap_child::output_http_links(header_mode_t modes)
{
    // completely empty? avoid generating strings
    //
    if(f_http_links.empty())
    {
        return;
    }

    // geneteate the "Link: ..." string
    //
    std::string result(get_name(name_t::SNAP_NAME_CORE_HTTP_LINK_HEADER));
    result += ": ";

    char const * sep = "";
    for(auto const & l : f_http_links)
    {
        if(modes == HEADER_MODE_EVERYWHERE          // allow anything
        || (modes & ~HEADER_MODE_NO_ERROR) == 0     // normal circumstances
        || (l.second.get_redirect() && (modes & HEADER_MODE_REDIRECT) != 0))       // allowed in redirects
        {
            result += sep;
            result += l.second.to_http_header();

            sep = ", ";
        }
    }

    // got any links?
    // if not, then don't emit the header
    //
    if(*sep != '\0')
    {
        result += "\n";
        write(result.c_str());
    }
}


/** \brief Add a cookie.
 *
 * This function adds a cookie to send to the user.
 *
 * Contrary to most other headers, there may be more than one cookie
 * in a reply and the set_header() does not support such. Plus cookies
 * have a few other parameters so this function is used to save those
 * in a separate vector of cookies.
 *
 * The input cookie information is copied in the vector of cookies so
 * you can modify it.
 *
 * The same cookie can be redefined multiple times. Calling the function
 * again overwrites a previous call with the same "name" parameter.
 *
 * \param[in] cookie_info  The cookie value, expiration, etc.
 *
 * \sa cookie()
 * \sa cookie_is_defined()
 * \sa output_cookies()
 */
void snap_child::set_cookie(http_cookie const & cookie_info)
{
    f_cookies[cookie_info.get_name()] = cookie_info;

    // TODO: the privacy-policy URI should be locked if we want this to
    //       continue to work forever (or have a way to retrieve and
    //       cache that value; the snapserver.conf is probably not a
    //       good idea because you could have thousands of websites
    //       all with a different path)
    f_cookies[cookie_info.get_name()].set_comment_url(f_site_key_with_slash + "terms-and-conditions/privacy-policy");
}


/** \brief Ask for the cookies to not be output.
 *
 * When implementing an interface such as REST or SOAP you do not want
 * to output the cookies. It would not be safe to send cookies to the
 * browser. This function should be called by any such implementation
 * to make sure that the cookies do not go back out.
 */
void snap_child::set_ignore_cookies()
{
    f_ignore_cookies = true;
}


/** \brief Output the cookies in your header.
 *
 * Since we generate HTTP headers in different places but still want to
 * always generate the cookies if possible (if they are available) we
 * have this function to add the cookies.
 *
 * This function directly outputs the cookies to the socket of the snap.cgi
 * tool.
 *
 * \sa cookie()
 * \sa set_cookie()
 * \sa cookie_is_defined()
 */
void snap_child::output_cookies()
{
    if(f_ignore_cookies)
    {
        return;
    }

    if(!f_cookies.isEmpty())
    {
        for(cookie_map_t::const_iterator it(f_cookies.begin());
                                         it != f_cookies.end();
                                         ++it)
        {
            // the to_http_header() ensures only ASCII characters
            // are used so we can use toLatin1() below
            QString cookie_header(it.value().to_http_header() + "\n");
//printf("snap session = [%s]?\n", cookie_header.toLatin1().data());
            write(cookie_header.toLatin1().data());
        }
    }
}


/** \brief Generate a unique number.
 *
 * This function uses a counter in a text file to generate a unique number.
 * The file is a 64 bit long number (binary) which gets locked to ensure
 * that the number coming out is unique.
 *
 * The resulting number is composed of the server name a dash and the
 * unique number generated from the unique number file.
 *
 * At this point it is not expected that we'd ever run out of unique
 * numbers. 2^64 per server is a really large number. However, you do
 * want to limit calls as much as possible (if you can reuse the same
 * number or check all possibilities that could cause an error before
 * getting the unique numbers so as to avoid wasting too many of them.)
 *
 * The server name is expected to be a unique name defined in the settings.
 * (the .conf file for the server.)
 *
 * \todo
 * All the servers in a given realm should all be given a unique name and
 * information about the other servers (i.e. at least the address of one
 * other server) so that way all the servers can communicate and make sure
 * that their name is indeed unique.
 *
 * \return A string with \<server name\>-\<unique number\>
 */
QString snap_child::get_unique_number()
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }

    QString const data_path(server->get_parameter("data_path"));

    quint64 c(0);
    {
        const QString name(data_path + "/counter.u64");
        QLockFile counter(name);
        if(!counter.open(QIODevice::ReadWrite))
        {
            throw snap_child_exception_unique_number_error("count not open counter file \"" + name + "\"");
        }
        // the very first time the size is zero (empty)
        if(counter.size() != 0)
        {
            if(counter.read(reinterpret_cast<char *>(&c), sizeof(c)) != sizeof(c))
            {
                throw snap_child_exception_unique_number_error("count not read the counter file \"" + name + "\"");
            }
        }
        ++c;
        counter.reset();
        if(counter.write(reinterpret_cast<char *>(&c), sizeof(c)) != sizeof(c))
        {
            throw snap_child_exception_unique_number_error("count not write to the counter file \"" + name + "\"");
        }
        // close the file now; we do not want to hold the file for too long
    }
    return QString("%1-%2").arg(server->get_server_name().c_str()).arg(c);
}

/** \brief Initialize the plugins.
 *
 * Each site may make use of a different set of plugins. This function
 * gathers the list of available plugins and loads them as expected.
 *
 * The bare minimum is hard coded here in order to ensure that minimum
 * functionality of a website. At this time, this list is:
 *
 * \li path
 * \li filter
 * \li robotstxt
 *
 * \param[in] add_defaults  Whether the default Snap! plugins are to be
 *                          added to the list of plugins (important for
 *                          the Snap! server itself, not so much for other
 *                          servers using plugins.)
 */
snap_string_list snap_child::init_plugins(bool const add_defaults)
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }

    // load the plugins for this website
    bool need_cleanup(true);
    QString site_plugins(server->get_parameter(get_name(name_t::SNAP_NAME_CORE_PARAM_PLUGINS))); // forced by .conf?
    if(site_plugins.isEmpty())
    {
        // maybe user defined his list of plugins in his website
        libdbproxy::value plugins(get_site_parameter(get_name(name_t::SNAP_NAME_CORE_PLUGINS)));
        site_plugins = plugins.stringValue();
        if(site_plugins.isEmpty())
        {
            // if the list of plugins is empty in the site parameters
            // then get the default from the server configuration
            site_plugins = server->get_parameter(get_name(name_t::SNAP_NAME_CORE_PARAM_DEFAULT_PLUGINS));
        }
        else
        {
            // we assume that the list of plugins in the database is already
            // cleaned up and thus avoid an extra loop (see below)
            //
            need_cleanup = false;
        }
    }
    snap_string_list list_of_plugins(site_plugins.split(',', QString::SkipEmptyParts));

    // clean up the list
    if(need_cleanup)
    {
        for(int i(0); i < list_of_plugins.length(); ++i)
        {
            list_of_plugins[i] = list_of_plugins[i].trimmed();
            if(list_of_plugins.at(i).isEmpty())
            {
                // remove parts that the trimmed() rendered empty
                list_of_plugins.removeAt(i);
                --i;
            }
        }
    }

    // ensure a certain minimum number of plugins
    if(add_defaults)
    {
        for(size_t i(0); i < sizeof(g_minimum_plugins) / sizeof(g_minimum_plugins[0]); ++i)
        {
            if(!list_of_plugins.contains(g_minimum_plugins[i]))
            {
                list_of_plugins << g_minimum_plugins[i];
            }
        }
    }

    // load the plugins
    QString const plugins_path( server->get_parameter("plugins_path") );
    if( plugins_path.isEmpty() )
    {
        // Sanity check
        //
        die( http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE
           , "Plugin path not configured"
           , "Server cannot find any plugins because the path is not properly configured."
           , "An error occured loading the server plugins (plugins_path parameter in snapserver.conf is undefined)."
           );
        NOTREACHED();
    }

    if(!snap::plugins::load(plugins_path, this, std::static_pointer_cast<snap::plugins::plugin>(server), list_of_plugins))
    {
        die( http_code_t::HTTP_CODE_SERVICE_UNAVAILABLE
           , "Plugin Unavailable"
           , "Server encountered problems with its plugins."
           , "An error occurred while loading the server plugins. See other errors from the plugin implementation for details."
           );
        NOTREACHED();
    }
    // at this point each plugin was allocated through their factory
    // but they are not really usable yet because we did not initialize them

    // now boot the plugin system (send signals)
    server->init();

    return list_of_plugins;
}


/** \brief Run all the updates as required.
 *
 * This function checks when the updates were last run. If never, then it
 * runs the update immediately. Otherwise, it waits at least 10 minutes
 * between running again to avoid overloading the server. We may increase
 * (i.e. wait more than 10 minutes) that amount of time as we get a better
 * feel of the necessity.
 *
 * The update is done by going through all the modules and checking their
 * modification date and time. If newer than what was registered for
 * them so far, then we call their do_update() function. When it never
 * ran, the modification date and time is always seen as \em newer and
 * thus all updates are run.
 *
 * \todo
 * We may want to look into a way to "install" a plugin which would have
 * the side effect of setting a flag requesting an update instead of
 * relying on the plugin .so file modification date and other such
 * tricks. A clear signal sent via a command line tool or directly
 * from a website could be a lot more effective.
 *
 * \todo
 * The current implementation makes use of a "plugin threshold" which
 * is wrong because when you update plugin A with a threshold P, then
 * later update plugin B with a threshold Q and Q < P, plugin B does
 * not get updated (we'd have to test to prove this, but from what I
 * can see in the algorithm that specific case is not assume to happen
 * because we're expected to upgrade all the plugins at the same time
 * which unfortunately is never the case if you keep the server running
 * while upgrading your installation--which is not really recommended,
 * but I'm sure we'll be doing that all the time...) At this point I
 * removed the test so whether Q = P, Q < P or Q > P has no more
 * effect on the update process.
 *
 * \param[in] list_of_plugins  The list of plugin names that were loaded
 *                             for this run.
 */
void snap_child::update_plugins(snap_string_list const & list_of_plugins)
{
    SNAP_LOG_INFO("update_plugins() called with \"")(list_of_plugins.join(", "))("\"");

    // system updates run at most once every 10 minutes
    QString const core_last_updated(get_name(name_t::SNAP_NAME_CORE_LAST_UPDATED));
    QString const core_last_dynamic_update(get_name(name_t::SNAP_NAME_CORE_LAST_DYNAMIC_UPDATE));
    libdbproxy::value last_updated(get_site_parameter(core_last_updated));
    if(last_updated.nullValue())
    {
        // use an "old" date (631152000)
        last_updated.setInt64Value(SNAP_UNIX_TIMESTAMP(1990, 1, 1, 0, 0, 0) * 1000000LL);
    }
    //int64_t const last_update_timestamp(last_updated.int64Value());
    // 10 min. elapsed since last update?
    //if(is_debug() // force update in debug mode so we don't have to wait 10 min.!
    //|| f_start_date - static_cast<int64_t>(last_update_timestamp) > static_cast<int64_t>(10 * 60 * 1000000))
    {
        // this can be called more than once in debug mode whenever multiple
        // files are being loaded for a page being accessed (i.e. the main
        // page and then the .js, .css, .jpg, etc.)
        //
        // if is_debug() returns false, it should be useless unless the
        // process takes over 10 minutes
        snap_lock lock(QString("%1#snap-child-updating").arg(get_site_key_with_slash()), 60 * 60); // lock for up to 1h

        // set the state to "updating" if it currently is "ready"
        libdbproxy::value state(get_site_parameter(get_name(name_t::SNAP_NAME_CORE_SITE_STATE)));
        if(state.nullValue())
        {
            // set the state to "initializing if it is undefined
            //
            state.setStringValue("initializing");
            set_site_parameter(get_name(name_t::SNAP_NAME_CORE_SITE_STATE), state);
        }
        else if(state.stringValue() == "ready"
             || state.stringValue() == "updating"       // check this one in case a previous update failed
             || state.stringValue() == "initializing")  // check this one in case a previous initialization failed
        {
            // set the state to "initializing if it is undefined
            //
            state.setStringValue("updating");
            set_site_parameter(get_name(name_t::SNAP_NAME_CORE_SITE_STATE), state);

            // in this case we want existing user requests to have a chance
            // to end before we proceed with the update
            //
            // right now, this is a really ugly way of doing things we
            // should instead be told once the last user left
            //
            sleep(10); // yeah... that's a hack which should work 99% of the time though
        }
        else
        {
            // unknown state... what to do? what to do?
            //
            SNAP_LOG_ERROR("Updating website failed as we do not understand its current state: \"")(state.stringValue())("\".");
            return;
        }

        // we do not allow a null value in the core::site_name field,
        // so we set it on initialization, user can replace the name
        // later from the UI
        //
        libdbproxy::value site_name(get_site_parameter(get_name(name_t::SNAP_NAME_CORE_SITE_NAME)));
        if(site_name.nullValue())
        {
            site_name.setStringValue("Website Name");
            set_site_parameter(get_name(name_t::SNAP_NAME_CORE_SITE_NAME), site_name);
        }

        // save that last time we checked for an update
        //
        last_updated.setInt64Value(f_start_date);
        QString const core_plugin_threshold(get_name(name_t::SNAP_NAME_CORE_PLUGIN_THRESHOLD));
        set_site_parameter(core_last_updated, last_updated);
        libdbproxy::value threshold(get_site_parameter(core_plugin_threshold));
        if(threshold.nullValue())
        {
            // same old date...
            threshold.setInt64Value(SNAP_UNIX_TIMESTAMP(1990, 1, 1, 0, 0, 0) * 1000000LL);
        }
        int64_t const plugin_threshold(threshold.int64Value());
        int64_t new_plugin_threshold(plugin_threshold);

        // first run through the plugins to know whether one or more
        // has changed since the last website update
        for(snap_string_list::const_iterator it(list_of_plugins.begin());
                it != list_of_plugins.end();
                ++it)
        {
            QString const plugin_name(*it);
            plugins::plugin * p(plugins::get_plugin(plugin_name));
            if(p != nullptr)
            {
                SNAP_LOG_INFO("update_plugins() called with \"")(plugin_name)("\"");
                trace(QString("Updating plugin \"%1\"\n").arg(plugin_name));

                // the plugin changed, we want to call do_update() on it!
                if(p->last_modification() > new_plugin_threshold)
                {
                    new_plugin_threshold = p->last_modification();
                }
                // run the updates as required
                // we have a date/time for each plugin since each has
                // its own list of date/time checks
                QString const specific_param_name(QString("%1::%2").arg(core_last_updated).arg(plugin_name));
                libdbproxy::value specific_last_updated(get_site_parameter(specific_param_name));
                int64_t const old_last_updated(specific_last_updated.safeInt64Value());
                if(specific_last_updated.nullValue())
                {
                    // use an "old" date (631152000)
                    specific_last_updated.setInt64Value(SNAP_UNIX_TIMESTAMP(1990, 1, 1, 0, 0, 0) * 1000000LL);
                }
                try
                {
                    specific_last_updated.setInt64Value(p->do_update(old_last_updated));

                    // avoid the database access if the value did not change
                    //
                    if(specific_last_updated.int64Value() != old_last_updated)
                    {
                        set_site_parameter(specific_param_name, specific_last_updated);
                    }
                }
                catch(std::exception const & e)
                {
                    SNAP_LOG_ERROR("Updating ")(plugin_name)(" failed with an exception: ")(e.what());
                }
            }
        }

        // this finishes the content.xml updates
        SNAP_LOG_INFO("update_plugins() finalize the content.xml (a.k.a. static) update.");
        finish_update();

        // now allow plugins to have a more dynamic set of updates
        for(snap_string_list::const_iterator it(list_of_plugins.begin());
                it != list_of_plugins.end();
                ++it)
        {
            QString const plugin_name(*it);
            plugins::plugin * p(plugins::get_plugin(plugin_name));
            if(p != nullptr)
            {
                trace(QString("Dynamically update plugin \"%1\"\n").arg(plugin_name));

                // run the updates as required
                // we have a date/time for each plugin since each has
                // its own list of date/time checks
                QString const specific_param_name(QString("%1::%2").arg(core_last_dynamic_update).arg(plugin_name));
                libdbproxy::value specific_last_updated(get_site_parameter(specific_param_name));
                int64_t const old_last_updated(specific_last_updated.safeInt64Value());
                if(specific_last_updated.nullValue())
                {
                    // use an "old" date (631152000)
                    specific_last_updated.setInt64Value(SNAP_UNIX_TIMESTAMP(1990, 1, 1, 0, 0, 0) * 1000000LL);
                }
                // IMPORTANT: Note that we save the newest date found in the
                //            do_update() to make 100% sure we catch all the
                //            updates every time (using "now" would often mean
                //            missing many updates!)
                try
                {
                    specific_last_updated.setInt64Value(p->do_dynamic_update(old_last_updated));

                    // avoid the database access if the value did not change
                    //
                    if(specific_last_updated.int64Value() != old_last_updated)
                    {
                        set_site_parameter(specific_param_name, specific_last_updated);
                    }
                }
                catch(std::exception const & e)
                {
                    SNAP_LOG_ERROR("Dynamically updating ")(plugin_name)(" failed with an exception: ")(e.what());
                }
            }
        }

        // save the new threshold if larger
        //
        if(new_plugin_threshold > plugin_threshold)
        {
            set_site_parameter(core_plugin_threshold, new_plugin_threshold);
        }

        // always mark when the site was last updated
        //
        // get the date right now, DO NOT RESET START DATE (f_start_date)
        //
        set_site_parameter(get_name(name_t::SNAP_NAME_CORE_SITE_READY), get_current_date());

        // change the state while still locked
        //
        state.setStringValue("ready");
        set_site_parameter(get_name(name_t::SNAP_NAME_CORE_SITE_STATE), state);
    }
}


/** \brief After adding content, call this function to save it.
 *
 * When we add content with the add_xml() and add_xml_dom() functions,
 * the snap_child object records the fact and is then capable of knowing
 * that it needs to be saved.
 *
 * All the adds are expected to be called before the data gets saved
 * which allows us to not have to know in which order the XML files need
 * to be to add all of their contents at once. In other words, the save
 * function will automatically be capable of figuring out the right order
 * if it has all the data, hence this function getting called last.
 *
 * \note
 * This was separated so the layouts can be added at any time since we
 * do not activate all the layouts automatically in all of our users'
 * websites.
 */
void snap_child::finish_update()
{
    // if content was prepared for the database, save it now
    if(f_new_content)
    {
        f_new_content = false;
        server::pointer_t server( f_server.lock() );
        if(!server)
        {
            throw snap_logic_exception("server pointer is nullptr");
        }
        trace("Save content in database.\n");
        server->save_content();
        trace("Initialization content now saved in database.\n");
    }
}


/** \brief Called whenever a plugin prepares some content to the database.
 *
 * This function is called by the content plugin whenever one of its
 * add_...() function is called. This way the child knows that it has
 * to request the content to save the resulting content.
 *
 * The flag is first checked after the updates are run and the save is
 * called then. The check is done again at the end of the execute function
 * just in case some dynamic data was added while we were running.
 */
void snap_child::new_content()
{
    f_new_content = true;
}


/** \brief Canonalize a path or URL for this plugin.
 *
 * This function is used to canonicalize the paths used to check
 * URLs. This is used against the paths offered by other plugins
 * and the paths arriving from the HTTP server. This way, we know
 * that two paths will match 1 to 1.
 *
 * The canonicalization is done in place.
 *
 * Note that the canonicalization needs to occur before the
 * regular expresions are checked. Also, internal paths that
 * include regular expressions are not getting canonicalized
 * since we may otherwise break the regular expression
 * (i.e. unwillingly remove periods and slashes.)
 * This can explain why one of your paths doesn't work right.
 *
 * The function is really fast if the path is already canonicalized.
 *
 * \note
 * There is one drawback with "fixing" the URL from the user.
 * Two paths that look different will return the same page.
 * Instead we probably want to return an error (505 or 404
 * or 302.) This may be an dynamic setting too.
 *
 * \note
 * The remove() function on a QString is faster than most other
 * options because it is directly applied to the string data.
 *
 * \param[in,out] path  The path to canonicalize.
 */
void snap_child::canonicalize_path(QString & path)
{
    // we get the length on every loop because it could be reduced!
    int i(0);
    while(i < path.length())
    {
        switch(path[i].cell())
        {
        case '\\':
            path[i] = '/';
            break;

        case ' ':
        case '+':
        //case '_': -- this should probably be a flag?
            path[i] = '-';
            break;

        default:
            // other characters are kept as is
            break;

        }
        // here we do not have to check for \ since we just replaced it with /
        if(i == 0 && (path[0].cell() == '.' || path[0].cell() == '/' /*|| path[0].cell() == '\\'*/))
        {
            do
            {
                path.remove(0, 1);
            }
            while(!path.isEmpty() && (path[0].cell() == '.' || path[0].cell() == '/' || path[0].cell() == '\\'));
            // however, in the while we do since later characters were not yet
            // modified to just '/'
        }
        else if(path[i].cell()  == '/' && i + 1 < path.length())
        {
            if(path[i + 1].cell() == '/')
            {
                // remove double '/' in filename
                path.remove(i + 1, 1);
            }
            else if(path[i + 1].cell() == '.')
            {
                // Unix hidden files are forbidden (., .. and .<name>)
                // (here we remove 1 period, on next loop we may remove others)
                path.remove(i + 1, 1);
            }
            else
            {
                ++i;
            }
        }
        else if((path[i].cell() == '.' || path[i].cell() == '-' || path[i].cell() == '/') && i + 1 >= path.length())
        {
            // Filename cannot end with a period, dash (space,) or slash
            path.remove(i, 1);
        }
        else
        {
            ++i;
        }
    }
}


/** \brief We're ready to execute the page, do so.
 *
 * This time we're ready to execute the page the user is trying
 * to access.
 *
 * The function first prepares the HTTP request which includes
 * setting up default headers and the output buffer.
 *
 * Note that by default we expect text/html in the output. If a
 * different type of data is being processed, you are responsible
 * for changing the Content-type field.
 *
 * \todo
 * Take the Origin header in account. If it is not the right
 * origin, especially for log in, registration, and related
 * parges, then we may want to generate an error.
 */
void snap_child::execute()
{
    // prepare the output buffer
    // reserver 64Kb at once to avoid many tiny realloc()
    f_output.buffer().reserve(64 * 1024);
    f_output.open(QIODevice::ReadWrite);

    // TODO if the client says HTTP/1.0 and offers an Upgrade of 1.1, then
    //      we should force switch to 1.1 with a 101 response about here
    // TBD Apache may already take care of such things
    // TBD It may also be used to switch between HTTP and SHTTP
    //if(snapenv("SERVER_PROTOCOL").toUpper() == "HTTP/1.0")
    //{
    //    // if the Upgrade header was specified check whether it includes
    //    // HTTP/1.1 as one of the supported protocols
    //    // Note: we may want to make use of the http_string::WeightedHttpString
    //    //       object to split that client parameter
    //    if(snapenv("?REQUEST_UPGRADE?").simplified().replace(QRegExp("[ \t]?,[ \t]?"), ",").toLower().split(',', QString::SkipEmptyParts).contains("HTTP/1.1"))
    //    {
    //        set_header("Status", "HTTP/1.1 101 Switching Protocols");
    //    }
    //}

    // TODO: Check the cache request status from the client, if not defined
    //       or set to "max-age=0" or some other such value, then check whether
    //       the current page is cached and can safely be resent to the client
    //       (i.e. a public page without form...) if so send the cached version
    //       which will allow us to avoid all the processing.
    //
    // Note: the cached versions are saved really only if the page is
    //       public, mostly non-dynamic, and has no forms other than Search
    //       and similar...

    // prepare the default headers
    // Status is set to HTTP/1.1 or 1.0 depending on the incoming protocol
    // DO NOT PUT A STATUS OF 200 FOR FastCGI TAKES CARE OF IT
    // Sending a status of 200 to Apache results in a status of 500 Internal Server Error
    //set_header("Status", QString("%1 200 OK").arg(snapenv("SERVER_PROTOCOL")));

    // Normally Apache overwrites this information
    set_header("Server", "Snap! C++", HEADER_MODE_EVERYWHERE);

    // The Date field is added by Apache automatically
    // adding it here generates a 500 Internal Server Error
    //QDateTime date(QDateTime().toUTC());
    //date.setTime_t(f_start_date / 1000000); // micro-seconds
    //set_header("Date", date.toString("ddd, dd MMM yyyy hh:mm:ss' GMT'"));

    // cache controls are now defined in f_server_cache_control
    // and f_page_cache_control; the defaults are not exactly what
    // we want, so we change them in the f_page_cache_control
    //
    //set_cache_control(0, false, true);
    f_page_cache_control.set_no_store(true);
    f_page_cache_control.set_must_revalidate(true);

    // We are snapwebsites
    set_header(get_name(name_t::SNAP_NAME_CORE_X_POWERED_BY_HEADER), "snapwebsites", HEADER_MODE_EVERYWHERE);

    // By default we expect [X]HTML in the output
    set_header(get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER), "text/html; charset=utf-8", HEADER_MODE_EVERYWHERE);

    // XXX I moved that up here from just before sending the output because
    //     it seems that all answers should use this flag (because even pages
    //     that represent errors may end up reusing the same connection.)
    QString const connection(snapenv("HTTP_CONNECTION"));
//std::cout << "HTTP_CONNECTION=[" << connection << "]\n";
    if(connection.toLower() == "keep-alive")
    {
        set_header("Connection", "keep-alive");
    }
    else
    {
        set_header("Connection", "close");
    }

    // Let the caches know that the cookie changes all the time
    // (the content is likely to change too, but it could still be cached)
    // TBD -- I'm not entirely sure that this is smart; another default is
    //        to use "Vary: *" so all fields are considered as varying.
    //set_header("Vary", "Cookie");

    if(f_uri.protocol() == "https")
    {
        // this is used by different load balancers as an indication that
        // the request is secure
        //
        set_header("Front-End-Https", "on", HEADER_MODE_EVERYWHERE);
    }

    // give a chance to the system to use cookies such as the
    // cookie used to mark a user as logged in to kick in early
    //
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    server->process_cookies();

    // the cookie handling may generate an immediate response in which case
    // we want to exit ASAP and not execute anything
    //
    if(f_output.buffer().size() == 0)
    {
        // let plugins detach whatever data they attached to the user session
        // (note: nothing to do with the fork() which was called a while back)
        //
        server->detach_from_session();

        f_ready = true;

        // generate the output
        //
        // on_execute() is defined in the path plugin which retrieves the
        // path::primary_owner of the content that match f_uri.path() and
        // then calls the corresponding on_path_execute() function of that
        // primary owner
        server->execute(f_uri.path());

        // TODO: look into moving this call to the exit() function since
        //       it should be called no matter what (or maybe have some
        //       form of RAII, but if exit() gets called, RAII will not
        //       do us any good...)
        //
        // now that execution is over, we want to re-attach whatever did
        // not make it in this session (i.e. a message that was posted
        // after messages were added to the current page, or this page
        // did not make use of the messages that were detached earlier)
        server->attach_to_session();

        if(f_output.buffer().size() == 0)
        {
            // somehow nothing was output...
            // (that should not happen because the path::on_execute() function
            // checks and generates a Page Not Found on empty content)
            die(http_code_t::HTTP_CODE_NOT_FOUND, "Page Empty",
                "Somehow this page could not be generated.",
                "the execute() command ran but the output is empty (this is never correct with HTML data, it could be with text/plain responses though)");
            NOTREACHED();
        }
    }

    if(f_is_being_initialized)
    {
        trace("Initialization succeeded.\n");
        trace("#END\n");

        // TODO: should we also sent the headers and output buffer?
        //       we could send that righ after the #END mark
        //       (this could be done by removing the if/else so the
        //       output_result() function gets called either way!)
    }
    else
    {
        // created a page, output it now
        //
        output_result(HEADER_MODE_NO_ERROR, f_output.buffer());
    }
}


/** \brief Output the resulting buffer.
 *
 * This function takes all the data generated by the different processes
 * run and generates the final output.
 *
 * It must be used by all the functions that are ready to send their
 * result to the client (assuming the default scheme cannot be used,
 * somehow.) This is important because this function makes sure to:
 *
 * \li Signal all plugins of the event,
 * \li Only send the headers if the method was HEAD,
 * \li Answer AJAX requests as expected.
 *
 * \param[in] modes  Print the headers for these modes.
 * \param[in] output_data  The output being process.
 */
void snap_child::output_result(header_mode_t modes, QByteArray output_data)
{
    // give plugins a chance to tweak the output one more time
    server::pointer_t server(f_server.lock());
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    server->output_result(f_uri.path(), output_data);

    // Handling the compression has to be done before defining the
    // Content-Length header since that represents the compressed
    // data and not the full length

    // TODO make use of the pre-canonicalized compression information
    //      (but also look into the identity handling... at this point
    //      we do not generate a 406 if identity is defined and set to
    //      zero if another compression is available...)

    // TODO when downloading a file (an attachment) that's already
    //      compressed we need to specify the compression of the output
    //      buffer so that way here we can "adjust" the compression as
    //      required -- some of that is already done/supported TBD

    // was the output buffer generated from an already compressed file?
    // if so, then skip the encoding handling
    QString const current_content_encoding(get_header("Content-Encoding"));
    if(current_content_encoding.isEmpty())
    {
        // TODO image file formats that are already compressed should not be
        //      recompressed (i.e. JPEG, GIF, PNG...)

        // at this point we only attempt to compress known text formats
        // (should we instead have a list of mime types that we do not want to
        // compress? before, attempting to compress the "wrong" files would
        // make the browsers fail badly... but today not so much.)
        //
        QString const content_type(get_header(get_name(name_t::SNAP_NAME_CORE_CONTENT_TYPE_HEADER)));
        if(content_type.startsWith("text/plain;")
        || content_type.startsWith("text/html;")
        || content_type.startsWith("text/xml;")
        || content_type.startsWith("text/css;")
        || content_type.startsWith("text/javascript;"))
        {
            // TODO add compression capabilities with bz2, lzma and sdch as
            //      may be supported by the browser (although sdch is not
            //      possible here as long as we require/use Apache2)
            http_strings::WeightedHttpString encodings(snapenv("HTTP_ACCEPT_ENCODING"));

            // it looks like some browsers use "x-gzip" instead of
            // plain "gzip"; also our default (i.e. "*") is gzip so
            // check that value here too
            //
            float const gzip_level(std::max(std::max(encodings.get_level("gzip"), encodings.get_level("x-gzip")), encodings.get_level("*")));

            // plain zlib data is named "deflate"
            float const deflate_level(encodings.get_level("deflate"));

            if(gzip_level > 0.0f && gzip_level >= deflate_level)
            {
                // browser asked for gzip with higher preference
                QString compressor("gzip");
                output_data = compression::compress(compressor, output_data, 100, true);
                if(compressor == "gzip")
                {
                    // compression succeeded
                    set_header("Content-Encoding", "gzip", HEADER_MODE_EVERYWHERE);
                }
            }
            else if(deflate_level > 0.0f)
            {
                QString compressor("deflate");
                output_data = compression::compress(compressor, output_data, 100, true);
                if(compressor == "deflate")
                {
                    // compression succeeded
                    set_header("Content-Encoding", "deflate", HEADER_MODE_EVERYWHERE);
                }
            }
            else
            {
                // Code 406 is in the spec. (RFC2616) but frankly?!
                float const identity_level(encodings.get_level("identity"));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                if(!f_died && identity_level == 0.0f)
#pragma GCC diagnostic pop
                {
                    die(http_code_t::HTTP_CODE_NOT_ACCEPTABLE, "No Acceptable Compression Encoding",
                        "Your client requested a compression that we do not offer and it does not accept content without compression.",
                        "a client requested content with Accept-Encoding: identity;q=0 and no other compression we understand");
                    NOTREACHED();
                }
                // The "identity" SHOULD NOT be used with the Content-Encoding
                // (RFC 2616 -- https://tools.ietf.org/html/rfc2616)
                //set_header("Content-Encoding", "identity", HEADER_MODE_EVERYWHERE);
            }
        }
        else
        {
            // note that "html"-output is NOT html in this case!
            // (most likely an image... but any document really)
        }
    }

    QString const size(QString("%1").arg(output_data.size()));
    set_header("Content-Length", size, HEADER_MODE_EVERYWHERE);

    output_headers(modes);

    // write the body in all circumstances unless
    // the method is HEAD and the request did not fail
    //
    // IMPORTANT NOTE: it looks like Apache2 removes the body no matter what
    //                 which is probably sensible... if a HEAD is used it is
    //                 not a browser anyway
    if(snapenv(get_name(name_t::SNAP_NAME_CORE_REQUEST_METHOD)) != "HEAD"
    /*|| (modes & HEADER_MODE_NO_ERROR) == 0 -- not working... */)
    {
        write(output_data, output_data.size());
// Warning: do not use this std::cout if you allow compression... 8-)
//std::cout << "output [" << output_data.data() << "] [" << output_data.size() << "]\n";
    }
}


/** \brief Process the post if there was one.
 *
 * This function processes the post, as in checks all the validity of
 * all parameters and save the data in the database, and then returns.
 *
 * \note
 * If the post was successful it is possible that the function generates
 * an redirect and then exit immediately.
 */
void snap_child::process_post()
{
    if(f_has_post)
    {
        server::pointer_t server( f_server.lock() );
        if(!server)
        {
            throw snap_logic_exception("server pointer is nullptr");
        }
        server->process_post(f_uri.path());
    }
}


/** \brief Get the language defined by the user or browser.
 *
 * This function retrieves the language to be used in this
 * request:
 *
 * \li the user defined language (sub-domain, path, or a GET variable)
 * \li the preferred language of that user (user account)
 * \li the language that the browser sent (as a last resort.)
 *
 * The user preferred language requires the user to be logged in. This
 * call only works 100% correctly only if made the server::execute()
 * call started (f_ready is true in snap_child).
 *
 * The language information is expected to be used to return the
 * proper content of a page. However, you probably want to use the
 * get_language_key() function instead as it will properly concatenate
 * the language and the country exactly as required.
 *
 * If no language were defined, this function returns the "default"
 * (as in neutral) language which is "xx". This may be returned if
 * the function is called to early and no browser defaults were
 * found in the request.
 *
 * \note
 * The test to know whether the user has a preferred language selection
 * is done in this function because the canonicalize_options() function
 * runs too soon for such that query to function then (i.e. the plugins
 * are defined, but the process_cookies() signal was not yet sent.)
 *
 * \return The language (2 letter canonicalized language name.)
 */
QString snap_child::get_language()
{
    // was the language defined as a sub-domain, a path segment, or
    // a query string parameter? if so f_language is not empty
    if(f_language.isEmpty())
    {
        get_plugins_locales();
        if(!f_plugins_locales.isEmpty())
        {
            // a plugin defined a language
            //
            f_language = f_plugins_locales[0].get_language();
            f_country = f_plugins_locales[0].get_country();
        }
        else if(!f_browser_locales.isEmpty())
        {
            // use client locale as provided by the browser
            //
            f_language = f_browser_locales[0].get_language();
            f_country = f_browser_locales[0].get_country();
        }
        else
        {
            // no language defined, use the "no language" default in that case
            //
            f_language = "xx";
        }
    }

    return f_language;
}


/** \brief Retrieve all the languages in the order of preference.
 *
 * This function returns a vector of languages in the preference order
 * which can be used to search for a match.
 *
 * \todo
 * Look into whether we could have a vector of references instead because
 * right now we copy all the languages twice!
 *
 * \todo
 * Look into converting all the language/location to use the
 * WeightedHttpStrings so we can have additional parameters
 * attached to each language.
 *
 * \return An array of locales (language/country pairs).
 */
snap_child::locale_info_vector_t snap_child::get_all_locales()
{
    if(f_all_locales.isEmpty())
    {
        // first we want to check with the f_language and f_country
        //
        if(!f_language.isEmpty())
        {
            locale_info_t lang;
            lang.set_language(f_language);
            lang.set_country(f_country);

            f_all_locales.push_back(lang);
        }

        // next we want to add the plugin definitions because a logged
        // in user (or known, at least) who has preferences, we have to
        // respect those preferences
        //
        get_plugins_locales();
        for(auto const & l : f_plugins_locales)
        {
            if(std::find(f_all_locales.begin(), f_all_locales.end(), l) == f_all_locales.end())
            {
                // not in the list yet, add it
                //
                f_all_locales.push_back(l);
            }
        }

        // next we use the browser defined languages
        //
        for(auto const & l : f_browser_locales)
        {
            if(std::find(f_all_locales.begin(), f_all_locales.end(), l) == f_all_locales.end())
            {
                // not in the list yet, add it
                //
                f_all_locales.push_back(l);
            }
        }

        // now we want to re-add all those languages but without a country
        // it is important before we add tests for default languages
        //
        if(!f_language.isEmpty()
        && !f_country.isEmpty())
        {
            locale_info_t l;
            l.set_language(f_language);
            if(std::find(f_all_locales.begin(), f_all_locales.end(), l) == f_all_locales.end())
            {
                f_all_locales.push_back(l);
            }
        }

        for(auto p : f_plugins_locales)
        {
            if(!p.get_country().isEmpty())
            {
                locale_info_t l;
                l.set_language(p.get_language());
                f_all_locales.push_back(l);
            }
        }

        for(auto p : f_browser_locales)
        {
            if(!p.get_country().isEmpty())
            {
                locale_info_t l;
                l.set_language(p.get_language());
                if(std::find(f_all_locales.begin(), f_all_locales.end(), l) == f_all_locales.end())
                {
                    f_all_locales.push_back(l);
                }
            }
        }

        // finally, we add a few defaults in case no other languages
        // matches we want to have those fallbacks
        //
        {
            // Any English
            //
            locale_info_t l;
            l.set_language("en");
            if(std::find(f_all_locales.begin(), f_all_locales.end(), l) == f_all_locales.end())
            {
                f_all_locales.push_back(l);
            }
        }
        {
            // Try without a language ("neutral" for pages like a photo)
            //
            locale_info_t l;
            l.set_language("xx");
            if(std::find(f_all_locales.begin(), f_all_locales.end(), l) == f_all_locales.end())
            {
                f_all_locales.push_back(l);
            }
        }
        {
            // Finally we want to try with no language / no country
            // (it should never already be in the list?!)
            //
            locale_info_t l;
            if(std::find(f_all_locales.begin(), f_all_locales.end(), l) == f_all_locales.end())
            {
                f_all_locales.push_back(l);
            }
        }
    }

    return f_all_locales;
}


/** \brief Get the name of the country.
 *
 * This function returns the name of the country linked with the language
 * that the user requested. For example, in the locale "en_US" the country
 * code is US. This is what this function returns.
 *
 * In most cases this function is only used internally when getting the
 * language key. You should not make use of this information as meaning
 * that the user is from that country.
 *
 * \warning
 * You should always call get_language() FIRST because it will define
 * the country if the language was not defined as a sub-domain, a path
 * segment, or a query string parameter.
 *
 * \return The country (2 letter canonicalized country name.)
 *
 * \sa get_language()
 * \sa get_language_key()
 */
QString snap_child::get_country() const
{
    return f_country;
}


/** \brief The "language" (or locale) the user or browser defined.
 *
 * This function returns the language and country merged as a one key.
 * The name of the language and the name of the country are always
 * exactly 2 characters. The language name is in lowercase and the
 * country is in uppercase. The two names are separated by an
 * underscore (_).
 *
 * For example, English in the USA would be returned as: "en_US".
 * German in Germany is defined as "de_DE".
 *
 * \return The language key or "locale" for this request.
 */
QString snap_child::get_language_key()
{
    if(f_language_key.isEmpty())
    {
        f_language_key = get_language();
        QString country(get_country());
        if(!country.isEmpty())
        {
            f_language_key += '_';
            f_language_key += country;
        }
    }
    return f_language_key;
}


/** \brief Get the list of locales defined by plugins.
 *
 * At this point the main plugin defining locales to be used is the users
 * plugin which offers the user to edit a set his settings and as one
 * of the entries offers the user to enter one or more locales.
 *
 * The array of user locales is properly defined only if the f_ready
 * flag is true. Otherwise it won't be proper since the user won't already
 * be logged in.
 *
 * \return An array of locales.
 */
snap_child::locale_info_vector_t const& snap_child::get_plugins_locales()
{
    if(f_plugins_locales.isEmpty())
    {
        if(!f_ready)
        {
            // I do not think this happens, but just in case warn myself
            //
            SNAP_LOG_WARNING("get_plugins_locales() called before f_ready was set to true");
        }

        // retrieve the locales from all the plugins
        // we expect a string of locales defined as weighted HTTP strings
        // remember that without a proper weight the algorithm uses 1.0
        //
        server::pointer_t server( f_server.lock() );
        if(!server)
        {
            throw snap_logic_exception("server pointer is nullptr");
        }
        http_strings::WeightedHttpString locales;
        server->define_locales(locales);

        http_strings::WeightedHttpString::part_t::vector_t const & plugins_languages(locales.get_parts());
        if(!plugins_languages.isEmpty())
        {
            // not sorted by default, make sure it gets sorted
            // (if empty we can avoid this call and since the plugins_languages
            // parameter is a reference, we will still see the result)
            //
            locales.sort_by_level();

            int const max_languages(plugins_languages.size());
            for(int i(0); i < max_languages; ++i)
            {
                QString plugins_lang(plugins_languages[i].get_name());
                QString plugins_country;
                if(verify_locale(plugins_lang, plugins_country, false))
                {
                    // only keep those that are considered valid (they all should
                    // be, but in case a hacker or strange client access our
                    // systems...)
                    //
                    locale_info_t l;
                    l.set_language(plugins_lang);
                    l.set_country(plugins_country);
                    f_plugins_locales.push_back(l);
                    // added in order
                }
                else
                {
                    // plugin sent us something we do not understand
                    //
                    SNAP_LOG_WARNING(QString("plugin locale \"%1\" does not look like a legal locale, ignore")
                                                .arg(plugins_languages[i].get_name()));
                }
            }
        }
    }

    return f_plugins_locales;
}


/** \brief Retrieve the list of locales as defined in the browser.
 *
 * This function returns the array of locales as defined in the user's
 * browser. This is considered the fallback in case the user did not
 * force a language in his request (sub-domain, path segment, or query
 * string parameter.)
 *
 * \return A constant reference to the browser locales.
 */
snap_child::locale_info_vector_t const & snap_child::get_browser_locales() const
{
    return f_browser_locales;
}


/** \brief Check whether the current (false) or current working (true) branch should be used.
 *
 * This function returns true if the user requested the working branch.
 *
 * \return true if the current working branch should be used.
 */
bool snap_child::get_working_branch() const
{
    return f_working_branch;
}


/** \brief Retrieve the branch number to be accessed.
 *
 * This function returns SPECIAL_VERSION_UNDEFINED (which is -1) if the
 * branch was not defined by the user.
 */
snap_version::version_number_t snap_child::get_branch() const
{
    return f_branch;
}


/** \brief Get the revision number that the user defined.
 *
 * This function returns zero if the user has not set the revision.
 * Note that zero is also a valid revision.
 */
snap_version::version_number_t snap_child::get_revision() const
{
    return f_revision;
}


/** \brief Retrieve the full revision key.
 *
 * The branch and revision numbers combined with a period as the separator
 * represents the revision key of this request.
 *
 * Note that if the user did not specify a revision, then the string
 * returned is just the branch. If no branch and no revision were
 * specified, then the key is empty and the default for that page is to
 * be used.
 *
 * \return A string defined as "\<branch>.\<revision>".
 */
QString snap_child::get_revision_key() const
{
    return f_revision_key;
}


/** \brief Get compression information.
 *
 * This function retrieves a reference to the array of compressions accepted
 * by the client. This can be used by plugins that generate the output
 * directly and the snap_child functions won't get a chance to compress
 * the data later or for such a plugin to let the snap_child object
 * know that the data is pre-compressed (i.e. when we pre-compress data
 * with gzip we can just return that data, it means we read less from
 * the database and we may have compressed that data on a backend
 * server leaving the front end running full speed.)
 *
 * The array may end with COMPRESSION_INVALID in which case one of the
 * compressions defined before MUST have been a match. If not, then the
 * file cannot be returned to the user and a 406 error
 * (HTTP_CODE_NOT_ACCEPTABLE) should be returned to the client.
 *
 * \return The array of compressions expected by the client, preferred first.
 */
snap_child::compression_vector_t snap_child::get_compression() const
{
    return f_compressions;
}


/** \brief Convert a time/date value to a string.
 *
 * This function transform a date such as the content::modified field
 * to a format that is useful to the XSL parser. It supports various
 * formats:
 *
 * \li DATE_FORMAT_SHORT -- YYYY-MM-DD
 * \li DATE_FORMAT_LONG  -- YYYY-MM-DDTHH:MM:SSZ
 * \li DATE_FORMAT_TIME  -- HH:MM:SS
 * \li DATE_FORMAT_EMAIL -- dd MMM yyyy hh:mm:ss +0000
 * \li DATE_FORMAT_HTTP  -- ddd, dd MMM yyyy hh:mm:ss +0000
 *
 * The long format includes the time.
 *
 * The HTTP format uses the day and month names in English only since
 * the HTTP protocol only expects English.
 *
 * The date is always output as UTC (opposed to local time.)
 *
 * \note
 * In order to display a date to the end user (in the HTML for the users)
 * you may want to setup the timezone information first and then use
 * the various functions supplied by the locale plugin to generate the
 * date. The locale plugin also supports formatting numbers, time,
 * messages, etc. going both ways (from the formatted data to internal
 * data and vice versa.) There is also JavaScript support for editor
 * widgets.
 *
 * \param[in] v  A 64 bit time / date value in microseconds, although we
 *               really only use precision to the second.
 * \param[in] date_format  Which format should be used.
 *
 * \return The formatted date and time.
 */
QString snap_child::date_to_string(int64_t v, date_format_t date_format)
{
    // go to seconds
    time_t seconds(v / 1000000);

    struct tm time_info;
    gmtime_r(&seconds, &time_info);

    char buf[256];
    buf[0] = '\0';

    switch(date_format)
    {
    case date_format_t::DATE_FORMAT_SHORT:
        strftime(buf, sizeof(buf), "%Y-%m-%d", &time_info);
        break;

    case date_format_t::DATE_FORMAT_SHORT_US:
        strftime(buf, sizeof(buf), "%m-%d-%Y", &time_info);
        break;

    case date_format_t::DATE_FORMAT_LONG:
        // TBD do we want the Z when generating time for HTML headers?
        // (it is useful for the sitemap.xml at this point)
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &time_info);
        break;

    case date_format_t::DATE_FORMAT_TIME:
        strftime(buf, sizeof(buf), "%H:%M:%S", &time_info);
        break;

    case date_format_t::DATE_FORMAT_EMAIL:
        { // dd MMM yyyy hh:mm:ss +0000
            // do it manually so the date is ALWAYS in English
            return QString("%1 %2 %3 %4:%5:%6 +0000")
                .arg(time_info.tm_mday, 2, 10, QChar('0'))
                .arg(QString::fromLatin1(g_month_name[time_info.tm_mon], 3))
                .arg(time_info.tm_year + 1900, 4, 10, QChar('0'))
                .arg(time_info.tm_hour, 2, 10, QChar('0'))
                .arg(time_info.tm_min, 2, 10, QChar('0'))
                .arg(time_info.tm_sec, 2, 10, QChar('0'));
        }
        break;

    case date_format_t::DATE_FORMAT_HTTP:
        { // ddd, dd MMM yyyy hh:mm:ss GMT
            // do it manually so the date is ALWAYS in English
            return QString("%1, %2 %3 %4 %5:%6:%7 +0000")
                .arg(QString::fromLatin1(g_week_day_name[time_info.tm_wday], 3))
                .arg(time_info.tm_mday, 2, 10, QChar('0'))
                .arg(QString::fromLatin1(g_month_name[time_info.tm_mon], 3))
                .arg(time_info.tm_year + 1900, 4, 10, QChar('0'))
                .arg(time_info.tm_hour, 2, 10, QChar('0'))
                .arg(time_info.tm_min, 2, 10, QChar('0'))
                .arg(time_info.tm_sec, 2, 10, QChar('0'));
        }
        break;

    }

    return buf;
}


/** \brief Convert a date from a string to a time_t.
 *
 * This function transforms a date received by the client to a Unix
 * time_t value. We programmed our own because several fields are
 * optional and the strptime() function does not support such. Also
 * the strptime() uses the locale() for the day and month check
 * which is not expected for HTTP. The QDateTime object has similar
 * flaws.
 *
 * The function supports the RFC822, RFC850, and ANSI formats. On top
 * of these formats, the function understands the month name in full,
 * and the week day name and timezone parameters are viewed as optional.
 *
 * See the document we used to make sure we'd support pretty much all the
 * dates that a client might send to us:
 * http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html#sec3.3.1
 *
 * Formats that we support:
 *
 * \code
 *      YYYY-MM-DD
 *      DD-MMM-YYYY HH:MM:SS TZ
 *      DD-MMM-YYYY HH:MM:SS TZ
 *      WWW, DD-MMM-YYYY HH:MM:SS TZ
 *      MMM-DD-YYYY HH:MM:SS TZ
 *      WWW MMM-DD HH:MM:SS YYYY
 * \endcode
 *
 * The month and weekday may be a 3 letter abbreviation or the full English
 * name. The month must use letters. We support the ANSI format which must
 * start with the month or week name in letters. We distinguish the ANSI
 * format from the other RFC-2616 date if it starts with a week day and is
 * followed by a space, or it directly starts with the month name.
 *
 * The year may be 2 or 4 digits.
 *
 * The timezone is optional. It may use a space or a + or a - as a separator.
 * The timezone may be an abbreviation or a 4 digit number.
 *
 * \param[in] date  The date to convert to a time_t.
 *
 * \return The date and time as a Unix time_t number, -1 if the convertion fails.
 */
time_t snap_child::string_to_date(QString const & date)
{
    struct parser_t
    {
        parser_t(QString const & date)
            : f_date(date.simplified().toLower().toUtf8())
            , f_s(f_date.data())
            //, f_time_info() -- initialized below
        {
            // clear the info as default
            memset(&f_time_info, 0, sizeof(f_time_info));
        }

        void skip_spaces()
        {
            while(isspace(*f_s))
            {
                ++f_s;
            }
        }

        bool parse_week_day()
        {
            // day         =  "Mon"  / "Tue" /  "Wed"  / "Thu"
            //             /  "Fri"  / "Sat" /  "Sun"
            if(f_s[0] == 'm' && f_s[1] == 'o' && f_s[2] == 'n')
            {
                f_time_info.tm_wday = 1;
            }
            else if(f_s[0] == 't' && f_s[1] == 'u' && f_s[2] == 'e')
            {
                f_time_info.tm_wday = 2;
            }
            else if(f_s[0] == 'w' && f_s[1] == 'e' && f_s[2] == 'd')
            {
                f_time_info.tm_wday = 3;
            }
            else if(f_s[0] == 't' && f_s[1] == 'h' && f_s[2] == 'u')
            {
                f_time_info.tm_wday = 4;
            }
            else if(f_s[0] == 'f' && f_s[1] == 'r' && f_s[2] == 'i')
            {
                f_time_info.tm_wday = 5;
            }
            else if(f_s[0] == 's' && f_s[1] == 'a' && f_s[2] == 't')
            {
                f_time_info.tm_wday = 6;
            }
            else if(f_s[0] == 's' && f_s[1] == 'u' && f_s[2] == 'n')
            {
                f_time_info.tm_wday = 0;
            }
            else
            {
                return false;
            }

            // check whether the other characters follow
            if(strncmp(f_s + 3, g_week_day_name[f_time_info.tm_wday] + 3, g_week_day_length[f_time_info.tm_wday] - 3) == 0)
            {
                // full day (RFC850)
                f_s += g_week_day_length[f_time_info.tm_wday];
            }
            else
            {
                f_s += 3;
            }

            return true;
        }

        bool parse_month()
        {
            // month       =  "Jan"  /  "Feb" /  "Mar"  /  "Apr"
            //             /  "May"  /  "Jun" /  "Jul"  /  "Aug"
            //             /  "Sep"  /  "Oct" /  "Nov"  /  "Dec"
            if(f_s[0] == 'j' && f_s[1] == 'a' && f_s[2] == 'n')
            {
                f_time_info.tm_mon = 0;
            }
            else if(f_s[0] == 'f' && f_s[1] == 'e' && f_s[2] == 'b')
            {
                f_time_info.tm_mon = 1;
            }
            else if(f_s[0] == 'm' && f_s[1] == 'a' && f_s[2] == 'r')
            {
                f_time_info.tm_mon = 2;
            }
            else if(f_s[0] == 'a' && f_s[1] == 'p' && f_s[2] == 'r')
            {
                f_time_info.tm_mon = 3;
            }
            else if(f_s[0] == 'm' && f_s[1] == 'a' && f_s[2] == 'y')
            {
                f_time_info.tm_mon = 4;
            }
            else if(f_s[0] == 'j' && f_s[1] == 'u' && f_s[2] == 'n')
            {
                f_time_info.tm_mon = 5;
            }
            else if(f_s[0] == 'j' && f_s[1] == 'u' && f_s[2] == 'l')
            {
                f_time_info.tm_mon = 6;
            }
            else if(f_s[0] == 'a' && f_s[1] == 'u' && f_s[2] == 'g')
            {
                f_time_info.tm_mon = 7;
            }
            else if(f_s[0] == 's' && f_s[1] == 'e' && f_s[2] == 'p')
            {
                f_time_info.tm_mon = 8;
            }
            else if(f_s[0] == 'o' && f_s[1] == 'c' && f_s[2] == 't')
            {
                f_time_info.tm_mon = 9;
            }
            else if(f_s[0] == 'n' && f_s[1] == 'o' && f_s[2] == 'v')
            {
                f_time_info.tm_mon = 10;
            }
            else if(f_s[0] == 'd' && f_s[1] == 'e' && f_s[2] == 'c')
            {
                f_time_info.tm_mon = 11;
            }
            else
            {
                return false;
            }

            // check whether the other characters follow
            if(strncmp(f_s + 3, g_month_name[f_time_info.tm_mon] + 3, g_month_length[f_time_info.tm_mon] - 3) == 0)
            {
                // full month (not in the specs)
                f_s += g_month_length[f_time_info.tm_mon];
            }
            else
            {
                f_s += 3;
            }

            skip_spaces();
            return true;
        }

        bool integer(unsigned int min_len, unsigned int max_len, unsigned int min_value, unsigned int max_value, int & result)
        {
            unsigned int u_result = 0;
            unsigned int count(0);
            for(; *f_s >= '0' && *f_s <= '9'; ++f_s, ++count)
            {
                u_result = u_result * 10 + *f_s - '0';
            }
            if(count < min_len || count > max_len
            || u_result < min_value || u_result > max_value)
            {
                result = static_cast<int>(u_result);
                return false;
            }
            result = static_cast<int>(u_result);
            return true;
        }

        bool parse_time()
        {
            if(!integer(2, 2, 0, 23, f_time_info.tm_hour))
            {
                return false;
            }
            if(*f_s != ':')
            {
                return false;
            }
            ++f_s;
            if(!integer(2, 2, 0, 59, f_time_info.tm_min))
            {
                return false;
            }
            if(*f_s != ':')
            {
                return false;
            }
            ++f_s;
            if(!integer(2, 2, 0, 60, f_time_info.tm_sec))
            {
                return false;
            }
            skip_spaces();
            return true;
        }

        bool parse_timezone()
        {
            // any timezone?
            if(*f_s == '\0')
            {
                return true;
            }

            // XXX not too sure that the zone is properly handled at this point
            // (i.e. should I do += or -=, it may be wrong in many places...)
            //
            // The newest HTTP format is to only support "+/-####"
            //
            // zone        =  "UT"  / "GMT"
            //             /  "EST" / "EDT"
            //             /  "CST" / "CDT"
            //             /  "MST" / "MDT"
            //             /  "PST" / "PDT"
            //             /  1ALPHA
            //             / ( ("+" / "-") 4DIGIT )
            if((f_s[0] == 'u' && f_s[1] == 't' && f_s[2] == '\0')                 // UT
            || (f_s[0] == 'u' && f_s[1] == 't' && f_s[2] == 'c' && f_s[3] == '\0')  // UTC (not in the spec...)
            || (f_s[0] == 'g' && f_s[1] == 'm' && f_s[2] == 't' && f_s[3] == '\0')) // GMT
            {
                // no adjustment for UTC (GMT)
            }
            else if(f_s[0] == 'e' && f_s[1] == 's' && f_s[2] == 't' && f_s[3] == '\0') // EST
            {
                f_time_info.tm_hour -= 5;
            }
            else if(f_s[0] == 'e' && f_s[1] == 'd' && f_s[2] == 't' && f_s[3] == '\0') // EDT
            {
                f_time_info.tm_hour -= 4;
            }
            else if(f_s[0] == 'c' && f_s[1] == 's' && f_s[2] == 't' && f_s[3] == '\0') // CST
            {
                f_time_info.tm_hour -= 6;
            }
            else if(f_s[0] == 'c' && f_s[1] == 'd' && f_s[2] == 't' && f_s[3] == '\0') // CDT
            {
                f_time_info.tm_hour -= 5;
            }
            else if(f_s[0] == 'm' && f_s[1] == 's' && f_s[2] == 't' && f_s[3] == '\0') // MST
            {
                f_time_info.tm_hour -= 7;
            }
            else if(f_s[0] == 'm' && f_s[1] == 'd' && f_s[2] == 't' && f_s[3] == '\0') // MDT
            {
                f_time_info.tm_hour -= 6;
            }
            else if(f_s[0] == 'p' && f_s[1] == 's' && f_s[2] == 't' && f_s[3] == '\0') // PST
            {
                f_time_info.tm_hour -= 8;
            }
            else if(f_s[0] == 'p' && f_s[1] == 'd' && f_s[2] == 't' && f_s[3] == '\0') // PDT
            {
                f_time_info.tm_hour -= 7;
            }
            else if(f_s[0] >= 'a' && f_s[0] <= 'z' && f_s[0] != 'j' && f_s[1] == '\0')
            {
                f_time_info.tm_hour += g_timezone_adjust[f_s[0] - 'a'];
            }
            else if((f_s[0] == '+' || f_s[0] == '-')
                  && f_s[1] >= '0' && f_s[1] <= '9'
                  && f_s[2] >= '0' && f_s[2] <= '9'
                  && f_s[3] >= '0' && f_s[3] <= '9'
                  && f_s[4] >= '0' && f_s[4] <= '9'
                  && f_s[5] == '\0')
            {
                f_time_info.tm_hour += ((f_s[1] - '0') * 10 + f_s[2] - '0') * (f_s[0] == '+' ? 1 : -1);
                f_time_info.tm_min  += ((f_s[3] - '0') * 10 + f_s[4] - '0') * (f_s[0] == '+' ? 1 : -1);
            }
            else
            {
                // invalid time zone
                return false;
            }

            // WARNING: the time zone doesn't get skipped!
            return true;
        }

        bool parse_ansi()
        {
            skip_spaces();
            if(!parse_month())
            {
                return false;
            }
            if(!integer(1, 2, 1, 31, f_time_info.tm_mday))
            {
                return false;
            }
            skip_spaces();
            if(!parse_time())
            {
                return false;
            }
            if(!integer(2, 4, 0, 3000, f_time_info.tm_year))
            {
                return false;
            }
            skip_spaces();
            return parse_timezone();
        }

        bool parse_us()
        {
            skip_spaces();
            if(!parse_month())
            {
                return false;
            }
            skip_spaces();
            if(!integer(1, 2, 1, 31, f_time_info.tm_mday))
            {
                return false;
            }
            skip_spaces();
            if(!integer(2, 4, 0, 3000, f_time_info.tm_year))
            {
                return false;
            }
            skip_spaces();
            return parse_time();
        }

        bool parse()
        {
            // support for YYYY-MM-DD
            if(f_date.size() == 10
            && f_s[4] == '-'
            && f_s[7] == '-')
            {
                if(!integer(4, 4, 0, 3000, f_time_info.tm_year))
                {
                    return false;
                }
                if(*f_s != '-')
                {
                    return false;
                }
                ++f_s;
                if(!integer(2, 2, 1, 12, f_time_info.tm_mon))
                {
                    return false;
                }
                --f_time_info.tm_mon; // expect 0 to 11 in final structure
                if(*f_s != '-')
                {
                    return false;
                }
                ++f_s;
                if(!integer(2, 2, 1, 31, f_time_info.tm_mday))
                {
                    return false;
                }
                return true;
            }

            // week day (optional in RFC822)
            if(*f_s >= 'a' && *f_s <= 'z')
            {
                if(!parse_week_day())
                {
                    // maybe that was the month, not the day
                    // if the time is last, we have a preprocessor date/time
                    // the second test is needed because the string gets
                    // simplified and thus numbers 1 to 9 generate a string
                    // one shorter
                    if((strlen(f_s) == 11 + 1 + 8
                     && f_s[11 + 1 + 8 - 6] == ':'
                     && f_s[11 + 1 + 8 - 3] == ':')
                    ||
                       (strlen(f_s) == 10 + 1 + 8
                     && f_s[10 + 1 + 8 - 6] == ':'
                     && f_s[10 + 1 + 8 - 3] == ':'))
                    {
                        return parse_us();
                    }
                    return parse_ansi();
                }

                if(f_s[0] == ' ')
                {
                    // the ANSI format is completely random!
                    return parse_ansi();
                }

                if(f_s[0] != ',')
                {
                    return false;
                }
                ++f_s; // skip the comma
                skip_spaces();
            }

            if(!integer(1, 2, 1, 31, f_time_info.tm_mday))
            {
                return false;
            }

            if(*f_s == '-')
            {
                ++f_s;
            }
            skip_spaces();

            if(!parse_month())
            {
                return false;
            }
            if(*f_s == '-')
            {
                ++f_s;
                skip_spaces();
            }
            if(!integer(2, 4, 0, 3000, f_time_info.tm_year))
            {
                return false;
            }
            skip_spaces();
            if(!parse_time())
            {
                return false;
            }

            return parse_timezone();
        }

        struct tm       f_time_info;
        QByteArray      f_date;
        char const *    f_s;
    } parser(date);

    if(!parser.parse())
    {
        return -1;
    }

    // 2 digit year?
    // How to handle this one? At this time I do not expect our software
    // to work beyond 2070 which is probably short sighted (ha! ha!)
    // However, that way we avoid calling time() and transform that in
    // a tm structure and check that date
    if(parser.f_time_info.tm_year < 100)
    {
        parser.f_time_info.tm_year += 1900;
        if(parser.f_time_info.tm_year < 1970)
        {
            parser.f_time_info.tm_year += 100;
        }
    }

    // make sure the day is valid for that month/year
    if(parser.f_time_info.tm_mday > last_day_of_month(parser.f_time_info.tm_mon + 1, parser.f_time_info.tm_year))
    {
        return -1;
    }

    // now we have a time_info which is fully adjusted except for DST...
    // let's make time
    parser.f_time_info.tm_year -= 1900;
    return mkgmtime(&parser.f_time_info);
}


/** \brief From a month and year, get the last day of the month.
 *
 * This function gives you the number of the last day of the month.
 * In all cases, except February, it returns 30 or 31.
 *
 * For the month of February, we first compute the leap year flag.
 * If the year is a leap year, then it returns 29, otherwise it
 * returns 28.
 *
 * The leap year formula is:
 *
 * \code
 *      leap = !(year % 4) && (year % 100 || !(year % 400));
 * \endcode
 *
 * \warning
 * This function throws if called with September 1752 because the
 * month has missing days within the month (days 3 to 13).
 *
 * \param[in] month  A number from 1 to 12 representing a month.
 * \param[in] year  A year, including the century.
 *
 * \return Last day of month, 30, 31, or in February, 28 or 29.
 */
int snap_child::last_day_of_month(int month, int year)
{
    if(month < 1 || month > 12)
    {
        throw snap_logic_exception(QString("last_day_of_month called with %1 as the month number").arg(month));
    }

    if(month == 2)
    {
        // special case for February
        //
        // The time when people switch from Julian to Gregorian is country
        // dependent, Great Britain changed on September 2, 1752, but some
        // countries changed as late as 1952...
        //
        // For now, we use the GB date. Once we have a valid way to handle
        // this with the locale, we can look into updating the code. That
        // being said, it should not matter too much because most dates on
        // the Internet are past 2000.
        //
        if(year <= 1752)
        {
            return year % 4 == 0 ? 29 : 28;
        }
        return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0) ? 29 : 28;
    }

    if(month == 9 && year == 1752)
    {
        // we cannot handle this nice one here, days 3 to 13 are missing on
        // this month... (to adjust the calendar all at once!)
        throw snap_logic_exception(QString("last_day_of_month called with %1 as the year number").arg(year));
    }

    return g_month_days[month - 1];
}


/** \brief Get a list of all language names.
 *
 * This function returns the current list of language names as defined
 * in ISO639 and similar documents.
 *
 * The table returned ends with a nullptr entry. You may use it in this
 * way:
 *
 * \code
 * for(snap_child::language_name_t const *l(snap_child::get_languages()); l->f_name; ++l)
 * \endcode
 *
 * See also https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
 *
 * \return A pointer to the table of languages.
 */
snap_child::language_name_t const * snap_child::get_languages()
{
    return g_language_names;
}


/** \brief Get a list of all country names.
 *
 * This function returns the current list of country names as defined in
 * ISO3166 and similar documents.
 *
 * See also https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
 *
 * \return A pointer to the table of countries.
 */
snap_child::country_name_t const * snap_child::get_countries()
{
    return g_country_names;
}


/** \brief Send the backend_process() signal to all plugins.
 *
 * This function sends the server::backend_process() signal to all
 * the currently registered plugins.
 *
 * This is called by the content plugin whenever the action is set
 * to "snapbackend" which is the default when no action was specified
 * and someone started the snapbackend process:
 *
 * \code
 *      snapbackend
 *        [or]
 *      snapbackend --action snapbackend
 * \endcode
 */
void snap_child::backend_process()
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    server->backend_process();
}


/** \brief Send a PING message to the specified service.
 *
 * This function sends a PING message to the specified service.
 * This is used to wake up a backend process after you saved
 * data in the Cassandra cluster. That backend can then "slowly"
 * process the data further.
 *
 * The message is sent using a UDP packet. It is sent to the
 * Snap! Communicator running on the same server as this
 * child.
 *
 * Remember that UDP is not reliable so we do not in any way
 * guarantee that this goes anywhere. The function returns no
 * feedback at all. We do not wait for a reply since at the time
 * we send the message the listening server may be busy. The
 * idea of this ping is just to make sure that if the backend is
 * sleeping at the time, it wakes up sooner rather than later
 * so it can immediately start processing the data we just added
 * to Cassandra.
 *
 * The \p service_name is the name of the backend as it appears
 * when you run the following command:
 *
 * \code
 *      snapinit --list
 * \endcode
 *
 * At time of writing, we have the following backends: "images::images",
 * "list::pagelist", and "sendmail::sendmail".
 *
 * \param[in] service_name  The name of the backend (service) to ping.
 */
void snap_child::udp_ping(char const * service_name)
{
    server::pointer_t server( f_server.lock() );
    if(!server)
    {
        throw snap_logic_exception("server pointer is nullptr");
    }
    // the URI to be used with PING is the website URI, not the full page
    // URI (otherwise it does not work as expected)
    //
    server->udp_ping_server(service_name, f_uri.get_website_uri());
}


/** \brief Check a tag
 *
 * This function determines whether the input named tag is an inline tag.
 * Note that CSS can transform an inline tag in a block so the result of
 * this function are only relatively correct.
 *
 * The function returns true for what is considered neutral tags. For example,
 * the \<map\> and \<script\> tags are viewed as neutral. They do not affect
 * the output just by their presence in the flow. (Although a script may
 * affect the flow at run time by writing to it.)
 *
 * \param[in] tag   The name of the tag in a C-string.
 * \param[in] length  Use -1 if \p tag is null terminated, otherwise the length of the string.
 *
 * \return true if the tag is considered to be inline by default.
 */
bool snap_child::tag_is_inline(char const * tag, int length)
{
    if(tag == nullptr)
    {
        throw snap_logic_exception("tag_is_inline() cannot be called with nullptr as the tag pointer");
    }

    if(length < 0)
    {
        length = static_cast<int>(strlen(tag));
    }

    switch(tag[0])
    {
    case 'a':
        // <a>, <abbr>, <acronym>, <area>
        if(length == 1
        || strncmp(tag + 1, "bbr", length) == 0
        || strncmp(tag + 1, "cronym", length) == 0   // deprecated in HTML 5
        || strncmp(tag + 1, "rea", length) == 0)
        {
            return true;
        }
        break;

    case 'b':
        // <b>, <basefont>, <bb>, <bdi>, <bdo>, <bgsound>, <big>, <blink>, <br>, <button>
        if(length == 1
        || strncmp(tag + 1, "asefont", length) == 0      // <basefont> deprecated in HTML 4.01
        || (length == 2 && (tag[1] == 'b' || tag[1] == 'r'))
        || strncmp(tag + 1, "di", length) == 0
        || strncmp(tag + 1, "do", length) == 0
        || strncmp(tag + 1, "ig", length) == 0           // <big> deprecated in HTML 5
        || strncmp(tag + 1, "gsound", length) == 0       // <bgsound> deprecated in HTML 5
        || strncmp(tag + 1, "link", length) == 0         // <blink> deprecated in HTML 5
        || strncmp(tag + 1, "utton", length) == 0)
        {
            return true;
        }
        break;

    case 'c':
        // <cite>, <code>, <command>
        if(strncmp(tag + 1, "ite", length) == 0
        || strncmp(tag + 1, "ode", length) == 0
        || strncmp(tag + 1, "ommand", length) == 0)
        {
            return true;
        }
        break;

    case 'd':
        // <data>, <del>, <dfn>
        if(strncmp(tag + 1, "ata", length) == 0
        || strncmp(tag + 1, "el", length) == 0
        || strncmp(tag + 1, "fn", length) == 0)
        {
            return true;
        }
        break;

    case 'e':
        // <em>
        if(length == 2 && tag[1] == 'm')
        {
            return true;
        }
        break;

    case 'f':
        // <font>
        if(strncmp(tag + 1, "ont", length) == 0)         // <font> deprecated in HTML 4.01
        {
            return true;
        }
        break;

    case 'i':
        // <i>, <img>, <ins>, <isindex>
        if(tag[1] == '\0'
        || strncmp(tag + 1, "mg", length) == 0
        || strncmp(tag + 1, "ns", length) == 0
        || strncmp(tag + 1, "sindex", length) == 0)      // <isindex> deprecated in HTML 4.01
        {
            return true;
        }
        break;

    case 'k':
        // <kbd>
        if(tag[1] == 'b' && tag[2] == 'd' && tag[3] == '\0')
        {
            return true;
        }
        break;

    case 'l':
        // <label>
        if(strncmp(tag + 1, "abel", length) == 0)
        {
            return true;
        }
        break;

    case 'm':
        // <map>, <mark>, <meter>
        if((tag[1] == 'a' && tag[2] == 'p' && tag[3] == '\0')
        || strncmp(tag + 1, "ark", length) == 0
        || strncmp(tag + 1, "eter", length) == 0)
        {
            return true;
        }
        break;

    case 'n':
        // <nobr>
        if(strncmp(tag + 1, "obr", length) == 0)     // <nobr> deprecated in HTML 5
        {
            return true;
        }
        break;

    case 'p':
        // <progress>
        if(strncmp(tag + 1, "rogress", length) == 0)
        {
            return true;
        }
        break;

    case 'q':
        // <q>
        if(tag[1] == '\0')
        {
            return true;
        }
        break;

    case 'r':
        // <rb>, <rp>, <rt>, <rtc>, <ruby>
        if(((tag[1] == 'b' || tag[1] == 'p' || tag[1] == 't') && tag[2] == '\0')
        || (tag[1] == 't' || tag[2] == 'c' || tag[3] == '\0')
        || strncmp(tag + 1, "uby", length) == 0)
        {
            return true;
        }
        break;

    case 's':
        // <s>, <samp>, <script>, <select>, <small>, <span>, <strike>, <strong>, <style>, <sub>, <sup>
        if(length == 1                              // <s> deprecated in HTML 4.01
        || strncmp(tag + 1, "amp", length) == 0
        || strncmp(tag + 1, "cript", length) == 0
        || strncmp(tag + 1, "elect", length) == 0
        || strncmp(tag + 1, "mall", length) == 0
        || strncmp(tag + 1, "pan", length) == 0
        || strncmp(tag + 1, "trike", length) == 0   // <strike> depreacated in HTML 4.01
        || strncmp(tag + 1, "trong", length) == 0
        || strncmp(tag + 1, "tyle", length) == 0
        || strncmp(tag + 1, "ub", length) == 0
        || strncmp(tag + 1, "up", length) == 0)
        {
            return true;
        }
        break;

    case 't':
        // <time>, <tt>
        if(strncmp(tag + 1, "ime", length) == 0
        || (length == 2 && tag[1] == 't'))      // <tt> deprecated in HTML 5
        {
            return true;
        }
        break;

    case 'u':
        // <u>
        if(length == 1)                  // <u> deprecated in HTML 4.01
        {
            return true;
        }
        break;

    case 'v':
        // <var>
        if(length == 3 && tag[1] == 'a' && tag[2] == 'r')
        {
            return true;
        }
        break;

    case 'w':
        // <wbr>
        if(length == 3 && tag[1] == 'b' && tag[2] == 'r')  // somehow <wbr> is marked as obsolete in HTML 5... TBD
        {
            return true;
        }
        break;

    }

    return false;
}


/** \brief Debug function
 *
 * This function is just for debug purposes. It can be used to make sure
 * that the resources you are expecting to exist are indeed available.
 * You may find it with the wrong path, for example.
 *
 * \param[in] out  An output stream such as std::err.
 */
void snap_child::show_resources(std::ostream & out)
{
    QDirIterator it(":", QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        out << it.next() << "\n";
    }
}


void snap_child::extract_resource(QString const & resource_name, QString const & output_filename)
{
    // TBD: should we make sure this is a resource?
    QFile resource(resource_name);
    if(!resource.open(QIODevice::ReadOnly))
    {
        die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                "Resource Unavailable",
                QString("Somehow resource \"%1\" could not be loaded.").arg(resource_name),
                "The resource name is wrong, maybe the ':' is missing at the start?");
        NOTREACHED();
    }

    // read the entire file
    QByteArray const data(resource.readAll());

    // create the output file
    QFile out(output_filename);
    if(!out.open(QIODevice::WriteOnly))
    {
        die(snap_child::http_code_t::HTTP_CODE_INTERNAL_SERVER_ERROR,
                "I/O Error",
                QString("Somehow we could not create output file \"%1\".").arg(output_filename),
                "The resource name is wrong, maybe the ':' is missing at the start?");
        NOTREACHED();
    }

    // save the resource
    out.write(data);

    // Qt closes both files automatically
}


} // namespace snap

// vim: ts=4 sw=4 et
