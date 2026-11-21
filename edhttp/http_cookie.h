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
#pragma once

// C++
//
#include    <cstdint>
#include    <string>
#include    <vector>



namespace edhttp
{



constexpr time_t const                      COOKIE_MAXIMUM_TIMEOUT = 86400LL * 400LL;


enum class http_cookie_type_t
{
    HTTP_COOKIE_TYPE_PERMANENT,
    HTTP_COOKIE_TYPE_SESSION,
    HTTP_COOKIE_TYPE_DELETE,
};


enum class http_same_site_t
{
    HTTP_SAME_SITE_UNKNOWN,
    HTTP_SAME_SITE_DEFAULT,
    HTTP_SAME_SITE_STRICT,
    HTTP_SAME_SITE_LAX,
    HTTP_SAME_SITE_NONE,
};


class http_cookie
{
public:
    typedef std::vector<http_cookie>        vector_t;

                        http_cookie(); // for std::map to work; DO NOT USE!
                        http_cookie(std::string const & name, std::string const & value = "");

    void                set_value(std::string const & value);
    void                set_same_site(http_same_site_t same_site);
    void                set_domain(std::string domain);
    void                set_path(std::string const & path);
    void                set_delete();
    void                set_session();
    void                set_expire(std::string const & date_time);
    void                set_expire_in(std::int64_t seconds);
    void                set_secure(bool secure = true);
    void                set_http_only(bool http_only = true);

    std::string const & get_name() const;
    std::string const & get_value() const;
    http_cookie_type_t  get_type() const;
    http_same_site_t    get_same_site() const;
    std::string const & get_domain() const;
    std::string const & get_path() const;
    time_t              get_expire() const;
    bool                get_secure() const;
    bool                get_http_only() const;

    static http_cookie  from_http_header(std::string const & header);
    std::string         to_http_header() const;

    static bool         is_valid_value(std::string const & value, bool strict);

private:
    std::string         f_name = std::string();         // name of the cookie
    std::string         f_value = std::string();        // the cookie value (binary buffer)
    std::string         f_domain = std::string();       // domain for which the cookie is valid
    std::string         f_path = std::string();         // path under which the cookie is valid
    time_t              f_expire = -1;                  // when to expire the cookie (if null, session, if past delete)
    http_same_site_t    f_same_site = http_same_site_t::HTTP_SAME_SITE_UNKNOWN;
    bool                f_secure = false;               // only valid on HTTPS
    bool                f_http_only = false;            // JavaScript cannot access this cookie
};



} // namespace edhttp
// vim: ts=4 sw=4 et
