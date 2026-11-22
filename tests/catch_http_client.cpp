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

/** \file
 * \brief Verify the http_client_connection and related classes.
 *
 * This file implements tests to verify that the HTTP class used to
 * connect to an HTTP server is working as expected.
 *
 * The test creates a separate server so the client can connect to
 * something.
 */

// edhttp
//
#include    <edhttp/http_client_connection.h>

#include    <edhttp/exception.h>


// self
//
#include    "catch_main.h"


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/tcp_server_client_connection.h>
#include    <eventdispatcher/tcp_server_connection.h>


// libutf8
//
#include    <libutf8/libutf8.h>


// snapdev
//
#include    <snapdev/hexadecimal_string.h>
#include    <snapdev/tokenize_string.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C
//
#include    <string.h>



namespace
{



class client_connection;

// assuming the ed::communicator does it's work properly, we do not need
// to create a thread to have a server; it's a bit weaker as a test because
// that means things are not happening 100% in parallel, but it should
// simplify the test for now
//
class server_connection
    : public ed::tcp_server_connection
{
public:
                        server_connection(addr::addr const & a);
    virtual             ~server_connection() override;

    virtual void        process_accept() override;

private:
    std::shared_ptr<client_connection>  f_client = std::shared_ptr<client_connection>();
};


// this is the client connection server side
//
class client_connection
    : public ed::tcp_server_client_connection
{
public:
    typedef std::shared_ptr<client_connection>      pointer_t;

                        client_connection(server_connection * parent, ed::tcp_bio_client::pointer_t client);
                        client_connection(client_connection const & rhs) = delete;
    virtual             ~client_connection() override = default;

    client_connection   operator = (client_connection const & rhs) = delete;

    virtual void        process_read() override;

private:
    server_connection * f_parent = nullptr;
};


class response_handler
{
public:
    typedef std::shared_ptr<response_handler>       pointer_t;

    void                set_server_connection(server_connection::pointer_t server);
    void                set_client_connection(edhttp::http_client_connection::pointer_t client);
    void                set_request(edhttp::http_request::pointer_t request);

    bool                verify_response(edhttp::http_response::pointer_t response);

private:
    server_connection::pointer_t                f_server = server_connection::pointer_t();
    edhttp::http_client_connection::pointer_t   f_client = edhttp::http_client_connection::pointer_t();
    edhttp::http_request::pointer_t             f_request = edhttp::http_request::pointer_t();
};



server_connection::server_connection(addr::addr const & a)
    : tcp_server_connection(
          a
        , std::string()
        , std::string()
        , ed::mode_t::MODE_PLAIN
        , -1
        , true)
{
    set_name("test_http_server");
    set_keepalive();
    set_close_on_exec();
}


server_connection::~server_connection()
{
std::cerr << "--- server connection releasing server::client\n";
    ed::communicator::instance()->remove_connection(f_client);
}


void server_connection::process_accept()
{
    ed::tcp_bio_client::pointer_t s(accept());
    if(s == nullptr)
    {
        throw std::runtime_error("somehow server could not capture client.");
    }

    client_connection::pointer_t client(std::make_shared<client_connection>(this, s));
    if(!ed::communicator::instance()->add_connection(client))
    {
        throw std::runtime_error("could not add client connection to communicator.");
    }

    f_client = client;
}




client_connection::client_connection(server_connection * parent, ed::tcp_bio_client::pointer_t client)
    : tcp_server_client_connection(client)
    , f_parent(parent)
{
    set_name("test_http_server_client");
}


void client_connection::process_read()
{
    int s(get_socket());
    if(s == -1)
    {
        throw std::runtime_error("process_read() called with a closed socket.");
    }

    char buf[4096];
    ssize_t r(read(buf, sizeof(buf)));
    if(r < 0)
    {
        throw std::runtime_error("process_read() could not read data from socket.");
    }

std::cerr << "--- read [" << libutf8::to_visible_ascii(std::string(buf, r)) << "] (" << r << " bytes)\n";

    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "content-length:88\r\n";
    snapdev::timespec_ex const date(snapdev::now());
    ss << "date:" << date.to_string("%a, %d %b %Y %H:%M:%S GMT") << "\r\n";
    ss << "set-cookie:user-id=secret;domain=localhost;path=/;secure;httponly\r\n";
    ss << "svc-alt:h2=\":4003\";ma=3600\r\n";
    ss << "\r\n";
    ss << "<html><head><title>It worked</title></head><body><p>It finally worked.</p></body></html>";
    std::string const response(ss.str());
    write(response.data(), response.length());
}


void response_handler::set_server_connection(server_connection::pointer_t server)
{
    f_server = server;
}


void response_handler::set_client_connection(edhttp::http_client_connection::pointer_t client)
{
    f_client = client;
}


void response_handler::set_request(edhttp::http_request::pointer_t request)
{
    f_request = request;
}


bool response_handler::verify_response(edhttp::http_response::pointer_t response)
{
    CATCH_REQUIRE(response->get_request() == f_request);

    // TODO: test the date properly... (we need to pass it so the server can send a specific date)
    //
    //CATCH_REQUIRE(response->get_original_header() == 
    //        "HTTP/1.1 200 OK\r\n"
    //        "content-length:88\r\n"
    //        "date:???\r\n"
    //        "\r\n");

    CATCH_REQUIRE(response->get_protocol() == edhttp::http_protocol_t::HTTP_PROTOCOL_1_1);
    CATCH_REQUIRE(response->get_response_code() == 200);
    CATCH_REQUIRE(response->get_reason_phrase() == "OK");
    CATCH_REQUIRE(response->has_header("Content-Length"));
    CATCH_REQUIRE(response->get_header("content-length") == "88");
    CATCH_REQUIRE(response->get_header("svc-alt") == "h2=\":4003\";ma=3600");

    edhttp::http_cookie const * cookie(response->get_cookie("user-id"));
    CATCH_REQUIRE(cookie != nullptr);
    CATCH_REQUIRE(cookie->get_name() == "user-id");
    CATCH_REQUIRE(cookie->get_value() == "secret");
    CATCH_REQUIRE(cookie->get_type() == edhttp::http_cookie_type_t::HTTP_COOKIE_TYPE_SESSION);
    CATCH_REQUIRE(cookie->get_same_site() == edhttp::http_same_site_t::HTTP_SAME_SITE_UNKNOWN);
    CATCH_REQUIRE(cookie->get_domain() == "localhost");
    CATCH_REQUIRE(cookie->get_path() == "/");
    CATCH_REQUIRE(cookie->get_expire() == -1); // no expire means -1 -- also meaning that's a session cookie which disappear once done
    CATCH_REQUIRE(cookie->get_secure());
    CATCH_REQUIRE(cookie->get_http_only());

    edhttp::buffer_t const & body(response->get_response());
    std::string const s(reinterpret_cast<char const *>(body.data()), body.size());
    CATCH_REQUIRE(s == "<html><head><title>It worked</title></head><body><p>It finally worked.</p></body></html>");
    CATCH_REQUIRE(s == response->get_response_as_string());

    // we're done for now
    //
std::cerr << "--- verification done!\n";
    ed::communicator::instance()->remove_connection(f_client);
    f_client.reset();
    ed::communicator::instance()->remove_connection(f_server);
    f_server.reset();

    return true;
}



} // no name namespace



CATCH_TEST_CASE("http_client", "[http][client]")
{
    CATCH_START_SECTION("http_client: connect as HTTP/1.1 and do a GET")
    {
        {
            addr::addr server_address(addr::localhost_ipv4());
            server_address.set_port(4001);
            server_connection::pointer_t server(std::make_shared<server_connection>(server_address));
            if(!ed::communicator::instance()->add_connection(server))
            {
                throw std::runtime_error("could not add server connection to communicator.");
            }

std::cerr << "--- creating the client...\n";
            edhttp::http_client_connection::pointer_t client(std::make_shared<edhttp::http_client_connection>());
std::cerr << "--- set client's name...\n";
            client->set_name("catch_http_client");

std::cerr << "--- verify client's defaults...\n";
            // check defaults
            //
            CATCH_REQUIRE(client->get_keep_alive());
            CATCH_REQUIRE(client->get_allowed_http_protocols() == edhttp::ALLOWED_HTTP_ALL);
            CATCH_REQUIRE(client->get_http_connection_timeout() == edhttp::DEFAULT_CONNECTION_TIMEOUT);
            CATCH_REQUIRE(client->get_http_idle_timeout() == snapdev::timespec_ex());
            CATCH_REQUIRE(client->get_happy_eyeballs_delay() == edhttp::DEFAULT_HAPPY_EYEBALLS_DELAY);
            CATCH_REQUIRE(client->get_maximum_connection_attempts() == edhttp::MAXIMUM_CONNECTION_ATTEMPTS_DEFAULT);
            CATCH_REQUIRE(client->get_maximum_header_size() == edhttp::MAXIMUM_HEADER_SIZE);

            // force HTTP/1.1 only for this test
            //
std::cerr << "--- set protocols...\n";
            client->set_allowed_http_protocols(edhttp::ALLOWED_HTTP_PROTOCOL_1_1);
            CATCH_REQUIRE(client->get_allowed_http_protocols() == edhttp::ALLOWED_HTTP_PROTOCOL_1_1);
            if(!ed::communicator::instance()->add_connection(client))
            {
                throw std::runtime_error("could not add client connection to communicator.");
            }

std::cerr << "--- create request...\n";
            edhttp::http_request::pointer_t request(std::make_shared<edhttp::http_request>());
            request->set_host("localhost");
            request->set_port(4001);
std::cerr << "--- set callback...\n";

            response_handler::pointer_t rh(std::make_shared<response_handler>());
            rh->set_server_connection(server);
            rh->set_client_connection(client);
            rh->set_request(request);

            request->add_response_callback(std::bind(&response_handler::verify_response, rh, std::placeholders::_1));
std::cerr << "--- send request...\n";
            client->send_request(request);
        }

std::cerr << "--- run() all of that\n";
        ed::communicator::instance()->run();
std::cerr << "--- done!?\n";
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("http_client_error", "[http][client][error]")
{
    CATCH_START_SECTION("http_client_error: various errors (TBD)")
    {
        CATCH_REQUIRE(true);
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
