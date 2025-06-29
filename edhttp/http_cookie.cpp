// Copyright (c) 2011-2025  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/edhttp
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


// self
//
#include    "edhttp/http_cookie.h"

#include    "edhttp/exception.h"
#include    "edhttp/http_date.h"
#include    "edhttp/mkgmtime.h"
#include    "edhttp/names.h"
#include    "edhttp/token.h"


// snapdev
//
#include    <snapdev/hexadecimal_string.h>


// C
//
#include    <sys/time.h>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{


namespace
{



void safe_comment(std::string & result, std::string const & comment)
{
    for(auto const c : comment)
    {
        if(c > ' ' && c != '"' && c < 0x7F)
        {
            result += c;
        }
    }
}



} // no name namespace


/** \brief Create an invalid cookie (no name).
 *
 * This function creates an invalid cookie. It is here because the QMap
 * implementation requires it. You should not use it otherwise since the
 * f_snap pointer will be set to NULL and is likely going to crash your
 * server.
 */
http_cookie::http_cookie()
{
}


/** \brief Initializes the cookie.
 *
 * This function initializes the cookie. The default for any cookie
 * is to have the following parameters:
 *
 * \li The name as supplied here.
 * \li The cookie contents, empty by default.
 * \li The domain set to this website full domain unless the user defined a
 *     cookie domain as a site parameter (SNAP_NAME_CORE_COOKIE_DOMAIN).
 * \li A path set to "/".
 * \li No expiration date (i.e. cookie is for this session only, deleted when browser is closed)
 * \li Not secure
 * \li Not limited to HTTP
 *
 * Note that the name of the cookie is set when you create it. I cannot be
 * changed later.
 *
 * \note
 * The name of a cookie is case sensitive. In other words cookie "foo" and
 * cookie "Foo" can cohexist (although it certainly should not be used!)
 *
 * \warning
 * The cookie domain cannot be determined without a pointer to the snap
 * child object. If you do not have access to that pointer, make sure that
 * an object that has access calls the set_domain() at some point or the
 * cookie is likely to fail.
 *
 * \todo
 * If there is a redirect (i.e. we show website A but the user was
 * accessible website B,) then the default domain name will be wrong.
 * We should have a way to retrieve the primary domain name for this
 * purpose.
 *
 * \param[in] snap  The snap child creating this cookie.
 * \param[in] name  The name of the cookie.
 * \param[in] value  The value of the cookie. To set a binary value, use set_value() with a QByteArray instead.
 *
 * \sa set_value()
 * \sa set_domain()
 * \sa set_path()
 * \sa set_delete()
 * \sa set_session()
 * \sa set_expire()
 * \sa set_expire_in()
 * \sa set_secure()
 * \sa set_http_only()
 * \sa set_comment()
 * \sa set_comment_url()
 */
http_cookie::http_cookie(std::string const & name, std::string const & value)
    : f_name(name)
    , f_path("/")
{
    if(!is_token(f_name))
    {
        throw cookie_parse_exception("cookie name cannot be empty, start with '$', or icnlude a reserved character.");
    }

    // TODO: here f_snap would always be nullptr but in snap we still need
    //       to fix this
    //
    //if(f_snap)
    //{
    //    libdbproxy::value cookie_domain(f_snap->get_site_parameter(snap::get_name(name_t::SNAP_NAME_CORE_COOKIE_DOMAIN)));
    //    if(cookie_domain.nullValue())
    //    {
    //        // use the fully qualified website domain name
    //        f_domain = f_snap->get_website_key();
    //    }
    //    else
    //    {
    //        f_domain = cookie_domain.stringValue();
    //    }
    //}

    set_value(value);
}


/** \brief Set the value of the cookie.
 *
 * This function allows you to change the value of the cookie.
 * By default it is set to an empty string unless you define
 * the value in the constructor.
 *
 * The value is encoded using the usual urlencode mechanism
 * as to avoid problems with controls and other data.
 *
 * We support binary data by calling the set_value() with
 * the QByteArray parameter.
 *
 * \param[in] value  The new value of the cookie.
 */
void http_cookie::set_value(std::string const & value)
{
    f_value = value;
}


/** \brief Set the cookie domain.
 *
 * This function is used to set the cookie domain although
 * it generally should not be required because the constructor
 * already does so automatically for you using the website
 * key as defined in the snap child.
 *
 * If you want to support many sub-domains, then you should
 * define the cookie domain as a site parameter instead.
 *
 * \note
 * Using the wrong domain name does nothing as the browser
 * ignores cookies with wrong domain names.
 *
 * \param[in] domain  The new domain name for this cookie.
 *
 * \sa get_domain();
 */
void http_cookie::set_domain(std::string const & domain)
{
    f_domain = domain;

    // TODO?
    // Enhance the check so we don't accept two periods one after
    // another or two dashes or a name that starts/ends with invalid
    // characters (i.e. cannot start/end with a dash.) Although some
    // of those would not be necessary if we check the domain against
    // the website domain name.
    int max_len(f_domain.length());
    if(max_len > 0 && f_domain[0] == '.')
    {
        f_domain = f_domain.substr(1);
        max_len = f_domain.length();
    }
    if(max_len == 0)
    {
        throw cookie_parse_exception("the domain of a cookie cannot be empty.");
    }
    for(int i(0); i < max_len; ++i)
    {
        // TODO: use libtld instead
        // TBD -- How is that supporting Unicode characters in domain names?
        char c(f_domain[i]);
        if((c < 'A' || c > 'Z')
        && (c < 'a' || c > 'z')
        && (c < '0' || c > '9')
        && c != '.' && c != '-' && c != '_')
        {
            throw cookie_parse_exception("the domain of a cookie must only include domain name compatible characters.");
        }
    }

    // TODO: add a check to make sure this is a valid domain name
    //       (i.e. to the minimum "domain + TLD")
}


/** \brief Set the path where the cookie is to be saved.
 *
 * By default the cookie is setup to be viewed everywhere (i.e. the path
 * is set to "/".) To constrain the cookie to a section of a website
 * set the path to that section here.
 *
 * This can be useful to setup a secure cookie so administrators can
 * get a special cookie that really only works in the administrative
 * part of the website.
 *
 * \param[in] path  The new cookie path.
 *
 * \sa get_path();
 */
void http_cookie::set_path(std::string const & path)
{
    // TODO:
    // TBD -- How is that supporting Unicode characters in paths?
    // (we may have to change them to some %XX syntax
    int max_len(path.length());
    for(int i(0); i < max_len; ++i)
    {
        char c(f_domain[i]);
        if((c < ' ' || c > '~')
        && c != ',' && c != ';')
        {
            throw cookie_parse_exception("the path of a cookie must only include ASCII characters except controls, ',' and ';'.");
        }
    }

    f_path = path;
}


/** \brief Mark the cookie for deletion.
 *
 * This function sets the expiration date in the past so the cookie
 * gets deleted. It is usual to set the date to January 1, 1970
 * (i.e. Unix time of 0) and we do so here.
 *
 * \sa set_expire();
 * \sa set_session();
 * \sa get_type();
 */
void http_cookie::set_delete()
{
    // January 1, 1970 00:00:00 is represented as 0
    f_expire = 0;
}


/** \brief Mark the cookie as a session cookie.
 *
 * This function invalidates the expiration date of the cookie, which is the
 * default. When the expiration date is invalid, it is not sent to the browser
 * and it transforms the cookie in a session cookie.
 *
 * This type of cookie only lasts until you close the browser window. For
 * sensitive accounts (dealing with e-Commerce and similar) it is a good idea
 * to use this form of cookie.
 *
 * \sa set_delete();
 * \sa set_expire();
 * \sa get_type();
 */
void http_cookie::set_session()
{
    // invalid expiration date
    f_expire = -1;
}


/** \brief Set the expiration date of the cookie.
 *
 * This function changes the expiration date to the specified date and time.
 *
 * In most cases, it is easier to use the set_expire_in() function which uses
 * now + the specified number of seconds. The result will be the same either
 * way because we only send an explicit expiration date and not a Max-Age
 * parameter.
 *
 * In order to create a session cookie (the default), you may set the date
 * to an invalid date or call the set_session() function:
 *
 * \code
 *   QDateTime invalid;
 *   cookie.set_expiration(invalid);
 * \endcode
 *
 * To delete a cookie, you can set the expiration date to a date in the past.
 * This is also achieved by calling the set_delete() function.
 *
 * \note
 * If the date represents a date more than 1 year in the future, then it
 * gets clamped.
 *
 * \param[in] date_time  The new expiration date and time.
 *
 * \sa set_session();
 * \sa set_delete();
 * \sa get_expire();
 * \sa get_type();
 */
void http_cookie::set_expire(std::string const & date_time)
{
    time_t const now(time(nullptr));
    time_t const seconds(string_to_date(date_time));
    if(seconds - now > 86400LL * 365LL)
    {
        // save 'now + 1 year' instead of date_time which is further in
        // the future and thus not HTTP 1.1 compatible
        //
        f_expire = now + 86400LL * 365LL;
    }
    else if(seconds < 0)
    {
        // the date is past, that means we want to delete
        // (TBD: should this be an error instead?)
        //
        f_expire = 0;
    }
    else
    {
        f_expire = seconds;
    }
}


/** \brief This function sets the expiration date seconds in the future.
 *
 * This function is most often the one used and allows you to set the
 * expiration date of the cookie the specified number of seconds in
 * the future.
 *
 * The function makes use of the snap child start date plus that number
 * of seconds, but it sends the cookie with an Expire field (i.e. we do
 * not make use of the Max-Age field.)
 *
 * \note
 * If the HTTP cookie object was passed a pointer to the snap child
 * object, then the request start date is used, otherwise the current
 * date is used as the fallback.
 *
 * \param[in] seconds  The number of seconds from now when the cookie expires.
 *
 * \sa set_expire();
 * \sa get_expire();
 * \sa set_delete();
 * \sa set_session();
 */
void http_cookie::set_expire_in(int64_t seconds)
{
    // clamp to 1 year (max. allowed by HTTP 1.1)
    if(seconds > 86400LL * 365LL)
    {
        seconds = 86400LL * 365LL;
    }

    time_t const now(time(nullptr));
    f_expire = now + seconds;
}


/** \brief Mark the cookie as secure.
 *
 * By default cookies are not marked as secure. Call this function with
 * the \p secure parameter set to true so the cookie only travels between
 * the browser and the server if SSL is used.
 *
 * Note that a secure cookie is not seen if the user decides to access
 * your website using the HTTP protocol (opposed to the HTTPS protocol.)
 * Websites make use of a secure cookie when they have a certificate.
 *
 * \note
 * Snap! implements ways to support logged in users on non-secure
 * connections, but with much lower rights.
 *
 * \param[in] secure  Whether the cookie should be made secure or not.
 *
 * \sa get_secure();
 */
void http_cookie::set_secure(bool secure)
{
    f_secure = secure;
}


/** \brief Set the HttpOnly flag.
 *
 * This function changes the value of the HttpOnly flag. This flag is
 * used by browsers to prevent cookies from being visible form JavaScript.
 * This is important to avoid all sorts of session highjack attacks.
 *
 * By default the cookies are visible by JavaScript and other client
 * supported languages.
 *
 * \param[in] http_only  Whether this cookie is only for HTTP.
 *
 * \sa get_http_only();
 */
void http_cookie::set_http_only(bool http_only)
{
    f_http_only = http_only;
}


/** \brief Set a comment.
 *
 * This function sets the comment of the cookie.
 *
 * In general this is verbatim information about the cookie in regard to
 * the user privacy.
 *
 * The set_comment_url() can also be used to set a page where the cookie
 * privacy information can be found.
 *
 * \param[in] comment  The comment about this cookie.
 *
 * \sa get_comment();
 */
void http_cookie::set_comment(std::string const & comment)
{
    f_comment = comment;
}


/** \brief Set a comment URL.
 *
 * This function sets the comment URL of the cookie. This is actually
 * made mandatory in the Snap! webserver.
 *
 * \param[in] comment_url  The URL to a page tha explains the cookie usage.
 *
 * \sa get_comment_url();
 */
void http_cookie::set_comment_uri(std::string const & comment_uri)
{
    f_comment_uri = comment_uri;
}


/** \brief Retrive the name of the cookie.
 *
 * The name of the cookie is set when you create it. It cannot be
 * changed.
 *
 * \return The name of the cookie.
 */
std::string const & http_cookie::get_name() const
{
    return f_name;
}


/** \brief Retrieve the cookie value.
 *
 * This function returns the cookie value as a QByteValue. If you set the cookie
 * as a string, then you can convert it back to a string with the following:
 *
 * \code
 * std::string const & v(cookie.get_value());
 * QString str(QString::fromUtf8(v.data(), v.size()));
 * \endcode
 *
 * \return A constant reference to the cookie contents.
 *
 * \sa set_value()
 */
std::string const & http_cookie::get_value() const
{
    return f_value;
}


/** \brief Get the current cookie type.
 *
 * Depending on how the expiration date is setup, the cookie may have
 * one of the following types:
 *
 * \li HTTP_COOKIE_TYPE_PERMANENT -- the expiration date and time is valid and in the future
 * \li HTTP_COOKIE_TYPE_SESSION -- the expiration date and time is not valid
 * \li HTTP_COOKIE_TYPE_DELETE -- the expiration date and time is in the past
 *
 * \return One of the HTTP_COOKIE_TYPE_... values.
 *
 * \sa set_expire()
 * \sa set_delete()
 * \sa set_session()
 * \sa get_expire()
 * \sa set_expire_in()
 */
http_cookie::http_cookie_type_t http_cookie::get_type() const
{
    if(f_expire < 0)
    {
        return http_cookie_type_t::HTTP_COOKIE_TYPE_SESSION;
    }
    if(f_expire == 0)
    {
        return http_cookie_type_t::HTTP_COOKIE_TYPE_DELETE;
    }
    return http_cookie_type_t::HTTP_COOKIE_TYPE_PERMANENT;
}


/** \brief Get the cookie domain information.
 *
 * This function retreives the current domain information of the cookie.
 * In general the domain is equal to the website key.
 *
 * \return The domain defined in this cookie.
 *
 * \sa set_domain();
 */
std::string const & http_cookie::get_domain() const
{
    return f_domain;
}


/** \brief Retrieve the path under which the cookie is valid.
 *
 * This function retrieves the current path for this cookie.
 * By default it is set to "/" which means the cookie is
 * available over the entire website. It is rarely necessary
 * to change this value.
 *
 * \return The current path for this cookie.
 *
 * \sa set_path();
 */
std::string const & http_cookie::get_path() const
{
    return f_path;
}


/** \brief Get the expiration date.
 *
 * This function returns the current expiration date. The date represents
 * different status of the cookie which can be determined by calling the
 * get_type() function.
 *
 * The default is an invalid cookie which means that the cookie is a
 * session cookie (lasts until the browser is closed.)
 *
 * \return The expiration date of this cookie.
 *
 * \sa set_expire();
 * \sa set_expire_in();
 * \sa set_delete();
 * \sa set_session();
 * \sa get_type();
 */
time_t http_cookie::get_expire() const
{
    return f_expire;
}


/** \brief Retrieve whether the cookie is secure.
 *
 * This function returns whether the cookie should only travel on a
 * secure (SSL) connection.
 *
 * The default is false (cookie is visible on non-secure connections.)
 *
 * \return true if the cookie is only for secure connections, false otherwise.
 *
 * \sa set_secure();
 */
bool http_cookie::get_secure() const
{
    return f_secure;
}


/** \brief Retrieve whether the cookie is only for HTTP.
 *
 * By default cookies are visible to JavaScript and other client languages.
 * This parameter can be used to hide the content of the cookie from praying
 * eyes and only allow the data to travel between the browser and server.
 *
 * The default is false.
 *
 * \return The current HttpOnly flag status.
 *
 * \sa set_http_only();
 */
bool http_cookie::get_http_only() const
{
    return f_http_only;
}


/** \brief Retrieve the cookie comment.
 *
 * This function returns the verbatim cookie comment.
 *
 * \return The verbatim comment of the cookie.
 *
 * \sa set_comment();
 */
std::string const & http_cookie::get_comment() const
{
    return f_comment;
}


/** \brief Retrieve the cookie comment URL.
 *
 * This function returns the cookie comment URL.
 *
 * \return The comment URL of the cookie.
 *
 * \sa set_comment_url();
 */
std::string const & http_cookie::get_comment_uri() const
{
    return f_comment_uri;
}


/** \brief Transform the cookie for the HTTP header.
 *
 * This function transforms the cookie so it works as an HTTP header.
 * It follows the HTTP 1.1 specifications, although it never makes
 * use of the Max-Age field.
 *
 * \return A valid HTTP cookie header.
 */
std::string http_cookie::to_http_header() const
{
    // Note: the name was already checked for invalid characters
    std::string result(g_name_edhttp_field_set_cookie);
    result += ": ";
    result += f_name;
    result += '=';

    for(auto const c : f_value)
    {
        if(c == 0x21
        || (c >= 0x23 && c <= 0x2B)
        || (c >= 0x2D && c <= 0x3A)
        || (c >= 0x3C && c <= 0x5B)
        || (c >= 0x5D && c <= 0x7E))
        {
            result += c;
        }
        else
        {
            // add the byte as %XX
            result += '%';
            result += snapdev::int_to_hex(c, true, 2);
        }
    }

    switch(get_type())
    {
    case http_cookie_type_t::HTTP_COOKIE_TYPE_PERMANENT:
        // compute date/time
        // HTTP format generates: Sun, 06 Nov 1994 08:49:37 GMT
        // (see http://tools.ietf.org/html/rfc2616#section-3.3.1)
        //
        result += "; ";
        result += g_name_edhttp_param_expires;
        result += '=';
        result += date_to_string(f_expire, date_format_t::DATE_FORMAT_HTTP);

        // Modern browsers are expected to use the Max-Age=... field
        // instead of the Expires to avoid potential date synchronization
        // problems between our server and the client
        // (see http://tools.ietf.org/html/rfc6265#section-4.1.2.2)
        //
        // TBD: although this works, we may want to know the exact
        //      intend of the person setting the expiration time and
        //      maybe use that amount (or even change our current
        //      expire to a max-age and calculate the date in Expires=...
        //      and not the one in Max-Age.)
        {
            time_t const max_age(f_expire - time(nullptr));
            if(max_age > 0)
            {
                result += "; ";
                result += g_name_edhttp_param_max_age;
                result += '=';
                result += std::to_string(max_age);
            }
        }
        break;

    case http_cookie_type_t::HTTP_COOKIE_TYPE_SESSION:
        // no Expires
        break;

    case http_cookie_type_t::HTTP_COOKIE_TYPE_DELETE:
        // no need to waste time computing that date
        result += "; ";
        result += g_name_edhttp_param_expires;
        result += '=';
        result += g_name_edhttp_jan1_1970;
        break;

    }

    if(!f_domain.empty())
    {
        // the domain sanity was already checked so we can save it as it here
        result += "; ";
        result += g_name_edhttp_param_domain;
        result += '=';
        result += f_domain;
    }

    if(!f_path.empty())
    {
        // the path sanity was already checked so we can save it as it here
        result += "; ";
        result += g_name_edhttp_param_path;
        result += '=';
        result += f_path;
    }

    if(f_secure)
    {
        result += "; ";
        result += g_name_edhttp_param_secure;
    }

    if(f_http_only)
    {
        result += "; ";
        result += g_name_edhttp_param_http_only;
    }

    if(!f_comment.empty())
    {
        // we need to escape all "bad" characters, not just quotes
        result += "; ";
        result += g_name_edhttp_param_comment;
        result += "=\"";
        safe_comment(result, f_comment);
        result += '"';
    }

    if(!f_comment_uri.empty())
    {
        // we need to escape all "bad" characters, not just quotes
        result += "; ";
        result += g_name_edhttp_param_comment_url;
        result += "=\"";
        safe_comment(result, f_comment_uri);
        result += '"';
    }

    return result;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
