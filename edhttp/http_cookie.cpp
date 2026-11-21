// Copyright (c) 2011-2026  Made to Order Software Corp.  All Rights Reserved
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
#include    "edhttp/names.h"
#include    "edhttp/token.h"
#include    "edhttp/uri.h"
#include    "edhttp/utils.h"


// libutf8
//
#include    <libutf8/libutf8.h>


// libtld
//
#include    <libtld/tld.h>


// snapdev
//
#include    <snapdev/hexadecimal_string.h>
#include    <snapdev/compare_switch_string.h>


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
 * cookie "Foo" can coexist (although it certainly should not be used!)
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
 * \param[in] name  The name of the cookie.
 * \param[in] value  The value of the cookie. It can be binary data.
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
        throw cookie_parse_exception("cookie name cannot be empty or include a reserved character.");
    }

    // TODO: here f_snap would always be nullptr but in snap we could
    //       determine a cookie domain in this way
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
 * The string can include absolute any bytes.
 *
 * \param[in] value  The new value of the cookie.
 */
void http_cookie::set_value(std::string const & value)
{
    if(!is_valid_value(value, true))
    {
        throw cookie_parse_exception("cookie value cannot include special characters, double quotes, ';', or spaces (1).");
    }

    if(!value.empty()
    && value.front() == ' ' || value.front() == '\t'
    || value.back() == ' ' || value.back() == '\t')
    {
        // without the quote, the whitespaces would be removed
        //
        f_value = '"' + value + '"';
    }
    else
    {
        f_value = value;
    }
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
void http_cookie::set_domain(std::string domain)
{
    if(domain.empty())
    {
        f_domain.clear();
        return;
    }

    std::size_t max_len(domain.length());
    if(max_len > 0 && domain[0] == '.')
    {
        domain.erase(0, 1);
        max_len = domain.length();
    }
    if(max_len == 0)
    {
        throw cookie_parse_exception("the domain of a cookie cannot just be \".\".");
    }

    // test special domains first (such as localhost)
    //
    if(!is_accepted_domain(domain))
    {
        tld_info info;
        char const * fd(domain.c_str());
        tld_result const r(::tld(fd, &info));
        if(r != TLD_RESULT_SUCCESS)
        {
            // the URI is invalid or it doesn't end with a known TLD
            //
            throw cookie_parse_exception("invalid domain name.");
        }
    }

    // TBD: is the tld() call above sufficient and we can drop this for()
    //      loop altogether? (I think it is, let's write a test?)
    //
    //for(std::size_t i(0); i < max_len; ++i)
    //{
    //    char c(domain[i]);
    //    if((c < 'A' || c > 'Z')
    //    && (c < 'a' || c > 'z')
    //    && (c < '0' || c > '9')
    //    && c != '.' && c != '-' && c != '_')
    //    {
    //        throw cookie_parse_exception("the domain of a cookie must only include domain name compatible characters.");
    //    }
    //}

    f_domain = domain;
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
    if(!libutf8::is_valid_utf8(path))
    {
        throw cookie_parse_exception("the path of a cookie must only include ASCII characters except controls, ',' and ';'.");
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
    //
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
    // invalid expiration date; will not be included which makes the cookie
    // a session cookie (deleted when the window is closed)
    //
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
 * \sa set_expire_in()
 * \sa get_expire();
 * \sa get_type();
 */
void http_cookie::set_expire(std::string const & date_time)
{
    time_t const seconds(string_to_date(date_time));
    if(seconds == -1 && errno != 0)
    {
        // TODO: show the errno and the input so we know why it failed
        //
        throw cookie_parse_exception("cookie expiration time invalid.");
    }

    if(seconds < 0)
    {
        // definitely too late
        //
        f_expire = 0;
    }
    else
    {
        time_t const now(time(nullptr));
        if(seconds - now > COOKIE_MAXIMUM_TIMEOUT)
        {
            // save 'now + 1 year' instead of date_time which is further in
            // the future and thus not HTTP 1.1 compatible
            //
            f_expire = now + COOKIE_MAXIMUM_TIMEOUT;
        }
        else
        {
            f_expire = seconds;
        }
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
 * \sa get_type();
 */
void http_cookie::set_expire_in(std::int64_t seconds)
{
    // clamp to tad over 1 year (max. allowed by HTTP/1.1)
    //
    if(seconds > COOKIE_MAXIMUM_TIMEOUT)
    {
        seconds = COOKIE_MAXIMUM_TIMEOUT;
    }

    time_t const now(time(nullptr));
    f_expire = now + seconds;
}


/** \brief Set the same site parameter of the cookie.
 *
 * A cookie can be required to only be sent to the exact same site. It can
 * also be made very lax so it gets sent to any domain.
 *
 * \param[in] same_site  The new same site setup.
 */
void http_cookie::set_same_site(http_same_site_t same_site)
{
    f_same_site = same_site;
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
http_cookie_type_t http_cookie::get_type() const
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


http_same_site_t http_cookie::get_same_site() const
{
    return f_same_site;
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


bool http_cookie::is_valid_value(std::string const & value, bool strict)
{
    // value restrictions are based on RFC6265 and RFC6265bis
    //
    //     cookie-value = *cookie-octet / ( DQUOTE *cookie-octet DQUOTE )
    //
    bool const quoted(value.length() >= 2
                   && value.front() == '"'
                   && value.back() == '"');

    int const max(value.length() - (quoted ? 1 : 0));
    for(int idx(quoted ? 1 : 0); idx < max; ++idx)
    {
        unsigned char c(value[idx]);
        if(strict)
        {
            if(c < 0x21
            || c > 0x7E
            || c == '"'
            || c == ','
            || c == ';'
            || c == '\\')
            {
                return false;
            }
        }
        else
        {
            // non-strict allows for anything except controls and ';'
            //
            if(c == ';'
            || c == 0x7F
            || (c < 0x20 && c != '\t'))
            {
                return false;
            }
        }
    }

    return true;
}


/** \brief Create a cookie read from a header line.
 *
 * This function parses a `Set-Cookie: ...` line sent to us by a server.
 * The function expects the value of the `Set-Cookie: ...` field, not
 * the entire line including the `Set-Cookie` token itself.
 *
 * The grammar is as follow:
 *
 * \code
 *     set-cookie-header = "Set-Cookie:" SP set-cookie-string
 *     set-cookie-string = cookie-pair *( ";" SP cookie-av )
 *     cookie-pair       = cookie-name "=" cookie-value
 *     cookie-name       = token
 *     cookie-value      = *cookie-octet / ( DQUOTE *cookie-octet DQUOTE )
 *     cookie-octet      = %x21 / %x23-2B / %x2D-3A / %x3C-5B / %x5D-7E
 *                           ; US-ASCII characters excluding CTLs,
 *                           ; whitespace DQUOTE, comma, semicolon,
 *                           ; and backslash
 *     token             = <token, defined in [RFC2616], Section 2.2>
 *
 *     cookie-av         = expires-av / max-age-av / domain-av /
 *                         path-av / secure-av / httponly-av /
 *                         samesite-av / extension-av
 *     expires-av        = "Expires=" sane-cookie-date
 *     sane-cookie-date  = <rfc1123-date, defined in [RFC2616], Section 3.3.1>
 *     max-age-av        = "Max-Age=" non-zero-digit *DIGIT
 *                           ; In practice, both expires-av and max-age-av
 *                           ; are limited to dates representable by the
 *                           ; user agent.
 *     non-zero-digit    = %x31-39
 *                           ; digits 1 through 9
 *     domain-av         = "Domain=" domain-value
 *     domain-value      = <subdomain>
 *                           ; defined in [RFC1034], Section 3.5, as
 *                           ; enhanced by [RFC1123], Section 2.1
 *     path-av           = "Path=" path-value
 *     path-value        = <any CHAR except CTLs or ";">
 *     secure-av         = "Secure"
 *     httponly-av       = "HttpOnly"
 *     extension-av      = <any CHAR except CTLs or ";">
 *     samesite-av       = "SameSite" BWS "=" BWS samesite-value
 *     samesite-value    = "Strict" / "Lax" / "None"
 * \endcode
 *
 * Note that the grammar must be followed to the letter for a server
 * to be supported any number of user agents. Especially, the SP just
 * after the ':' character should be viewed as an OWS (optional white
 * space) by a reader.
 *
 * \param[in] header  The Set-Cookie header value to be converted.
 *
 * \return A new cookie with \p header saved in it.
 */
http_cookie http_cookie::from_http_header(std::string const & header)
{
    if(has_controls(header, true, true))
    {
        throw cookie_parse_exception("cookie data includes control characters.");
    }

    char const * h(header.c_str());

    auto is_white_space = [](char c)
    {
        return c == ' ' || c == '\t';
    };

    // skip spaces between the ':' and the cookie name
    // (these should already be removed by the caller, but just in case)
    //
    for(; is_white_space(*h); ++h);

    // capture the start and end positions of the cookie name
    //
    char const * s(h);
    for(; *h != '=' && *h != '\0'; ++h);
    if(*h != '=')
    {
        throw cookie_parse_exception("cookie name must be followed by an equal character.");
    }
    char const * e(h);
    for(; e > s && is_white_space(e[-1]); --e);
    std::string const name(s, e - s);
    if(!is_token(name))
    {
        throw cookie_parse_exception("cookie name must be a valid HTTP token.");
    }
    ++h;
    for(; is_white_space(*h); ++h);

    // read the value, it can be ASCII (except for a few characters)
    //
    // because of the white space trimming, we do this in two steps
    //
    s = h;
    for(; *h != '\0' && *h != ';'; ++h);
    e = h;
    for(; e > s && is_white_space(e[-1]); --e);
    // we're expected to keep and send back the value quoted in a similar way
    //if(e > s + 1 && *s == '"' && e[-1] == '"')
    //{
    //    ++s;
    //    --e;
    //}
    std::string const value(s, e - s);
    if(*h == ';')
    {
        ++h;
    }

    // now we can create the cookie
    //
    http_cookie result(name, value);
std::cerr << "--- got cookie name [" << name << "] = value [" << value << "]\n";

    // next we parse the parameters such as Secure or Path=/a/b/c ...
    //
    for(;;)
    {
        for(; is_white_space(*h); ++h);
        if(*h == '\0')
        {
            break;
        }

        // read the parameter name
        //
        s = h;
        for(; *h != '=' && *h != ';' && *h != '\0'; ++h);
        e = h;
        for(; e > s && is_white_space(e[-1]); --e);
        std::string const param_name(s, e - s);
std::cerr << "--- got param name [" << param_name << "]\n";
        bool const has_value(*h == '=');
        std::string param_value;
        if(has_value)
        {
            ++h;
            for(; is_white_space(*h); ++h);

            // read the value, it can be ASCII (except for a few characters)
            //
            // because of the white space trimming, we do this in two steps
            //
            s = h;
            for(; *h != '\0' && *h != ';'; ++h);
            e = h;
            for(; e > s && is_white_space(e[-1]); --e);
            param_value = std::string(s, e - s);

            // prevent controls & UTF-8
            //
            for(char const * v(param_value.c_str()); *v != '\0'; ++v)
            {
                switch(*v)
                {
                default:
                    if(static_cast<signed char>(*v) >= '!')
                    {
                        break;
                    }
                    [[fallthrough]];
                case 0x7F:
                    throw cookie_parse_exception("cookie parameter value cannot include special characters.");

                }
            }
        }
        if(*h == ';')
        {
            ++h;
        }

std::cerr << "--- got value [" << param_value << "] switching on [" << param_name << "] now\n";
        switch(param_name[0])
        {
        case 'd':
        case 'D':
            if(snapdev::compare_upper_exact_switch_string<"DOMAIN">(param_name))
            {
                if(!has_value || param_value.empty())
                {
                    throw cookie_parse_exception("the Expires parameter must be a valid date.");
                }
                result.set_domain(param_value);
            }
            break;

        case 'e':
        case 'E':
            if(snapdev::compare_upper_exact_switch_string<"EXPIRES">(param_name))
            {
                if(!has_value || param_value.empty())
                {
                    throw cookie_parse_exception("the Expires parameter must be a valid date.");
                }
                result.set_expire(param_value);
            }
            break;

        case 'h':
        case 'H':
            if(snapdev::compare_upper_exact_switch_string<"HTTPONLY">(param_name))
            {
                if(has_value)
                {
                    throw cookie_parse_exception("the Secure flag cannot be given a value.");
                }
                result.set_http_only();
            }
            break;

        case 'm':
        case 'M':
            if(snapdev::compare_upper_exact_switch_string<"MAX-AGE">(param_name))
            {
                if(!has_value || param_value.empty())
                {
                    throw cookie_parse_exception("the Max-Age parameter must be given a value.");
                }
                if(param_value[0] < '1' || param_value[0] > '9')
                {
                    throw cookie_parse_exception("the Max-Age parameter must start with a non-zero digit.");
                }
                std::int64_t seconds(0);
                s = param_value.c_str();
                while(*s != '\0')
                {
                    if(*s < '0' || *s > '9')
                    {
                    }
                    seconds *= 10;
                    seconds += *s - '0';
                    if(seconds >= COOKIE_MAXIMUM_TIMEOUT * 2)
                    {
                        throw cookie_parse_exception("the Max-Age parameter cannot be more than 400 days.");
                    }
                }
                result.set_expire_in(seconds);
            }
            break;

        case 'p':
        case 'P':
            if(snapdev::compare_upper_exact_switch_string<"PATH">(param_name))
            {
                // Note: certain grammars allow for an empty string here, but
                //       that is not really allowed
                //
                if(!has_value || param_value.empty() || param_value[0] != '/')
                {
                    throw cookie_parse_exception("the Expires parameter must be a valid date.");
                }
                result.set_path(param_value);
            }
            break;

        case 's':
        case 'S':
            if(snapdev::compare_upper_exact_switch_string<"SECURE">(param_name))
            {
                if(has_value)
                {
                    throw cookie_parse_exception("the Secure flag cannot be given a value.");
                }
                result.set_secure();
            }
            else if(snapdev::compare_upper_exact_switch_string<"SAMESITE">(param_name))
            {
                if(!has_value || param_value.empty())
                {
                    throw cookie_parse_exception("the SameSite parameter must be one of Strict, Lax, or None.");
                }
                if(snapdev::compare_upper_exact_string<"STRICT">(param_value))
                {
                    result.set_same_site(http_same_site_t::HTTP_SAME_SITE_STRICT);
                }
                else if(snapdev::compare_upper_exact_string<"LAX">(param_value))
                {
                    result.set_same_site(http_same_site_t::HTTP_SAME_SITE_LAX);
                }
                else if(snapdev::compare_upper_exact_string<"NONE">(param_value))
                {
                    result.set_same_site(http_same_site_t::HTTP_SAME_SITE_NONE);
                }
                else
                {
                    // used given value not recognized, use the default instead
                    //
                    result.set_same_site(http_same_site_t::HTTP_SAME_SITE_DEFAULT);
                }
            }
            break;

        }
    }

    if(result.get_same_site() == http_same_site_t::HTTP_SAME_SITE_NONE
    && !result.get_secure())
    {
        throw cookie_parse_exception("the SameSite=None parameter requires the Secure flag to be set.");
    }

    return result;
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
    //
    std::string result(g_name_edhttp_field_set_cookie);
    result += ':';
    result += f_name;
    result += '=';
    result += f_value;

    // WARNING: although spaces are technically not required after each ";"
    //          character; we include them because some user agents may not
    //          function properly at all otherwise (yeah, I know...)
    //
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
        result += urlencode(f_path, g_name_edhttp_accepted_characters_cookie);
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

    return result;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
