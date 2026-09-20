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
#include    "edhttp/http_sync_client.h"

//#include    "edhttp/exception.h"
//#include    "edhttp/names.h"
//#include    "edhttp/token.h"
//#include    "edhttp/uri.h"
//#include    "edhttp/version.h"


// snaplogger
//
#include    <snaplogger/message.h>


// eventdispatcher
//
#include    <eventdispatcher/exception.h>


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
namespace
{



typedef std::vector<char>       buffer_t;


class http_tcp_connection
    : public ed::tcp_client_buffer_connection
{
public:
    typedef std::shared_ptr<http_tcp_connection>        pointer_t;
    typedef std::map<sdtd::string, pointer_t>           map_t;

                        http_tcp_connection(
                              http_client_connection_impl * parent
                            , addr::addr const & address
                            , mode_t const mode
                            , tcp_bio_options const & opt);

    bool                is_h2() const;
    bool                is_websocket() const;

    // ed::tcp_client_connection implementation
    //
    virtual void                process_read() override;
    virtual void                process_hup() override;

private:
    void                        process_h2_read();

    http_client_connection_impl *
                        f_parent = nullptr;
    bool                f_is_h2 = false;
    bool                f_got_http1_header = false;
    bool                f_is_websocket = false;
    buffer_t            f_buffer = buffer_t();
    ssize_t             f_buffer_size = 0;
    ssize_t             f_buffer_offset = 0;
};


http_tcp_connection::http_tcp_connection(
          http_client_connection_impl * parent
        , addr::addr const & address
        , mode_t const mode
        , tcp_bio_options const & opt)
    : tcp_client_buffer_connection(address, mode, false, opt)
    , f_parent(parent)
    , f_buffer(16 * 1024)
{
    f_is_h2 = tcp_bio_client::get_selected_alpn() == "h2";
}


bool http_tcp_connection::is_h2() const
{
    return f_is_h2;
}


/** \brief An HTTP/1.1 socket was converted to a websocket.
 *
 * With HTTP/1.1, we can upgrade a standard HTTP/1.1 connection to
 * a websocket. Once that upgrade takes hold, we are in a websocket
 * mode and need to handle the frames ourselves. Also the socket
 * cannot be reused for additional HTTP requests such as a GET.
 *
 * \return true if the socket was used with HTTP/1.1 and was converted
 *         to a websocket (i.e. bi-directional stream).
 */
bool http_tcp_connection::is_websocket() const
{
    return f_is_websocket;
}


void http_tcp_connection::process_read()
{
    if(is_h2())
    {
        // if h2 we let the library do all the work in regard to what to do
        // about the data we receive and send
        //
        process_h2_read();
    }
    else
    {
        // with HTTP/1.1 we first want to read lines, once we find an empty
        // line, we found all the headers and can start reading the both
        //
        if(f_got_http1_header)
        {
        }
        else
        {
            tcp_client_buffer_connection::process_read();
        }
    }
}


void http_client_connection::process_h2_read()
{
    if(!valid_socket())
    {
        return;
    }

    // the nghttp2 library is a state machine that handles the HTTP/2 data
    // through callbacks -- here we just pass the data to the library
    // session we created earlier and handle the data when a callback is
    // called
    //
    int count_bytes(0);
    std::int64_t const date_limit(get_current_date() + get_processing_time_limit());
    for(;;)
    {
        errno = 0;
        if(f_buffer_size == 0)
        {
            f_buffer_size = read(&f_buffer[0], f_buffer.size());
        }
        if(f_buffer_size > 0)
        {
            while(f_buffer_offset < f_buffer_size)
            {
                ssize_t const processed_bytes(nghttp2_session_mem_recv(
                        session,
                        reinterpret_cast<std::uint8_t const *>(&f_buffer[0]) + f_buffer_offset,
                        r - offset));
                if(processed_bytes <= 0)
                {
                    if(processed_bytes < 0)
                    {
                        process_error();
                    }
                    return;
                }
                f_buffer_offset += processed_bytes;
                count_bytes += processed_bytes;

                if(count_bytes >= get_event_limit()
                || get_current_date() >= date_limit)
                {
                    // we reach one or both limits, stop processing so
                    // the other events have a chance to run
                    //
                    return;
                }
            }
        }
        else if(f_buffer_size == 0 || errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // no more data available at this time
            //
            return;
        }
        else //if(f_buffer_size < 0)
        {
            // TODO: do something about the error
            //
            int const e(errno);
            SNAP_LOG_ERROR
                << "an error occurred while reading from socket (errno: "
                << e
                << " -- "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }
    }
    snapdev::NOT_REACHED();
}


void http_client_connection::process_line(std::string const & line)
{
    // this only happens with HTTP/1.1
    //
    if(line.empty())
    {
        f_got_http1_header = true;

        // get remaining bytes from f_line
        //
        f_body += get_line();
    }
    else
    {
        f_header += line;
    }
}



class http_udp_connection
    : public ed::udp_client
{
public:

private:
};




class http_client_connection_impl
    : public ed::timer
{
public:
                                    http_client_connection_impl(http_client_connection * parent);

    bool                            get_keep_alive() const;
    void                            set_keep_alive(bool keep_alive);

private:
    http_client_connection *        f_parent = nullptr;
    bool                            f_keep_alive = true;

    http_tcp_connection::map_t      f_tcp_connections = http_tcp_connection::map_t();
};


http_client_connection_impl::http_client_connection_impl(http_client_connection * parent)
    : f_parent(parent)
{
}


bool http_client_connection_impl::get_keep_alive() const
{
    return f_keep_alive;
}


void http_client_connection_impl::set_keep_alive(bool keep_alive)
{
    f_keep_alive = keep_alive;
}


void http_client_connection::process_read()
{
}



} // no name namespace



http_client_connection::http_client_connection()
    : f_connection(std::make_shared<http_client_connection_impl>(this))
{
}


bool http_client_connection::get_keep_alive() const
{
    return f_connection->get_keep_alive();
}


void http_client_connection::set_keep_alive(bool keep_alive)
{
    f_connection->set_keep_alive(keep_alive);
}


/** \brief Send a request to an HTTP server.
 *
 * This functions sends an HTTP request to a server. The response will
 * be sent to you through a callback.
 *
 * The request can be sent using HTTP/1.1, HTTP/2, or HTTP/3.
 *
 * \param[in] request  The request to send to the HTTP server.
 */
void http_sync_client::send_request(http_request const & request)
{
    f_connection->send_request(request);
}


/** \brief This function gets called whenever we receive a response.
 *
 * Whenever you call the send_request() function, the system creates a
 * packet and sends it to the specified HTTP server. Once the server
 * sent a reply, it gets gathered inside an http_response object which
 * is then sent to your code through this function.
 *
 * To avoid reimplementing the http_client object, you can use the
 * add_response_callback() function instead.
 *
 * \warning
 * The default process_response() makes sure that the user added
 * callbacks (see add_response_callback()) do get called. Please
 * make sure that you call this function if you reimplement it in
 * your own connection class.
 *
 * \todo
 * Add a way to share a partial buffer for large replies.
 *
 * \param[in] r  The HTTP response.
 *
 * \sa add_response_callback()
 */
void http_client::process_response(http_response::pointer_t r)
{
    f_response_callbacks.call(r);
}


/** \brief Add a response callback.
 *
 * Instead of deriving and reimplementing the process_response() function,
 * you can instead use this function to add a response callback. This
 * function gets called any time you receive a response.
 *
 * Any number of response callbacks can be added.
 *
 * If you keep note of the callback identifier (returned by the function,)
 * then you can remove that callback later.
 *
 * \param[in] f  The function to add.
 *
 * \return An identifier which can be used to remove the callback later.
 */
snapdev::callback_manager::callback_id_t http_client::add_response_callback(response_callback_t f)
{
    return f_response_callbacks.add_callback(f);
}


/** \brief Remove a response callback.
 *
 * You may add any number of response callback using the add_response_callback()
 * function. That function returns an identifier that can be used to later
 * remove that callback.
 *
 * \param[in] id  The identifier of the callback to remove.
 *
 * \return true if the callback was found and removed.
 *
 * \sa add_response_callback()
 */
bool http_client::remove_response_callback(snapdev::callback_manager::callback_id_t id)
{
    return f_response_callbacks.remove_callback(id);
}



} // namespace edhttp
// vim: ts=4 sw=4 et
