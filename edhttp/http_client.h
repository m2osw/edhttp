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



class http_client_connection_impl;


class http_client_connection
    : public ed::timer
{
public:
    typedef std::shared_ptr<http_cient_connection>          pointer_t;
    typedef std::function<void(http_response::pointer_t)>   response_callback_t;

                                    http_client_connection();

                                    http_client_connection(http_client_connection const &) = delete;
    http_client_connection &        operator = (http_client_connection const &) = delete;

    bool                            get_keep_alive() const;
    void                            set_keep_alive(bool keep_alive);

    void                            send_request(http_request const & request);

    // new callback
    //
    virtual void                    process_response(http_response::pointer_t r);

    snapdev::callback_manager::callback_id_t
                                    add_response_callback(response_callback_t f);
    bool                            remove_response_callback(snapdev::callback_manager::callback_id_t id);

private:
    void                            read_response(http_response::pointer_t connection);

    std::shared_ptr<http_client_connection_impl>
                                    f_connection = std::shared_ptr<http_client_connection_impl>();
    response_callback_t             f_response_callbacks = response_callback_t();
};



} // namespace edhttp
// vim: ts=4 sw=4 et
