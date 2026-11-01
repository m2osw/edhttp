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
#include    <edhttp/http_cookie.h>

#include    <edhttp/token.h>


// snapdev
//
#include    <snapdev/callback_manager.h>


// C++
//
#include    <map>
#include    <memory>
#include    <string>



namespace edhttp
{



enum class http_protocol_t
{
    HTTP_PROTOCOL_UNKNOWN,

    HTTP_PROTOCOL_1_0,
    HTTP_PROTOCOL_1_1,
    HTTP_PROTOCOL_2,
    HTTP_PROTOCOL_3,
};


typedef std::uint16_t           http_response_code_t; // 1XX to 5XX or 0 when undefined

constexpr http_response_code_t const    HTTP_RESPONSE_CODE_UNDEFINED = 0;


class http_response
{
public:
    typedef std::shared_ptr<http_response>                          pointer_t;
    typedef std::function<void(http_response::pointer_t response)>  callback_t;
    typedef snapdev::callback_manager<callback_t>                   callback_manager_t;

    std::string                 get_original_header() const;
    http_protocol_t             get_protocol() const;
    http_response_code_t        get_response_code() const;
    std::string                 get_reason_phrase() const;
    bool                        has_header(token const & name) const;
    std::string                 get_header(token const & name) const;
    http_cookie const &         get_cookie(std::string const & name) const;
    http_cookie::vector_t const &
                                get_cookies() const;
    std::string const &         get_response() const;

    void                        append_original_header(std::string const & header);
    void                        set_protocol(http_protocol_t protocol);
    void                        set_response_code(http_response_code_t code);
    void                        set_reason_phrase(std::string const& message);
    void                        add_header(token const & name, std::string const & value);
    void                        add_cookie(http_cookie const & cookie);
    void                        set_response(std::string const & response);
    void                        append_response(std::string const & chunk);
    void                        reserve_response(std::size_t size);

    bool                        parse_status_line(std::string const & status_line);
    bool                        parse_header(std::string const & header);

private:
    typedef std::map<token, std::string>  header_t;

    std::string                 f_original_header = std::string();
    http_protocol_t             f_protocol = http_protocol_t::HTTP_PROTOCOL_UNKNOWN;
    http_response_code_t        f_response_code = HTTP_RESPONSE_CODE_UNDEFINED;
    std::string                 f_reason_phrase = std::string();
    header_t                    f_header = header_t();
    http_cookie::vector_t       f_cookies = http_cookie::vector_t();
    std::string                 f_response = std::string();
};


char const *                    get_reason_phrase_by_code(http_response_code_t code);
std::string                     get_reason_phrase_by_code_message(http_response_code_t code);



} // namespace edhttp
// vim: ts=4 sw=4 et
