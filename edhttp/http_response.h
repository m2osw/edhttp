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

// eventdispatcher
//
//#include    <eventdispatcher/tcp_bio_client.h>


// libaddr
//
//#include    <libaddr/addr_range.h>


// C++
//
#include    <map>
#include    <memory>
#include    <string>



namespace edhttp
{



class http_response
{
public:
    typedef std::shared_ptr<http_response>      pointer_t;

    enum class protocol_t
    {
        UNKNOWN,
        HTTP_1_0,
        HTTP_1_1
    };

    std::string                 get_original_header() const;
    protocol_t                  get_protocol() const;
    int                         get_response_code() const;
    std::string                 get_http_message() const;
    bool                        has_header(std::string const & name) const;
    std::string                 get_header(std::string const & name) const;
    std::string                 get_response() const;

    void                        append_original_header(std::string const & header);
    void                        set_protocol(protocol_t protocol);
    void                        set_response_code(int code);
    void                        set_http_message(std::string const& message);
    void                        set_header(std::string const & name, std::string const & value);
    void                        set_response(std::string const & response);

private:
    typedef std::map<std::string, std::string>  header_t;

    std::string                 f_original_header = std::string();
    protocol_t                  f_protocol = protocol_t::UNKNOWN;
    int32_t                     f_response_code = 0;
    std::string                 f_http_message = std::string();
    header_t                    f_header = header_t();
    std::string                 f_response = std::string();
};



} // namespace edhttp
// vim: ts=4 sw=4 et
