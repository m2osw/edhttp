// Copyright (c) 2011-2025  Made to Order Software Corp.  All Rights Reserved
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
#include    "edhttp/http_link.h"

#include    "edhttp/exception.h"
#include    "edhttp/uri.h"


// C
//
//#include    <sys/time.h>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



/** \brief Initializes the link.
 *
 * This function initializes the link.
 *
 * \param[in] link  The URI for this link.
 * \param[in] rel  What the link represents (relative).
 *
 * \sa add_param()
 * \sa has_param()
 * \sa get_param()
 * \sa get_params()
 * \sa to_http_header()
 */
http_link::http_link(std::string const & link, std::string const & rel)
    : f_link(link)
    , f_rel(rel)
{
    if(f_link.empty())
    {
        throw link_parse_exception("the URI of a link cannot be empty.");
    }
    uri u;
    if(!u.set_uri(f_link))
    {
        // the uri class verifies the TLD with libtld
        //
        throw link_parse_exception("link URI is not valid.");
    }
}


/** \brief Retrieve the link "name".
 *
 * This function returns the relation string for this link. This is most
 * often viewed as the link name. In generatl, links are saved by name.
 * As a result, you cannot have two links with the same name.
 *
 * \return The "rel" string as passed to the constructor.
 */
std::string http_link::get_name() const
{
    return f_rel;
}


/** \brief Set whether to include this link on a redirect or not.
 *
 * Whenever Snap! generates a 301 or a 302, links do not get added to
 * the header (there are generally useless there in that situation.)
 *
 * By calling this function with true you indicate that it should be
 * added whether the process is about to redirect the client to another
 * page or not.
 *
 * See the shortcut implementation for an example where this is used.
 *
 * \param[in] redirect  Whether to include the link on a redirect.
 */
void http_link::set_redirect(bool redirect)
{
    f_redirect = redirect;
}


/** \brief Check whether to add this link on a redirect.
 *
 * By default links do not get added to the header if the request
 * ends up in a redirect.
 *
 * If this function returns true, then it will be added to the header.
 *
 * \return true if link is expected to be added on normal or redirect calls.
 */
bool http_link::get_redirect() const
{
    return f_redirect;
}


/** \brief Add a parameter to this link.
 *
 * Each link accept any number of parameter, although usually it will be
 * limited to just 2 or 3. The "rel" parameter is defined on construction
 * and cannot be re-added or modified with this function.
 *
 * The name of the parameter cannot be the empty string.
 *
 * If that parameter already exists, its value gets replaced.
 *
 * \param[in] name  The parameter name.
 * \param[in] value  The value for the named parameter.
 */
void http_link::add_param(std::string const & name, std::string const & value)
{
    if(name.empty())
    {
        throw link_parameter_exception("the name of a link parameter cannot be empty.");
    }
    if(name == "rel")
    {
        throw link_parameter_exception("the \"rel\" link parameter cannot be modified, it is set on construction only.");
    }
    for(char const * s(name.c_str()); *s != '\0'; ++s)
    {
        if(*s < 'a'
        || *s > 'z')
        {
            // this is probably wrong, but right now that's all we need
            // extend as required
            throw link_parameter_exception("the name of a link parameter must be defined with lowercase letters only (a-z).");
        }
    }
    for(char const * s(value.c_str()); *s != '\0'; ++s)
    {
        if(*s == '"'
        || *s < ' ')
        {
            throw link_parameter_exception("the value of a link parameter cannot include a control character or a double quote (\").");
        }
    }

    f_params[name] = value;
}


/** \brief Check whether a named parameter exists.
 *
 * This function checks whether a parameter with the specified name exists
 * in the list of parameters.
 *
 * \param[in] name  The new value of the cookie.
 *
 * \return true if the named parameter is defined.
 */
bool http_link::has_param(std::string const & name) const
{
    return f_params.find(name) != f_params.end();
}


/** \brief Retrieve the value of a parameter.
 *
 * This function returns the value of the specified parameter. If the
 * parameter is not defined, the empty string is returned.
 *
 * \param[in] name  The name of the parameter to read.
 *
 * \sa add_param();
 */
std::string http_link::get_param(std::string const & name) const
{
    if(has_param(name))
    {
        auto const it(f_params.find(name));
        return it->second;
    }

    return std::string();
}


/** \brief Get the complete list of parameters.
 *
 * This function can be used to retrieve the entire list of parameters from
 * this link.
 *
 * \warning
 * By default the function returns a reference meaning that a call to
 * add_param() will change the map you are dealing with here.
 *
 * \return The list of parameters, a map.
 */
http_link::param_t const & http_link::get_params() const
{
    return f_params;
}


/** \brief Transform the link for the HTTP header.
 *
 * This function transforms the link so it works as an HTTP header.
 *
 * \warning
 * This generates that one link string. Not the actual header. The
 * header requires all the links to be added in one "Link: ..." entry.
 *
 * See https://tools.ietf.org/html/rfc5988
 *
 * \return A valid HTTP link header.
 */
std::string http_link::to_http_header() const
{
    // Note: the name was already checked for invalid characters
    std::string result;

    result += "<";
    result += f_link;
    result += ">; rel=";
    result += f_rel;

    for(auto p : f_params)
    {
        // Note: if we test the value of each parameter, then we could
        //       already know whether it needs quoting or not when we
        //       reach here
        //
        result += "; ";
        result += p.first;
        result += "=\"";
        result += p.second;
        result += "\"";
    }

    return result;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
