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

// self
//
#include    <edhttp/parametrized_value.h>



namespace edhttp
{



typedef std::uint32_t           parametrized_value_mode_t;

constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_NO_BARE_ITEM                   = 0x0001;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_ONLY            = 0x0002;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_BARE_ITEM_VALUE_ONLY           = 0x0004;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_VALUE           = 0x0008;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_OPTIONAL_VALUE  = 0x0010;

constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_NO_PARAMETERS                  = 0x0020;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_PARAMETER_NAME_ONLY            = 0x0040;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_PARAMETER_MANDATORY_VALUE      = 0x0080;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_PARAMETER_OPTIONAL_VALUE       = 0x0100;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_QUOTED_STRING                  = 0x0200;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_QUOTED_STRING_KEEP_QUOTES      = 0x0400;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_QUOTED_STRING_COOKIE_SEMICOLON = 0x0800;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_QUOTED_STRING_ESCAPE_CHARACTER = 0x1000;
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_QUOTE_NON_TOKEN_VALUE          = 0x2000;

constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_STRICT_PARAMETRIZED_VALUE_LIST =
                                                  PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_OPTIONAL_VALUE
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING_ESCAPE_CHARACTER;

// alternate services
//
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_STRICT_ALT_SVC =
                                                  PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_VALUE
                                                | PARAMETRIZED_VALUE_MODE_PARAMETER_MANDATORY_VALUE
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING_ESCAPE_CHARACTER;

// cache controls do not really have a value and parameters; it's like all
// parameters
//
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_CACHE_CONTROL =
                                                  PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_OPTIONAL_VALUE
                                                | PARAMETRIZED_VALUE_MODE_NO_PARAMETERS
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING_ESCAPE_CHARACTER;

// cookies support quoted strings without any escape and not only that,
// the ';' and ',' characters are still viewed as separators
//
//    name = value ';' param [= value]
//    value = 1*CHAR - [;,]
//
// the value is trimmed, but the quotes are kept
//
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_COOKIE =
                                                  PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_VALUE
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING_KEEP_QUOTES
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING_COOKIE_SEMICOLON;

// languages are defined with a name viewed as a value
// and an optional q=... parameter
//
//     name ';' q=<level>
//
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_LANGUAGE =
                                                  PARAMETRIZED_VALUE_MODE_BARE_ITEM_VALUE_ONLY
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING
                                                | PARAMETRIZED_VALUE_MODE_QUOTED_STRING_ESCAPE_CHARACTER;

// vary is just a comma separated list of names
//
constexpr parametrized_value_mode_t const   PARAMETRIZED_VALUE_MODE_VARY =
                                                  PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_ONLY;



// for HTTP_ACCEPT_ENCODING, HTTP_ACCEPT_LANGUAGE, HTTP_ACCEPT
class parametrized_value_list
{
public:
                            parametrized_value_list(
                                  std::string const & str = std::string()
                                , parametrized_value_mode_t mode = PARAMETRIZED_VALUE_MODE_STRICT_PARAMETRIZED_VALUE_LIST);

    bool                    parse(
                                  std::string const & str
                                , bool reset = false);
    std::string const &     get_string() const { return f_original; }
    parametrized_value::level_t
                            get_level(std::string const & name);
    void                    sort_by_level();
    parametrized_value *    get_part(std::string const & name) const;
    parametrized_value *    get_part_by_value(std::string const & name) const;
    parametrized_value::vector_t &
                            get_parts() { return f_parts; }
    parametrized_value::vector_t const &
                            get_parts() const { return f_parts; }
    std::string             to_string() const;
    std::string const &     error_messages() const { return f_error_messages; }

private:
    parametrized_value_mode_t const
                            f_mode;
    std::string             f_original = std::string();
    parametrized_value::vector_t
                            f_parts = parametrized_value::vector_t(); // do NOT use a map, we want to keep the parameters in order
    std::string             f_error_messages = std::string();
};



} // namespace edhttp
// vim: ts=4 sw=4 et
