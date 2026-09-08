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
#include    "edhttp/http_response.h"

//#include    "edhttp/exception.h"
//#include    "edhttp/names.h"
//#include    "edhttp/token.h"
//#include    "edhttp/uri.h"
//#include    "edhttp/version.h"


// snaplogger
//
//#include    <snaplogger/message.h>


// eventdispatcher
//
//#include    <eventdispatcher/exception.h>


// libaddr
//
//#include    <libaddr/addr_parser.h>


// snapdev
//
//#include    <snapdev/base64.h>
//#include    <snapdev/not_reached.h>


// C++
//
//#include    <algorithm>
//#include    <iostream>
//#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



std::string http_response::get_original_header() const
{
    return f_original_header;
}


http_response::protocol_t http_response::get_protocol() const
{
    return f_protocol;
}


int http_response::get_response_code() const
{
    return f_response_code;
}


std::string http_response::get_http_message() const
{
    return f_http_message;
}


bool http_response::has_header(std::string const & name) const
{
    return f_header.find(name) != f_header.end();
}


std::string http_response::get_header(std::string const & name) const
{
    return f_header.at(name);
}


std::string http_response::get_response() const
{
    return f_response;
}


void http_response::append_original_header(std::string const & header)
{
    f_original_header += header;
    f_original_header += "\r\n";
}


void http_response::set_protocol(protocol_t protocol)
{
    f_protocol = protocol;
}


void http_response::set_response_code(int code)
{
    f_response_code = code;
}


void http_response::set_http_message(std::string const & message)
{
    f_http_message = message;
}


void http_response::set_header(std::string const& name, std::string const & value)
{
    f_header[name] = value;
}


void http_response::set_response(std::string const & response)
{
    f_response = response;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
