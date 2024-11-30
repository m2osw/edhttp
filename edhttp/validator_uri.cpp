// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Implementation of the URI validator.
 *
 * The URI validator allows us to check the input as an URI address.
 */

// self
//
#include    "edhttp/validator_uri.h"

#include    "edhttp/uri.h"


// cppthread
//
#include    <cppthread/log.h>


// last include
//
#include    <snapdev/poison.h>




namespace edhttp
{



namespace
{



class validator_uri_factory
    : public advgetopt::validator_factory
{
public:
    validator_uri_factory()
    {
        advgetopt::validator::register_validator(*this);
    }

    virtual std::string get_name() const override
    {
        return std::string("uri");
    }

    virtual std::shared_ptr<advgetopt::validator> create(advgetopt::string_list_t const & data) const override
    {
        return std::make_shared<validator_uri>(data);
    }
};

validator_uri_factory         g_validator_uri_factory;



} // no name namespace





validator_uri::validator_uri(advgetopt::string_list_t const & param_list)
{
    for(auto const & p : param_list)
    {
        if(p == "accept_ip" || p == "accept-ip")
        {
            f_accept_ip = true;
        }
        else if(p == "accept_path" || p == "accept-path")
        {
            f_accept_path = true;
        }
        else if(p == "refuse_ip" || p == "refuse-ip")
        {
            f_accept_ip = false;
        }
        else if(p == "refuse_path" || p == "refuse-path")
        {
            f_accept_path = false;
        }
        else
        {
            cppthread::log << cppthread::log_level_t::error
                           << "validator_uri(): unknown parameter \""
                           << p
                           << "\"."
                           << cppthread::end;
        }
    }
}


/** \brief Return the name of this validator.
 *
 * This function returns "uri".
 *
 * \return "uri".
 */
std::string validator_uri::name() const
{
    return std::string("uri");
}


/** \brief Check the value to make sure URIs are considered valid.
 *
 * This function is used to verify the value for a valid URI.
 *
 * \param[in] value  The value to be validated.
 *
 * \return true on a match.
 */
bool validator_uri::validate(std::string const & value) const
{
    uri u;
    return u.set_uri(value, f_accept_path, f_accept_ip);
}



} // namespace edhttp
// vim: ts=4 sw=4 et
