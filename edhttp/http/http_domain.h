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
#include    <edhttp/http_client_connection.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/timer.h>



namespace edhttp
{
namespace detail
{



class http_client_connection_impl;


enum class http_domain_status_t
{
    HTTP_DOMAIN_STATUS_CONNECTING,
    HTTP_DOMAIN_STATUS_CONNECTED,
    HTTP_DOMAIN_STATUS_DISCONNECTED,
};


class http_domain
    : public ed::timer
{
public:
    typedef std::shared_ptr<http_domain>        pointer_t;
    typedef std::map<std::string, pointer_t>    map_t;

                                    http_domain(
                                          http_client_connection_impl * parent
                                        , std::string const & host
                                        , addr::addr::vector_t const & addresses
                                        , ed::mode_t mode
                                        , allowed_http_protocol_t allowed);
                                    http_domain(http_domain const &) = delete;
    virtual                         ~http_domain() override;

    http_domain &                   operator = (http_domain const &) = delete;

    http_domain_status_t            get_status() const;
    std::string const &             get_host() const;
    bool                            send_request(http_request::pointer_t request);
    bool                            process_next_request();
    std::size_t                     get_maximum_header_size() const;
    void                            process_connected(ed::connection::pointer_t c);
    void                            process_connection_failed(ed::connection::pointer_t c);
    bool                            process_response(http_response::pointer_t response);

    // connection implementation
    //
    virtual void                    process_timeout() override;
    virtual void                    process_error() override;
    virtual void                    process_hup() override;

private:
    struct attempt_t
    {
        typedef std::vector<attempt_t>  vector_t;

        addr::addr                      f_address = addr::addr();
        ed::connection::pointer_t       f_connection = ed::connection::pointer_t();
        bool                            f_http3 = false;
        bool                            f_allow_http2 = false;
    };

    void                            close_attempts();
    void                            close();
    void                            start_attempt();
    bool                            apply_send_request(http_request::pointer_t request);

    http_client_connection_impl *   f_parent = nullptr;
    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    std::string                     f_host = std::string();
    ed::mode_t                      f_mode = ed::mode_t::MODE_PLAIN;
    http_request::list_t            f_request_list = http_request::list_t();
    attempt_t::vector_t             f_attempts = attempt_t::vector_t();
    connection_count_t              f_current_attempt = 0;
    ed::connection::pointer_t       f_child_connection = ed::connection::pointer_t();
};



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
