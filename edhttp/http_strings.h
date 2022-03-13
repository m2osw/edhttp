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



// for HTTP_ACCEPT_ENCODING, HTTP_ACCEPT_LANGUAGE, HTTP_ACCEPT
class weighted_http_string
{
public:
    class part_t
    {
    public:
        typedef float               level_t;
        typedef std::vector<part_t> vector_t; // do NOT use a map, we want to keep them in order!

        // an authoritative document at the IANA clearly says that
        // the default level (quality value) is 1.0f.
        //
        static level_t constexpr    DEFAULT_LEVEL() { return 1.0f; }

        static level_t constexpr    UNDEFINED_LEVEL() { return -1.0f; }

                                    part_t();
                                    part_t(std::string const & name);

        std::string const &         get_name() const;
        std::string const &         get_value() const;  // in case the first name has a "name=value" part
        void                        set_value(std::string const & value);
        level_t                     get_level() const;
        void                        set_level(level_t const level);
        std::string                 get_parameter(std::string const & name) const;
        void                        add_parameter(std::string const & name, std::string const & value);
        std::string                 to_string() const;

        bool                        operator < (part_t const & rhs) const;

    private:
        typedef std::map<std::string, std::string>      parameters_t;

        std::string                 f_name = std::string();
        std::string                 f_value = std::string();
        level_t                     f_level = DEFAULT_LEVEL(); // i.e.  q=0.8
        // TODO add support for any other parameter
        parameters_t                f_param = parameters_t();
    };

                            weighted_http_string(std::string const & str = std::string());

    bool                    parse(std::string const & str, bool reset = false);
    std::string const &     get_string() const { return f_str; }
    part_t::level_t         get_level(std::string const & name);
    void                    sort_by_level();
    part_t::vector_t &      get_parts() { return f_parts; }
    part_t::vector_t const &get_parts() const { return f_parts; }
    std::string             to_string() const;
    std::string const &     error_messages() const { return f_error_messages; }

private:
    std::string             f_str = std::string();
    part_t::vector_t        f_parts = part_t::vector_t(); // do NOT use a map, we want to keep them in order
    std::string             f_error_messages = std::string();
};



} // namespace edhttp
// vim: ts=4 sw=4 et
