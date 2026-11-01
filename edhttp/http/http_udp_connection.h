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
#include    <eventdispatcher/udp_server_connection.h>



namespace edhttp
{
namespace detail
{



class http_client_connection_impl;


class http_udp_connection
    : public ed::udp_server_connection
{
public:
    typedef std::shared_ptr<http_udp_connection>        pointer_t;

                                    http_udp_connection(http_client_connection_impl * parent, addr::addr const & address);
                                    http_udp_connection(http_udp_connection const & rhs) = delete;

    http_udp_connection &           operator = (http_udp_connection const & rhs) = delete;

    // connection implementation
    //
    virtual void                    process_read() override;

private:
    http_client_connection_impl *   f_parent = nullptr;
};



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
