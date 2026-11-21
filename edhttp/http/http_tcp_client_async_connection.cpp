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
#include    <edhttp/http/http_tcp_client_async_connection.h>

#include    <edhttp/exception.h>
#include    <edhttp/parametrized_value_list.h>


// snaplogger
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{
namespace detail
{



/** \class http_tcp_client_async_connection
 * \brief Connect to a TCP socket asynchronously.
 *
 * This class is used to allow for an asynchronous connection to a
 * server via the TCP protocol.
 *
 * At the moment, our base TCP connection is synchronous. Meaning that it
 * blocks until the connection either succeeded or failed. Here we want
 * to be able to return pretty much immediately so instead we create a
 * thread and manage that in order for the connection attempt itself to
 * work in the background.
 *
 * We still have a flow here: if the http_domain class decides that the
 * connection is taking too long, it will view the whole process as being
 * in error and a late connection gets dropped no matter what. However, if
 * that happens while the socket connect() function is running, the whole
 * process hangs anyway (not asynchronous...). Our implementation here
 * waits for all the TCP connections to be either successful or in a fail
 * state before destroying an http_domain object, but if quitting an
 * application, it may end up waiting for the failure to be reported.
 * The fix is to make the socket non-blocking immediately, before calling
 * the bind() and connect() functions.
 */



http_tcp_client_async_connection::http_tcp_client_async_connection(
          http_domain * parent
        , addr::addr const & address
        , std::string const & host
        , ed::mode_t mode
        , bool allow_http2)
    : timer(-1)
    , f_parent(parent)
    , f_communicator(ed::communicator::instance())
    , f_runner(this, address, host, mode, allow_http2)
    , f_thread("asynchronous connection handler thread", &f_runner)
{
std::cerr << "--- created runner/thread to connect to " << address << "\n";
    set_name("edhttp::detail::http_tcp_client_async_connection");
}


http_tcp_client_async_connection::~http_tcp_client_async_connection()
{
    
}


bool http_tcp_client_async_connection::start()
{
    // to avoid a potential race condition, we must add the thread done
    // signal first
    //
    f_thread_done = std::make_shared<thread_signal_handler>(this);
    f_communicator->add_connection(f_thread_done);

std::cerr << "--- start thread to connect to server\n";
    if(!f_thread.start())
    {
        f_communicator->remove_connection(f_thread_done);
        f_thread_done.reset();

        SNAP_LOG_ERROR
            << "The thread used to run the background connection process did not start."
            << SNAP_LOG_SEND;
        return false;
    }

    return true;
}


/** \brief Tell the main thread that the background thread is done.
 *
 * This function is called by the thread so the thread_done()
 * function of the thread done object gets called. Only the
 * thread should call this function.
 *
 * As a result the thread_done() function of this class will be
 * called by the main thread. This happens when the runner is done,
 * whether it successfully connected to a server or not.
 */
void http_tcp_client_async_connection::trigger_thread_done()
{
std::cerr << "--- call thread_done()?\n";
    f_thread_done->thread_done();
}


/** \brief Signal that the background thread is done.
 *
 * This callback is called whenever the background thread sends
 * a signal to us. This is used to avoid calling end user functions
 * that would certainly cause a lot of problem if called from the
 * thread.
 *
 * The function calls the process_connection_failed() if the
 * connection did not happen.
 *
 * The function calls the process_connected() if the connection
 * did happen.
 *
 * \note
 * This is used only if the user requested that the connection
 * happen in the background (i.e. use_thread was set to true
 * in the tcp_client_permanent_message_connection object
 * constructor.)
 */
void http_tcp_client_async_connection::thread_done()
{
std::cerr << "--- thread_done() called...\n";
    // if we used the thread we have to remove the signal used
    // to know that the thread was done
    //
    f_communicator->remove_connection(f_thread_done);
    f_thread_done.reset();

    // we will access the f_last_error member of the thread runner
    // which may not be available to the main thread yet, calling
    // stop forces a memory barrier so we are all good.
    //
    // calling stop() has no effect if we did not use the thread,
    // however, not calling stop() when we did use the thread
    // causes all sorts of other problems (especially, the thread
    // never gets joined)
    //
std::cerr << "--- stop() thread...\n";
    f_thread.stop();
std::cerr << "--- thread returned...\n";

    http_tcp_client_connection::pointer_t client(f_runner.release_client());

    // TODO: at the moment we do not handle the mark_done() signal
    //if(f_done)
    //{
    //    // already marked done, ignore the result and lose the
    //    // connection immediately
    //    //
    //    //f_thread_running.close(); -- not necessary, 'client' is the connection
    //    return;
    //}

    if(client == nullptr)
    {
        SNAP_LOG_ERROR
            << "connection to "
            << f_runner.get_address().to_ipv4or6_string(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT)
            << " failed with: "
            << f_runner.get_last_error()
            << SNAP_LOG_SEND;

        // signal that an error occurred
        //
        f_parent->process_connection_failed(shared_from_this());
    }
    else
    {
        f_client = client;

        // add the client to the communicator
        //
        f_communicator->add_connection(f_client);

        // let the client know we are now connected
        //
std::cerr << "--- calling process_connected() from http_tcp_client_async_connection.cpp\n";
        f_parent->process_connected(shared_from_this());
    }
}


bool http_tcp_client_async_connection::is_websocket() const
{
    return f_is_websocket;
}


bool http_tcp_client_async_connection::process_response(http_response::pointer_t response)
{
    std::string const alt_svc(response->get_header(g_name_edhttp_field_alt_svc));
    if(!alt_svc.empty())
    {
        parametrized_value_list ws;
        if(ws.parse(alt_svc))
        {
            // parsing worked, see if there are h2/h3 definitions
            //
            // for now, we completely ignore the "persist=1" possible
            // parameter since we are not monitoring the network
            // connection
            //
            for(int v(2); v <= 3; ++v)
            {
                std::string protocol("h");
                protocol += std::to_string(v);
                std::string const key(protocol + ':' + f_parent->get_host());
                parametrized_value const * p(ws.get_part(key));
                if(p != nullptr)
                {
                    std::int64_t ttl(86400);
                    if(p->has_parameter("ma"))
                    {
                        std::int64_t ma(-1);
                        if(p->get_parameter_as_integer("ma", ma)
                        && ma > 0
                        && ma < 31536000)
                        {
                            ttl = ma;
                        }
                    }
                    accepted_protocol(key, ttl);
                }
            }
        }
    }

    return  f_parent->process_response(response);
}


bool http_tcp_client_async_connection::process_next_request()
{
    return f_parent->process_next_request();
}


bool http_tcp_client_async_connection::is_h2() const
{
    return f_is_h2;
}


bool http_tcp_client_async_connection::send_request(http_request::pointer_t request)
{
    if(f_client == nullptr)
    {
        throw logic_error("http_tcp_client_async_connection::send_request() when client is not yet connected.");
    }

    return f_client->send_request(request);
}


std::size_t http_tcp_client_async_connection::get_maximum_header_size() const
{
    return f_parent->get_maximum_header_size();
}


void http_tcp_client_async_connection::process_hup()
{
    timer::process_hup();

    f_parent->process_hup();
}



} // namespace edhttp
} // namespace detail
// vim: ts=4 sw=4 et
