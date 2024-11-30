// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    <edhttp/string_part.h>



namespace edhttp
{



// for HTTP_ACCEPT_ENCODING, HTTP_ACCEPT_LANGUAGE, HTTP_ACCEPT
class weighted_http_string
{
public:
                            weighted_http_string(std::string const & str = std::string());

    bool                    parse(std::string const & str, bool reset = false);
    std::string const &     get_string() const { return f_str; }
    string_part::level_t    get_level(std::string const & name);
    void                    sort_by_level();
    string_part::vector_t & get_parts() { return f_parts; }
    string_part::vector_t const &
                            get_parts() const { return f_parts; }
    std::string             to_string() const;
    std::string const &     error_messages() const { return f_error_messages; }

private:
    std::string             f_str = std::string();
    string_part::vector_t   f_parts = string_part::vector_t(); // do NOT use a map, we want to keep them in order
    std::string             f_error_messages = std::string();
};



} // namespace edhttp
// vim: ts=4 sw=4 et
