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
#include    <edhttp/compression/compressor.h>
#include    <edhttp/utils.h>
#include    <edhttp/http_request.h>
#include    <edhttp/http_response.h>



// eventdispatcher
//
#include    <eventdispatcher/tcp_client_connection.h>


// nghttp2
//
#include    <nghttp2/nghttp2.h>



namespace edhttp
{
namespace detail
{



class http_tcp_client_async_connection;


constexpr std::size_t const         DEFAULT_HTTP2_MAX_STREAMS = 100;


class http_tcp_client_connection
    : public ed::tcp_client_connection
{
public:
    typedef std::shared_ptr<http_tcp_client_connection>        pointer_t;

                                    http_tcp_client_connection(
                                          http_tcp_client_async_connection * parent
                                        , addr::addr const & address
                                        , ed::mode_t const mode
                                        , ed::tcp_bio_options const & opt);
                                    http_tcp_client_connection(http_tcp_client_connection const &) = delete;
    virtual                         ~http_tcp_client_connection() override;

    http_tcp_client_connection      operator = (http_tcp_client_connection const &) = delete;

    bool                            has_output() const;
    bool                            is_h2() const;
    bool                            is_websocket() const;
    bool                            is_writer() const;
    bool                            send_request(http_request::pointer_t request);

    // connection implementation
    //
    virtual void                    process_read() override;
    virtual void                    process_hup() override;
    virtual void                    process_write() override;

    // tcp_client_connection implementation
    //
    //virtual ssize_t                 read(void * buf, std::size_t length) override;
    virtual ssize_t                 write(void const * buf, std::size_t length) override;

    // internal
    //
    void                            process_h2_response(nghttp2_frame const * frame);
    void                            set_h2_header(
                                          nghttp2_frame const * frame
                                        , std::string const & name
                                        , std::string const & value);
    void                            close_h2_stream(
                                          std::int32_t stream_id
                                        , std::uint32_t error_code);

private:
    enum class h1_state_t : std::uint8_t
    {
        H1_STATE_CONNECTING,        // for when we support asynchronous connections
        H1_STATE_READY,             // a request can be sent
        H1_STATE_WAITING,           // waiting for a reply
        H1_STATE_STATUS_LINE,       // 1st line returned by server
        H1_STATE_HEADER,            // lines up to empty line
        H1_STATE_CHECK_HEADER,      // verify header before reading body
        H1_STATE_BODY,              // read body data
        H1_STATE_STREAM,            // read body data as a stream (until socket is closed)
        H1_STATE_STREAM_CLOSED,     // EOF detected on stream
        H1_STATE_CHUNK_HEADER,      // read hexadecimal bytes as a size of a chunk
        H1_STATE_CHUNK_DATA,        // read chunk data
        H1_STATE_CHUNK_NL,          // read "\r\n" after the data chunk
        H1_STATE_RESPONSE_READY,    // response can be processed by application
        H1_STATE_DONE,              // streamed the body, we cannot get more (socket should be closed)
    };

    void                            create_h2_session();
    void                            send_h2_request(http_request::pointer_t request, http_response::pointer_t response);
    ssize_t                         process_h2_buffer();

    void                            send_h1_request(http_request::pointer_t request, http_response::pointer_t response);
    ssize_t                         process_h1_buffer();
    bool                            process_h1_status_line(std::string const & status_line);
    bool                            process_h1_header(std::string const & header);

    http_tcp_client_async_connection *
                                    f_parent = nullptr;
    bool                            f_is_h2 = false;
    bool                            f_got_http1_header = false;
    bool                            f_got_streaming_body = false;
    bool                            f_is_websocket = false;
    h1_state_t                      f_h1_state = h1_state_t::H1_STATE_CONNECTING;

    // HTTP/1.x supports one request at a time so we can have the request
    // and results as fields
    //
    http_response::pointer_t        f_h1_response = http_response::pointer_t(); // the response has the request pointer
    std::size_t                     f_content_length = 0;
    compressor *                    f_compressor = nullptr;
    buffer_t                        f_data = buffer_t();

    // h2 session
    //
    std::size_t                     f_h2_max_streams = DEFAULT_HTTP2_MAX_STREAMS;
    nghttp2_session *               f_h2_session = nullptr;
    http_response::map_t            f_h2_responses = http_response::map_t();

    buffer_t                        f_input = buffer_t();
    ssize_t                         f_input_position = 0;
    ssize_t                         f_input_size = 0;
    buffer_t                        f_output = buffer_t();
    std::size_t                     f_output_position = 0;
    std::size_t                     f_maximum_header_size = 0;
    std::size_t                     f_header_size = 0;
    std::string                     f_line = std::string();
};



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
