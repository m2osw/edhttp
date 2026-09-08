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
#include    <edhttp/http_request.h>
#include    <edhttp/http_response.h>


// eventdispatcher
//
#include    <eventdispatcher/tcp_bio_client.h>


// libaddr
//
#include    <libaddr/addr_range.h>



namespace edhttp
{



class http_sync_client
{
public:
                                    http_sync_client();

                                    http_sync_client(http_sync_client const &) = delete;
    http_sync_client &              operator = (http_sync_client const &) = delete;

    bool                            get_keep_alive() const;
    void                            set_keep_alive(bool keep_alive);

    http_response::pointer_t        send_request(http_request const & request);

private:
    void                            read_response(http_response::pointer_t connection);

    bool                            f_keep_alive = true;
    ed::tcp_bio_client::pointer_t   f_connection = ed::tcp_bio_client::pointer_t(); // TODO: convert to an impl so we can support TCP/UDP for HTTP/1.1, HTTP/2 & HTTP/3
    std::string                     f_host = std::string();
    std::int32_t                    f_port = -1;
};



} // namespace edhttp
// vim: ts=4 sw=4 et
