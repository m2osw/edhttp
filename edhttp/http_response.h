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
#include    <map>
#include    <memory>
#include    <string>



namespace edhttp
{



enum class http_protocol_t
{
    UNKNOWN,

    HTTP_1_0,
    HTTP_1_1,
    HTTP_2,
    HTTP_3,
};


typedef std::uint16_t           http_response_code_t; // 1XX to 5XX or 0 when undefined

constexpr http_response_code_t const    HTTP_RESPONSE_CODE_UNDEFINED = 0;


class http_response
{
public:
    typedef std::shared_ptr<http_response>      pointer_t;

    std::string                 get_original_header() const;
    http_protocol_t             get_protocol() const;
    http_response_code_t        get_response_code() const;
    std::string                 get_http_message() const;
    bool                        has_header(std::string const & name) const;
    std::string                 get_header(std::string const & name) const;
    std::string                 get_response() const;

    void                        append_original_header(std::string const & header);
    void                        set_protocol(http_protocol_t protocol);
    void                        set_response_code(http_response_code_t code);
    void                        set_http_message(std::string const& message);
    void                        set_header(std::string const & name, std::string const & value);
    void                        set_response(std::string const & response);

private:
    typedef std::map<std::string, std::string>  header_t;

    std::string                 f_original_header = std::string();
    http_protocol_t             f_protocol = http_protocol_t::UNKNOWN;
    http_response_code_t        f_response_code = HTTP_RESPONSE_CODE_UNDEFINED;
    std::string                 f_http_message = std::string();
    header_t                    f_header = header_t();
    std::string                 f_response = std::string();
};



} // namespace edhttp
// vim: ts=4 sw=4 et
