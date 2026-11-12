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
#include    "http/http_udp_connection.h"



// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{
namespace detail
{



http_udp_connection::http_udp_connection(http_client_connection_impl * parent, addr::addr const & address)
    : udp_server_connection(address)
    , f_parent(parent)
{
}


bool http_udp_connection::send_request(http_request::pointer_t request)
{
    header_t const & headers(request->get_headers());
    for(auto const & h : headers)
    {
        if(h.first == "todo")
        {
            break;
        }
    }

    return true;
}


bool http_udp_connection::is_h2() const
{
    return false;
}


void http_udp_connection::process_read()
{
}



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
