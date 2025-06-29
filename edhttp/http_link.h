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
#pragma once

// C++
//
#include    <map>
#include    <string>



namespace edhttp
{



class http_link
{
public:
    typedef std::map<std::string, http_link>        map_t;
    typedef std::map<std::string, std::string>      param_t;

                        http_link(std::string const & link, std::string const & rel);

    std::string         get_name() const;

    void                set_redirect(bool redirect = true);
    bool                get_redirect() const;

    void                add_param(std::string const & name, std::string const & value);

    bool                has_param(std::string const & name) const;
    std::string         get_param(std::string const & name) const;

    param_t const &     get_params() const;

    std::string         to_http_header() const;

private:
    std::string         f_link = std::string();     // this link URI
    std::string         f_rel = std::string();      // rel attribute, a.k.a. link name
    bool                f_redirect = false;         // whether to include this link in a redirect header
    param_t             f_params = param_t();       // other attributes
};



} // namespace edhttp
// vim: ts=4 sw=4 et
