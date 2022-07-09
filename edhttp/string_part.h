// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    <vector>



namespace edhttp
{


class string_part
{
public:
    typedef float               level_t;
    typedef std::vector<string_part> vector_t; // do NOT use a map, we want to keep them in order!

    // an authoritative document at the IANA clearly says that
    // the default level (quality value) is 1.0f.
    //
    // Note: we use functions because these values are floating points and
    //       for some odd reasons C++ doesn't support such as plain constants
    //
    static level_t constexpr    DEFAULT_LEVEL() { return 1.0f; }

    static level_t constexpr    UNDEFINED_LEVEL() { return -1.0f; }

                                string_part(std::string const & name);

    std::string const &         get_name() const;
    std::string const &         get_value() const;  // in case the first name has a "name=value" part
    void                        set_value(std::string const & value);
    level_t                     get_level() const;
    void                        set_level(level_t const level);
    std::string                 get_parameter(std::string const & name) const;
    void                        add_parameter(std::string const & name, std::string const & value);
    std::string                 to_string() const;

    bool                        operator < (string_part const & rhs) const;

    static char                 value_require_quotes(std::string const & value);

private:
    typedef std::map<std::string, std::string>      parameters_t;

    std::string                 f_name = std::string();
    std::string                 f_value = std::string();
    level_t                     f_level = DEFAULT_LEVEL(); // i.e.  q=1.0
    // TODO add support for any other parameter
    parameters_t                f_param = parameters_t();
};


} // namespace edhttp
// vim: ts=4 sw=4 et
