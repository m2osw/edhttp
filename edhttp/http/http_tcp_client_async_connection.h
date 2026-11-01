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
#include    "http/http_domain.h"

#include    "http/runner.h"
#include    "http/thread_signal_handler.h"


// cppthread
//
#include    <cppthread/thread.h>


// C++
//
//#include    <map>



namespace edhttp
{
namespace detail
{



class http_tcp_client_async_connection
    : public ed::timer
{
public:
    typedef std::shared_ptr<http_tcp_client_async_connection>       pointer_t;
    typedef std::map<std::string, pointer_t>                        map_t;

                                    http_tcp_client_async_connection(
                                          http_domain * parent
                                        , addr::addr const & address
                                        , std::string const & host
                                        , ed::mode_t mode);
                                    http_tcp_client_async_connection(http_tcp_client_async_connection const &) = delete;
    virtual                         ~http_tcp_client_async_connection() override;

    http_tcp_client_async_connection &
                                    operator = (http_tcp_client_async_connection const &) = delete;

    bool                            start();
    void                            trigger_thread_done();
    bool                            is_h2() const;
    bool                            is_websocket() const;
    void                            send_request(http_request::pointer_t request);
    std::size_t                     get_maximum_header_size() const;
    void                            process_response(http_response const & response);

    // connection implementation
    //
    virtual bool                    is_writer() const override;
    virtual void                    process_read() override;
    virtual void                    process_hup() override;
    virtual void                    process_write() override;

private:
    enum class h1_state_t : std::uint8_t
    {
        H1_STATE_READY,         // a request can be sent
        H1_STATE_WAITING,       // waiting for a reply
        H1_STATE_PROTOCOL,      // 1st line returned by server
        H1_STATE_HEADER,        // lines up to empty line
        H1_STATE_BODY,          // the body data
    };

    void                            process_h1_buffer();
    void                            process_h2_buffer();

    http_domain *                   f_parent = nullptr;
    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    runner                          f_runner;
    cppthread::thread               f_thread;
    thread_signal_handler::pointer_t
                                    f_thread_done = thread_signal_handler::pointer_t();

    bool                            f_is_h2 = false;
    bool                            f_got_http1_header = false;
    bool                            f_is_websocket = false;
    h1_state_t                      f_h1_state = h1_state_t::H1_STATE_READY;
    buffer_t                        f_input = buffer_t();
    ssize_t                         f_input_position = 0;
    ssize_t                         f_input_size = 0;
    buffer_t                        f_output = buffer_t();
    std::size_t                     f_output_position = 0;
    std::string                     f_line = std::string();
    std::size_t                     f_header_size = 0;
};



} // namespace edhttp
} // namespace detail
// vim: ts=4 sw=4 et
