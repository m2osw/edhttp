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
//#include    <edhttp/compression/compressor.h>
//#include    <edhttp/utils.h>
#include    <edhttp/http_request.h>



// eventdispatcher
//
//#include    <eventdispatcher/tcp_client_connection.h>



namespace edhttp
{
namespace detail
{




class http_connection
{
public:
    virtual                         ~http_connection() = default;

    virtual bool                    send_request(http_request::pointer_t request) = 0;
    virtual bool                    is_h2() const = 0;
};



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
