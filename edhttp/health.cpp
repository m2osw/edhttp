// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    "edhttp/health.h"

#include    "edhttp/http_client_server.h"


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/tcp_server_connection.h>
#include    <eventdispatcher/tcp_server_client_message_connection.h>


// snaplogger
//
#include    <snaplogger/map_diagnostic.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{


namespace
{



/** \brief The command line options that the health object supports.
 *
 * This variable holds the list of options that the health object can be
 * given. If not specified, then no health object is created (there is
 * not default). [TBD: we could let the programmer set defaults.]
 *
 * The options are:
 *
 * * `--health-certificate` -- the certificate (optional)
 * * `--health-listen` -- the IP address to listen
 * * `--health-private-key` -- the private key (optional)
 *
 * Note that the `--health-listen` option is also optional. If not specified,
 * then no health server is created and other processes will not have access
 * to the health of the service.
 */
advgetopt::option const g_options[] =
{
    // HEALTH LISTEN
    //
    advgetopt::define_option(
          advgetopt::Name("health-listen")
        , advgetopt::Flags(advgetopt::all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                    , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("the IP and port to listen on for health messages.")
    ),
    advgetopt::define_option(
          advgetopt::Name("health-certificate")
        , advgetopt::Flags(advgetopt::all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                    , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("certificate for --health-listen connection.")
    ),
    advgetopt::define_option(
          advgetopt::Name("health-private-key")
        , advgetopt::Flags(advgetopt::all_flags<
                      advgetopt::GETOPT_FLAG_GROUP_OPTIONS
                    , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("private key for --health-listen connection.")
    ),

    // END
    //
    advgetopt::end_options()
};



// TODO: this needs to be a the HTTP server (which we do not quite have yet)
class health_server_connection
    : public ed::tcp_server_connection
{
public:
    typedef std::shared_ptr<health_server_connection>      pointer_t;

                        health_server_connection(
                                      addr::addr const & addr
                                    , std::string const & certificate
                                    , std::string const & private_key
                                    , ed::mode_t mode = ed::mode_t::MODE_PLAIN
                                    , int max_connections = -1
                                    , bool reuse_addr = false);

    void                set_status(std::string const & status);
    std::string         get_status() const;

    // tcp_server_connection implementation
    virtual void        process_accept();

private:
    std::string         f_status = std::string();
};


class health_client_connection
    : public ed::tcp_server_client_message_connection
{
public:
                                health_client_connection(
                                      health_server_connection * server
                                    , ed::tcp_bio_client::pointer_t client);

                                health_client_connection(health_client_connection const &) = delete;
    health_client_connection &  operator = (health_client_connection const &) = delete;

private:
    health_server_connection *  f_server = nullptr;
};


health_server_connection::health_server_connection(
          addr::addr const & addr
        , std::string const & certificate
        , std::string const & private_key
        , ed::mode_t mode
        , int max_connections
        , bool reuse_addr)
    : tcp_server_connection(
          addr
        , certificate
        , private_key
        , mode
        , max_connections
        , reuse_addr)
{
    // do a set_status() so that way we get the set_diagnostic() for "free"
    //
    set_status(HEALTH_STARTING);
}


void health_server_connection::set_status(std::string const & status)
{
    f_status = status;

    snaplogger::set_diagnostic(DIAG_KEY_HEALTH, status);
}


std::string health_server_connection::get_status() const
{
    return f_status;
}


void health_server_connection::process_accept()
{
    ed::tcp_bio_client::pointer_t const new_client(accept());
    if(new_client == nullptr)
    {
        // an error occurred, report in the logs
        int const e(errno);
        SNAP_LOG_ERROR
            << "somehow accept() failed with errno: "
            << e
            << " -- "
            << strerror(e)
            << SNAP_LOG_SEND;
        return;
    }

    health_client_connection::pointer_t client(std::make_shared<health_client_connection>(this, new_client));
    if(!ed::communicator::instance()->add_connection(client))
    {
        SNAP_LOG_ERROR
            << "adding a health client connection to the list of connections failed."
            << SNAP_LOG_SEND;
        return;
    }
}





health_client_connection::health_client_connection(
          health_server_connection * server
        , ed::tcp_bio_client::pointer_t client)
    : tcp_server_client_message_connection(client)
    , f_server(server)
{
}



health_server_connection::pointer_t            g_health_connection;



} // no name namespace


void add_health_options(advgetopt::getopt & opts)
{
    opts.parse_options_info(g_options, true);
}


bool process_health_options(advgetopt::getopt & opts)
{
    if(!opts.is_defined("health-listen"))
    {
        return true;
    }

    std::string const address(opts.get_string("health-listen"));

    std::string certificate;
    std::string private_key;

    ed::mode_t mode(ed::mode_t::MODE_PLAIN);
    if(opts.is_defined("health-certificate")
    && opts.is_defined("health-private-key"))
    {
        certificate = opts.get_string("health-certificate");
        private_key = opts.get_string("health-private-key");

        if(certificate.empty()
        || private_key.empty())
        {
            SNAP_LOG_ERROR
                << "--health-certificate and --health-private-key must both be defined."
                << SNAP_LOG_SEND;
            return false;
        }

        mode = ed::mode_t::MODE_ALWAYS_SECURE;
    }

    addr::addr_parser p;
    p.set_protocol(IPPROTO_TCP);
    addr::addr_range::vector_t const vec(p.parse(address));
    if(p.has_errors())
    {
        SNAP_LOG_ERROR
            << "--health-listen was passed an invalid address ("
            << address
            << "): "
            << p.error_messages()
            << SNAP_LOG_SEND;
        return false;
    }

    if(vec.size() != 1
    || !vec[0].has_from()
    || vec[0].has_to())
    {
        SNAP_LOG_ERROR
            << "--health-listen was somehow passed multiple addresses ("
            << vec
            << "): "
            << p.error_messages()
            << SNAP_LOG_SEND;
        return false;
    }

    g_health_connection = std::make_shared<health_server_connection>(
              vec[0].get_from()
            , certificate
            , private_key
            , mode
            , 5             // max. connections
            , true);        // reuse_addr

    if(!ed::communicator::instance()->add_connection(g_health_connection))
    {
        SNAP_LOG_ERROR
            << "adding the health connection to the list of connections failed."
            << SNAP_LOG_SEND;
        return false;
    }

    return true;
}


void set_status(std::string const & status)
{
    if(g_health_connection != nullptr)
    {
        g_health_connection->set_status(status);
    }
}


std::string get_status()
{
    if(g_health_connection != nullptr)
    {
        return g_health_connection->get_status();
    }

    return std::string();
}



} // namespace edhttp
// vim: ts=4 sw=4 et
