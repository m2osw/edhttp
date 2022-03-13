// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


// self
//
#include    "edhttp/uri.h"



// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/hexadecimal_string.h>
#include    <snapdev/join_strings.h>
#include    <snapdev/not_used.h>
#include    <snapdev/tokenize_string.h>


// QtSerialization
//
//#include <QtSerialization/QSerializationComposite.h>
//#include <QtSerialization/QSerializationFieldBasicTypes.h>
//#include <QtSerialization/QSerializationFieldString.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// libtld
//
#include    <libtld/tld.h>


// C
//
#include    <netdb.h>
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>




namespace edhttp
{



/** \brief This function intializes a default Snap URI object.
 *
 * Initialize a default Snap URI object.
 *
 * By default, the protocol is set to HTTP and everything else is set to
 * empty. This also means the original URI is set to empty (and stays that
 * way unless you later call set_uri() with a valid URI.)
 *
 * \sa set_uri()
 * \sa set_protocol()
 * \sa set_domain()
 * \sa set_path()
 * \sa set_option()
 * \sa set_query_string()
 * \sa set_anchor()
 */
uri::uri()
{
}

/** \brief Set the URI to the specified string.
 *
 * This function sets the URI to the specified string. The parsing
 * is the same as in the set_uri() function.
 *
 * \todo
 * Should this function throw if the URI is considered invalid?
 *
 * \param[in] u  The URI to assign to this Snap URI object.
 *
 * \sa set_uri()
 */
uri::uri(std::string const & u)
{
    if(!set_uri(u))
    {
        // TBD: should we throw if set_uri() returns false?
        SNAP_LOG_ERROR
            << "URI \""
            << u
            << "\" is considered invalid."
            << SNAP_LOG_SEND;
    }
}

/** \brief Replace the URI of this Snap URI object.
 *
 * This function replaces the current Snap URI object information
 * with the specified \p str data.
 *
 * Before calling this function YOU must force a URI encoding if the
 * URI is not yet encoded.
 *
 * Anything wrong in the syntax and the function returns false. Wrong
 * means empty entries, invalid encoding sequence, etc.
 *
 * \param[in] str  The new URI to replace all the current data of this Snap URI object.
 *
 * \return false if the URI could not be parsed (in which case nothing's changed in the object); true otherwise
 */
bool uri::set_uri(std::string const & str)
{
    char const * u(str.c_str());

    // retrieve the protocol
    char const * s(u);
    while(*u != '\0' && *u != ':')
    {
        ++u;
    }
    if(u - s < 1 || *u == '\0' || u[1] != '/' || u[2] != '/')
    {
        // protocol is not followed by :// or is an empty string
        return false;
    }
    std::string uri_protocol(s, u - s);

    // skip the ://
    u += 3;

    // retrieve the sub-domains and domain parts
    // we may also discover a name, password, and port
    char const * colon1(nullptr);
    char const * colon2(nullptr);
    char const * at(nullptr);
    for(s = u; *u != '\0' && *u != '/'; ++u)
    {
        if(*u == ':')
        {
            if(colon1 == nullptr)
            {
                colon1 = u;
            }
            else
            {
                if(at != nullptr)
                {
                    if(colon2 != nullptr)
                    {
                        return false;
                    }
                    colon2 = u;
                }
                else
                {
                    return false;
                }
            }
        }
        if(*u == '@')
        {
            if(at != nullptr)
            {
                // we cannot have more than one @ character that wasn't escaped
                return false;
            }
            at = u;
        }
    }
    // without an at (@) colon1 indicates a port
    if(at == nullptr && colon1 != nullptr)
    {
        // colon2 is nullptr since otherwise we already returned with false
        colon2 = colon1;
        colon1 = nullptr;
    }

    std::string username;
    std::string password;
    std::string full_domain_name;
    int port(protocol_to_port(uri_protocol));

    // retrieve the data
    if(colon1 != nullptr)
    {
        // if(at == nullptr) -- missing '@'? this is not possible since we just
        //                   turned colon1 to colon2 if no '@' was defined
        username.insert(0, s, colon1 - s);
        s = colon1 + 1;
    }
    if(at != nullptr)
    {
        password.insert(0, s, at - s);
        s = at + 1;
    }
    if(colon2 != nullptr)
    {
        full_domain_name.insert(0, s, colon2 - s);
        char const * p(colon2 + 1);
        if(p == u)
        {
            // empty port entries are considered invalid
            return false;
        }
        port = 0;  // Reset port.
        for(; p < u; ++p)
        {
            char const d(*p);
            if(d < '0' || d > '9')
            {
                // ports only accept digits
                return false;
            }
            port = port * 10 + d - '0';
            if(port > 65535)
            {
                // port overflow
                return false;
            }
        }
    }
    else
    {
        full_domain_name.insert(0, s, u - s);
    }

    // verify that there is a domain
    if(full_domain_name.empty())
    {
        return false;
    }

    // force a username AND password or neither
    if(username.empty() ^ password.empty())
    {
        return false;
    }

    // break-up the domain in sub-domains, base domain, and TLD
    advgetopt::string_list_t sub_domain_names;
    std::string domain_name;
    std::string tld;
    if(!process_domain(full_domain_name, sub_domain_names, domain_name, tld))
    {
        return false;
    }

    // now we are ready to parse further (i.e. path)
    advgetopt::string_list_t uri_path;
    if(*u != '\0')
    {
        // skip the '/'
        //
        ++u;
        for(s = u; *u != '\0' && *u != '?' && *u != '#'; ++u)
        {
            if(*u == '/')
            {
                if(s != u)
                {
                    // decode right here since we just separate one segment
                    //
                    uri_path.push_back(urldecode(std::string(s, u - s)));
                }
                // skip the '/'
                //
                s = u + 1;
            }
        }
        if(s != u)
        {
            // last segment when it does not end with '/'
            //
            uri_path.push_back(urldecode(std::string(s, u - s)));
        }
    }

    uri_options_t query_strings;
    if(*u == '?')
    {
        // skip the '?' and then any (invalid?) introductory '&'
        do
        {
            ++u;
        }
        while(*u == '&');
        char const * e(nullptr);
        for(s = u;; ++u)
        {
            if(*u || *u == '&' || *u == '#')
            {
                if(e == nullptr)
                {
                    // special case when a parameter appears without value
                    // ...&name&...
                    e = u;
                }
                std::string name(s, e - s);
                if(name.empty())
                {
                    // this is a very special case!!!
                    // ...&=value&...
                    // so we use a "special" name, also even that name could be
                    // defined in the query string (with '%2A=value' although
                    // we do not decode the name)
                    //
                    name = "*";
                }

                // query strings are saved as options (name/value pairs)
                // although the value may not be defined at all (...&name&...)
                // query string names are case sensitive (as per 6.2.2.1 of RFC 3986)
                std::string value;
                if(e != u)
                {
                    // note that we reach here if there is an equal sign,
                    // the value may still be empty (i.e. u - e - 1 == 0 is
                    // possible)
                    //
                    value = std::string(e + 1, u - e - 1);
                }
                name = urldecode(name);
                if(query_strings.find(name) != query_strings.end())
                {
                    // two parameters with the same name, refused
                    //
                    // (this is not correct as far as URIs are concerned,
                    // the same parameter can appear any number of times,
                    // but in our world, we consider that useless and
                    // possibly dangerous)
                    //
                    return false;
                }
                query_strings[name] = urldecode(value);

                // skip all the & and then reset s and e
                while(*u == '&')
                {
                    ++u;
                }
                if(*u == '\0' || *u == '#')
                {
                    // reached the end of the query strings
                    break;
                }
                s = u;
                e = nullptr;
            }
            else if(e == nullptr && *u == '=')
            {
                e = u;
            }
        }
    }

    // finally check for an anchor
    // (note that browsers do not send us the anchor data, however, URIs
    // defined on the server side can very well include such.)
    //
    std::string uri_anchor;
    if(*u == '#')
    {
        ++u;

        // we need to decode the string so we add the whole string here
        //
        std::string p(u);
        p = urldecode(p);
        if(!p.empty() && p[0] == '!')
        {
            // what do we do here?!
            //
            // it seems to me that we should not get those here, but that
            // could be from someone who wrote the URL in their document.
            //
            u = p.c_str();
            for(s = u; *u != '\0'; ++u)
            {
                if(*u == '/')
                {
                    // encode right here since we have separate strings
                    //
                    if(s != u)
                    {
                        uri_path.push_back(urldecode(std::string(s, u - s)));
                    }
                    // skip the '/'
                    //
                    s = u + 1;
                }
            }
            if(s != u)
            {
                // last path that doesn't end with '/'
                //
                uri_path.push_back(urldecode(std::string(s, u - s)));
            }
        }
        else
        {
            uri_anchor = p;
        }
    }

    // the path may include some ".." which we want to eliminate
    // note that contrary to Unix we do not accept "/.." as an equivalent
    // to "/" and we do not verify that all the paths exist... (i.e.
    // if "c" does not exist under "/a/b" (folder /a/b/c), then it should
    // be an error to use "/a/b/c/.." since "/a/b/c" cannot be computed.)
    //
    int max_path(uri_path.size());
    for(int i(0); i < max_path; ++i)
    {
        if(uri_path[i] == "..")
        {
            if(i == 0 || max_path < 2)
            {
                // the path starts with a ".." or has too many ".."
                //
                return false;
            }

            // remove the ".." and previous path segment
            //
            uri_path.erase(uri_path.begin() + i - 1, uri_path.begin() + i + 1);
            --i;
            max_path -= 2;
        }
    }

    // totally unchanged URI, but only if it is considered valid
    //
    f_original = str;

    // now decode all the entries that may be encoded
    //
    f_protocol = uri_protocol;
    f_username = urldecode(username);
    f_password = urldecode(password);
    if(port != -1)
    {
        f_port = port;
    }
    f_domain = domain_name;
    f_top_level_domain = tld;
    f_sub_domains = sub_domain_names;
    f_path = uri_path;

    // options come from parsing the sub-domains, query strings and paths
    // and at this point we do not have that information...
    //
    f_options.clear();
    f_address_ranges.clear();

    f_query_strings = query_strings;
    f_anchor = uri_anchor;

    return true;
}


/** \brief Return the original URI used to define the Snap URI object.
 *
 * This function returns the original URI as defined when calling the
 * set_uri() or creating the Snap URI object with the uri() constructor
 * accepting a string.
 *
 * Note that it is possible to use the uri object without using the
 * set_uri() or a string in the constructor by calling the setters of
 * the different parts of a URI. This is actually how snap_child does it
 * because Apache does not give us one plane URI, instead we get pre
 * separated parts. Therefore the get_original_uri() is always empty when
 * called from that f_uri variable.
 *
 * Note that this URI may still include security issues, although if the
 * input was not considered valid (i.e. had a valid protocol, etc.) then
 * this function returns an empty string.
 *
 * \return A constant reference to the original Snap URI.
 */
std::string const & uri::get_original_uri() const
{
    return f_original;
}


/** \brief Return the current URI define in this Snap URI object.
 *
 * This function concatenate all the URI parts in a fully qualified URI
 * and returns the result.
 *
 * This function does NOT take the rules in account (since it does not
 * know anything about them.) So you may want to consider using the
 * uri_rules::process_uri() function instead.
 *
 * \note
 * The returned URI is already encoded as required by HTTP and such.
 *
 * \param[in] use_hash_bang  When this flag is set to true the URI is returned
 * as a hash bang (i.e. domain/path becomes domain/#!path).
 *
 * \return The URI represented by this Snap URI object.
 */
std::string uri::get_uri(bool use_hash_bang) const
{
    std::string result(f_protocol);

    result += "://";

    // username/password if defined
    if(!f_username.empty())
    {
        result += urlencode(f_username);
        if(!f_password.empty())
        {
            result += ':';
            result += urlencode(f_password);
        }
        result += '@';
    }

    // full domain
    // domains should rarely require encoding for special characters, however,
    // it often is for international domains that make use of UTF-8 characters
    // outside of the standard ASCII letters and those definitively require
    // URL encoding to work right.
    result += urlencode(full_domain());
    if(f_port != protocol_to_port(f_protocol))
    {
        result += std::to_string(f_port);
    }
    result += '/';

    // path if no hash bang
    //
    std::string const p(path());
    if(!use_hash_bang && p.length() > 0)
    {
        // avoid a double slash if possible
        //
        // XXX: should the path not have a leading slash?
        //      (as far as I know path() never return a path with a leading
        //      slash; but we would need a test to make sure of it)
        //
        if(p[0] == '/')
        {
            result += p.substr(1);
        }
        else
        {
            result += p;
        }
    }

    // query string
    std::string const q(query_string());
    if(!q.empty())
    {
        result += '?';
        result += q;
    }

    // anchor
    if(!f_anchor.empty())
    {
        if(use_hash_bang)
        {
            // hash bang and anchor are exclusive
            throw uri_exception_exclusive_parameters("you cannot use the hash bang (#!) and an anchor (#) in the same URI");
        }
        result += '#';
        result += urlencode(f_anchor, "!/~");
    }

    // path when using the hash bang but only if not empty
    if(use_hash_bang && !p.empty())
    {
        result += "#!/";
        result += p;
    }

    return result;
}


/** \brief Retrieve the URI of the website.
 *
 * This function returns the URI of the website, without any path,
 * query string options, anchor. The port is included only if it
 * does not correspond to the protocol and the \p include_port flag
 * is set to true.
 *
 * \param[in] include_port  Whether the port should be included.
 *
 * \return The domain name with the protocol and optionally the port.
 */
std::string uri::get_website_uri(bool include_port) const
{
    std::string result(f_protocol);

    result += "://";
    result += full_domain();

    // only include the port if the caller wants it and if it does not
    // match the default protocol port
    //
    if(include_port
    && protocol_to_port(f_protocol) != f_port)
    {
        result += ':';
        result += std::to_string(f_port);
    }

    result += '/';

    return result;
}


/** \brief Retrieve a part by name.
 *
 * This function allows you to retrieve a part by name.
 *
 * The supported parts are:
 *
 * \li anchor -- The anchor
 * \li domain -- The domain name
 * \li full-domain -- The full domain: with sub-domains, domain, and TLD
 * \li option -- The option number \p part
 * \li option-count -- The number of options
 * \li original -- The original URI or ""
 * \li password -- The password
 * \li path -- The folder name number \p part
 * \li path-count -- the number of paths
 * \li protocol -- The protocol
 * \li query-string -- The query string number \p part
 * \li query-string-count -- The number of query strings
 * \li sub-domain -- The sub-domain name number \p part
 * \li sub-domain-count -- The number of sub-domains
 * \li tld or top-level-domain -- the top-level domain name
 * \li uri -- the full URI as you want it in an href="..." attribute
 * \li username -- The username
 *
 * \param[in] name  The named part to retrieve.
 * \param[in] part  The part number when required (i.e. sub-domains)
 *
 * \return The data representing this part as a string.
 */
std::string uri::get_part(std::string const & name, int part) const
{
    if(name.empty())
    {
        // should this be an error?
        return "";
    }
    switch(name[0])
    {
    case 'a':
        if(name == "anchor")
        {
            return f_anchor;
        }
        break;

    case 'd':
        if(name == "domain")
        {
            return f_domain;
        }
        break;

    case 'f':
        if(name == "full-domain")
        {
            return full_domain();
        }
        break;

    case 'o':
        if(name == "option")
        {
            if(static_cast<std::size_t>(part) >= f_options.size())
            {
                throw edhttp_uri_exception_out_of_bounds(
                      "option "
                    + std::to_string(part)
                    + " does not exist (range is 0 to "
                    + std::to_string(f_options.size())
                    + ")");
            }
            auto it(f_options.begin());
            std::advance(it, part);
            return it->second;
        }
        if(name == "option-count")
        {
            return std::to_string(f_options.size());
        }
        if(name == "original")
        {
            return f_original;
        }
        break;

    case 'p':
        if(name == "password")
        {
            return f_password;
        }
        if(name == "path")
        {
            if(static_cast<std::size_t>(part) >= f_path.size())
            {
                throw edhttp_uri_exception_out_of_bounds(
                      "path "
                    + std::to_string(part)
                    + " is not available (range 0 to "
                    + std::to_string(f_path.size())
                    + ")");
            }
            return f_path[part];
        }
        if(name == "path-count")
        {
            return std::to_string(f_path.size());
        }
        if(name == "port")
        {
            return std::to_string(f_port);
        }
        if(name == "protocol")
        {
            return f_protocol;
        }
        break;

    case 'q':
        if(name == "query-string")
        {
            if(static_cast<std::size_t>(part) >= f_query_strings.size())
            {
                throw edhttp_uri_exception_out_of_bounds(
                      "query-string "
                    + std::to_string(part)
                    + " does not exist (range 0 to "
                    + std::to_string(f_query_strings.size())
                    + ")");
            }
            auto it(f_query_strings.begin());
            std::advance(it, part);
            return it->second;
        }
        if(name == "query-string-count")
        {
            return std::to_string(f_query_strings.size());
        }
        break;

    case 's':
        if(name == "sub-domain")
        {
            if(static_cast<std::size_t>(part) >= f_sub_domains.size())
            {
                throw edhttp_uri_exception_out_of_bounds(
                      "sub-domain "
                    + std::to_string(part)
                    + " does not exist (range 0 to "
                    + std::to_string(f_sub_domains.size())
                    + ")");
            }
            return f_sub_domains[part];
        }
        if(name == "sub-domain-count")
        {
            return std::to_string(f_sub_domains.size());
        }
        break;

    case 't':
        if(name == "tld" || name == "top-level-domain")
        {
            return f_top_level_domain;
        }
        break;

    case 'u':
        if(name == "uri")
        {
            return get_uri();
        }
        if(name == "username")
        {
            return f_username;
        }
        break;

    default:
        // no match for other characters
        break;

    }

    return "";
}


/** \brief Change the protocol.
 *
 * This function is called to set the protocol.
 *
 * The protocol is not checked since this can be used for any
 * URI, not just the HTTP and HTTPS protocols. The name is
 * expected to be all lowercase and lowercase letters [a-z].
 *
 * \param[in] uri_protocol  The name of the protocol.
 */
void uri::set_protocol(std::string const & uri_protocol)
{
    if(uri_protocol.empty())
    {
        throw uri_exception_invalid_parameter("the uri_protocol parameter cannot be an empty string");
    }
    f_protocol = uri_protocol;
}


/** \brief Retrieve a copy of the protocol.
 *
 * This value is the name that defines how messages are being
 * sent between the client and the server.
 *
 * The main interface only accepts "http" and "https", but the
 * uri object accepts all protocols so one can write URIs
 * with protocols such as "ftp", "mail", and "gopher".
 *
 * \return A constant reference to the protocol of this URI.
 */
std::string const & uri::protocol() const
{
    return f_protocol;
}


/** \brief Process a domain name and break it up.
 *
 * This function processes a domain name and breaks it up in
 * the domain name, the sub-domains, and the TLD.
 *
 * \note
 * If the function returns false, then the out parameters may not
 * all be defined properly. None of them should be used in that
 * case anyway.
 *
 * \param[in] full_domain_name  The complete domain with sub-domains and TLD.
 * \param[out] sub_domain_names  An array of sub-domains, may be empty.
 * \param[out] domain_name  The domain by itself (no TLD and no sub-domain.)
 * \param[out] tld  The TLD part by itself.
 *
 * \return true if the function succeeds, false otherwise
 */
bool uri::process_domain(
      std::string const & full_domain_name
    , advgetopt::string_list_t & sub_domain_names
    , std::string & domain_name
    , std::string & tld)
{
    // first we need to determine the TLD, we use the tld()
    // function from the libtld library for this purpose

    // (note that the URI is expected to be encoded so the UTF-8
    // encoding is the same as ASCII)
    struct tld_info info;
    char const *fd(full_domain_name.c_str());
    tld_result r(::tld(fd, &info));
    if(r != TLD_RESULT_SUCCESS)
    {
        // (should we accept TLD_RESULT_INVALID URIs?)
        // the URI doesn't end with a known TLD
        return false;
    }

    // got the TLD, save it in the user's supplied variable
    tld = urldecode(info.f_tld);

    // search where the domain name starts
    char const *compute_domain_name(fd + info.f_offset);
    while(compute_domain_name > fd)
    {
        --compute_domain_name;
        if(*compute_domain_name == '.')
        {
            ++compute_domain_name;
            break;
        }
    }
    domain_name = urldecode(std::string(compute_domain_name, info.f_tld - compute_domain_name));

    // now cut the remainder on each period, these are the sub-domains
    // there may be none if there are no other periods in the full name
    if(compute_domain_name > fd)
    {
        // forget the period
        --compute_domain_name;
    }
    std::string all_sub_domains(std::string(fd, compute_domain_name - fd));

    // verify that all the sub-domains are valid (i.e. no "..")
    if(!all_sub_domains.empty())
    {
        snapdev::tokenize_string(sub_domain_names, all_sub_domains, ".");

        for(auto & sub_domain : sub_domain_names)
        {
            if(sub_domain.empty())
            {
                // sub-domains cannot be empty or the URI includes
                // two period one after the other (this should actually
                // be caught by the tld() call.)
                //
                return false;
            }

            // make sure it is decodable
            //
            sub_domain = urldecode(sub_domain);

            // TODO: look into whether we have to check for periods in the
            //       decoded sub-domain names (i.e. a %2E is probably not a
            //       valid character in a sub-domain name, at the same time
            //       if we reach here, there should not be such a DNS entry...
            //       but not automatically because a hacker can take an IP
            //       and use it with any URI and send an HTTP request that
            //       way... still, we would catch that in our domain/website
            //       canonicalization.) Maybe we should decode the domain part
            //       first, then parse it.
        }
    }

    return true;
}


/** \brief Set the domain to 'domain'.
 *
 * This function changes the Snap URI to the specified full domain.
 * This means changing the set of sub-domains, the TLD and the domain
 * it-self are updated with the corresponding data from the full domain.
 * The function takes care of breaking the input
 *
 * If any error is discovered in the full domain name, then the internal
 * variables do not get modified.
 *
 * Note that the domain is not expected to include a user name, password
 * and port information. You want to get rid of that information before
 * calling this function or consider calling set_uri() instead.
 *
 * \note
 * The only potential problem is when you get an out of memory error
 * while allocating a string.
 *
 * \todo
 * Check that the URL is not an IPv4 or IPv6 address. Such will always
 * fail and we should look into avoiding the use of an exception in
 * that circumstance.
 *
 * \exception uri_exception_invalid_uri
 * If the domain cannot properly be broken up in sub-domains,
 * the doman name and the tld, then this exception is raised.
 *
 * \param[in] full_domain_name  A full domain name, without protocol, path,
 *                              query string or anchor.
 */
void uri::set_domain(std::string const & full_domain_name)
{
    advgetopt::string_list_t sub_domain_names;
    std::string domain_name;
    std::string tld;
    if(!process_domain(full_domain_name, sub_domain_names, domain_name, tld))
    {
        throw uri_exception_invalid_uri(
              "could not break up \""
            + full_domain_name
            + "\" as a valid domain name");
    }

    f_domain = domain_name;
    f_top_level_domain = tld;
    f_sub_domains = sub_domain_names;

    f_address_ranges.clear();
}


/** \brief Reconstruct the full domain from the broken down information
 *
 * This function rebuilds a full domain name from the broken down
 * data saved in the Snap URI: the sub-domains, the domain name,
 * and the TLD.
 *
 * \todo
 * Add caching so calling the function more than once will be fast.
 *
 * \return The full domain name representation of this Snap URI.
 */
std::string uri::full_domain() const
{
    std::string full_domains(snapdev::join_strings(f_sub_domains, "."));
    if(!full_domains.empty())
    {
        full_domains += '.';
    }
    full_domains += f_domain;
    full_domains += f_top_level_domain;
    return full_domains;
}

/** \brief Get the top level domain name.
 *
 * This function returns the top level domain name by itself.
 * For example, in "www.example.com", the top level domain name
 * is "com".
 *
 * \return The top level domain name of the Snap URI.
 */
std::string const& uri::top_level_domain() const
{
    return f_top_level_domain;
}


/** \brief Get the domain name by itself.
 *
 * This function returns the stripped down domain name. This name
 * has no period since it includes no sub-domains and no top level
 * domain names.
 *
 * \return The stripped down domain name.
 */
std::string const & uri::domain() const
{
    return f_domain;
}


/** \brief Return the concatenated list of sub-domains.
 *
 * This function returns the concatenated list of sub-domains
 * in one string.
 *
 * \return The concatenated sub-domains separated by periods.
 */
std::string uri::sub_domains() const
{
    return snapdev::join_strings(f_sub_domains, ".");
}


/** \brief Return the number of sub-domains defined.
 *
 * This function defines a set of sub-domains.
 *
 * \return The number of sub-domains.
 */
int uri::sub_domain_count() const
{
    return f_sub_domains.size();
}


/** \brief Return one of the sub-domain names.
 *
 * This function returns the specified domain name.
 *
 * \param[in] part  The sub-domain name index.
 *
 * \return The sub-domain corresponding to the specified index.
 */
std::string uri::sub_domain(int part) const
{
    if(static_cast<std::size_t>(part) >= f_sub_domains.size())
    {
        throw edhttp_uri_exception_out_of_bounds(
              "sub-domain "
            + std::to_string(part)
            + " does not exist (range 0 to "
            + std::to_string(f_sub_domains.size())
            + ")");
    }
    return f_sub_domains[part];
}


/** \brief Return the array of sub-domains.
 *
 * This function gives you a constant reference to all the sub-domains
 * at once. You may use this function to make use of the list iterator,
 * for example.
 *
 * The strings are in order as in the first is the left-most sub-domain
 * (or the furthest away from the domain name.)
 *
 * \return A list of strings representing the sub-domains.
 */
advgetopt::string_list_t const & uri::sub_domains_list() const
{
    return f_sub_domains;
}


/** \brief Transforms the hostname and port in an array of addresses.
 *
 * This function generates an array of addresses for the specified
 * hostname and port.
 *
 * The function calls the full_domain() function to get the domain name
 * and uses get_port() for the port. From the resulting data, it attempts
 * to compute one or more addresses which can be used to connect to
 * the specified domain (i.e. if you have an IPv6 and IPv4 or multiple
 * computers, then this will return more than one IP address).
 *
 * The domain can later be retrieved using the addr::get_hostname()
 * function.
 *
 * \return A reference to a vector of addr::addr_range objects.
 */
addr::addr_range::vector_t const & uri::address_ranges()
{
    if(f_address_ranges.empty())
    {
        addr::addr_parser p;
        p.set_default_port(get_port());
        p.set_protocol(IPPROTO_TCP);
        p.set_sort_order(addr::SORT_IPV6_FIRST | addr::SORT_NO_EMPTY);
        p.set_allow(addr::allow_t::ALLOW_REQUIRED_ADDRESS, true);
        f_address_ranges = p.parse(full_domain());
    }

    return f_address_ranges;
}


/** \brief Set the port to the specified string.
 *
 * This function changes the port of the URI from what it is now
 * to the specified value.
 *
 * The port value must be a positive number or zero.
 *
 * Negative values or other invalid numbers generate an error.
 *
 * You can retrieve the port number with the get_port() function.
 *
 * \exception uri_exception_invalid_parameter
 * This function generates an exception if an invalid port is detected
 * (negative, larger than 65535, or characters other than 0-9).
 *
 * \param[in] port  The new port for this Snap URI object.
 */
void uri::set_port(std::string const & port)
{
    long p = std::stol(port);
    if(p < 0 || p > 65535)
    {
        throw uri_exception_invalid_parameter(
              "\""
            + port
            + "\" is an invalid port number");
    }
    f_port = p;
    f_address_ranges.clear();
}


/** \brief Set the port to the specified string.
 *
 * This function changes the port of the URI from what it is now
 * to the specified value.
 *
 * The port value must be a positive number or zero.
 *
 * Negative values or invalid numbers generate an error.
 *
 * \exception uri_exception_invalid_parameter
 * This function generates an exception if an invalid port is
 * detected (negative or characters other than 0-9).
 *
 * \param[in] port  The new port for this Snap URI object.
 */
void uri::set_port(int port)
{
    if(port < 0 || port > 65535)
    {
        throw uri_exception_invalid_parameter(
              "port \""
            + std::to_string(port)
            + "\" is out of range (1 to 65535)");
    }
    f_port = port;
}


/** \brief Retrieve the port number.
 *
 * This function returns the specific port used to access
 * the server. This parameter can be used as one of the
 * options used to select a specific website.
 *
 * \return The port as an integer.
 */
int uri::get_port() const
{
    return f_port;
}


/** \brief Replace the current path.
 *
 * This function can be used to replace the entire path of
 * the URI by starting the new path with a slash (/something).
 * If the \p path parameter does not start with a slash, then
 * it is used as a relative path from the existing path.
 *
 * A path includes parts separated by one or more slashes (/).
 * The function removes parts that are just "." since these
 * mean "this directory" and they would not be valid in a
 * canonicalized path.
 *
 * A path may include one or more ".." as a path part. These
 * mean remove one part prior.
 *
 * The ".." are accepted in any path, however, it must be
 * correct in that it is not possible to use ".." without at
 * least one part just before that (i.e. "/this/one/../other/one" is
 * valid, but "/../that/one/is/not" since ".." from / does not
 * exist. This is not how Unix usually manages paths since
 * in Unix / and /.. are one and the same folder.)
 *
 * Note that if you wanted to make use of the hash bang feature,
 * you would still make use of this function to setup your path in
 * the Snap URI object. The hash bang feature determines how
 * the path is handled when you get the URI with get_uri().
 *
 * \exception uri_exception_invalid_path
 * The function raises this exception if the path includes more
 * ".." than there are "normal" parts on the left side of the "..".
 *
 * \param[in] uri_path  The new path for this URI.
 *
 * \sa path()
 */
void uri::set_path(std::string uri_path)
{
    // check whether the path starts with a '/':
    // if so, then we replace the existing path;
    // if not, then we append uri_path to the existing path.
    //
    if((uri_path.empty() || uri_path[0] != '/')
    && !f_path.empty())
    {
        // append unless the user passed a path starting with "/"
        // or the current path is empty
        uri_path = snapdev::join_strings(f_path, "/") + "/" + uri_path;
    }

    // if the path starts with a '/' or includes a double '/'
    // within itself, it will be removed because of the SkipEmptyParts
    advgetopt::string_list_t p;
    advgetopt::split_string(uri_path, p, {"/"});

    // next we remove all ".." (and the previous part); if ".." was
    // at the start of the path, then an exception is raised
    //
    int max_parts(p.size());
    for(int i(0); i < max_parts; ++i)
    {
        if(p[i] == ".")
        {
            // canonalization includes removing "." parts which are
            // viewed exactly as empty parts
            p.erase(p.begin() + i);
            --i;
            --max_parts;
        }
        else if(p[i] == "..")
        {
            // note: max should not be less than 2 if i != 0
            if(i == 0 || max_parts < 2)
            {
                throw uri_exception_invalid_path(
                      "path \""
                    + uri_path
                    + "\" is not valid (it includes too many \"..\")");
            }
            p.erase(p.begin() + i - 1, p.begin() + i + 1);
            --i;
            max_parts -= 2;
        }
    }

    // the input was valid, save the new result
    f_path.swap(p);
}


/** \brief Return the full path.
 *
 * This function returns the full concatenated path of the URI.
 *
 * The function encodes the path appropriately. The path can thus be
 * used anywhere an encoded path is accepted. The encoding can be
 * avoided by setting the \p encoded flag to false.
 *
 * Note that a non encoded path may include / characters instead of
 * the %2F encoded character and thus not match the internal path.
 *
 * \note
 * The URL encode will not encode the ~ character which is at times
 * used for user references (~username/...).
 *
 * \warning
 * The result of the function returns what looks like a relative path.
 * This is useful since in many cases you need to remove the starting
 * slash, so we avoid adding it in the first place. If there is no path,
 * the function returns the empty string ("").
 *
 * \param[in] encoded  Should the resulting path be URL encoded already?
 * By default the path is URL encoded as expected by the HTTP protocol.
 *
 * \return The full path of the URI.
 */
std::string uri::path(bool encoded) const
{
    if(encoded)
    {
        std::string output;
        bool first(true);
        for(auto const segment : f_path)
        {
            if(first)
            {
                first = false;
            }
            else
            {
                output += '/';
            }
            output += urlencode(segment, "~");
        }
        return output;
    }
    return snapdev::join_strings(f_path, "/");
}


/** \brief Retrieve the number of folder names defined in the path.
 *
 * This function returns the number of folder names defined in the
 * path. Each name can be retrieved with the path_folder() function.
 *
 * The function may return 0 if no folder name is available.
 *
 * \return The number of folder names available.
 *
 * \sa path_folder()
 */
int uri::path_count() const
{
    return f_path.size();
}


/** \brief Get a folder name from the path.
 *
 * This function is used to retrieve the name of a specific folder.
 * This is useful when you make use of a folder name as a dynamic
 * name. For example with a path such as "journal/george",
 * path_folder_name(1); returns "george" which may be the name of
 * the journal owner.
 *
 * When you use this function to retrieve dynamic entries, it is
 * assumed that you do it after the path options were removed so a
 * path such as "en/journal/george" would be changed to
 * "journal/george" and path_folder_name(1); would still return
 * "george".
 *
 * \exception edhttp_uri_exception_out_of_bounds
 * This function raises this exception if the \p part parameter is
 * outside the range of folder names available. \p part should be
 * between 0 and path_count() - 1. If the path is empty, then this
 * function cannot be called.
 *
 * \param[in] part  The index of the folder to retrieve.
 *
 * \return The folder name.
 *
 * \sa path_count();
 */
std::string uri::path_folder_name(int part) const
{
    if(static_cast<std::size_t>(part) >= f_path.size())
    {
        throw edhttp_uri_exception_out_of_bounds(
              "no path section "
            + std::to_string(part)
            + " available (range 0 to "
            + std::to_string(f_path.size())
            + ")");
    }
    return f_path[part];
}


/** \brief The array of folder names.
 *
 * This function returns a reference to the array used to hold the
 * folder names forming the URI path.
 *
 * \return A constant reference to the list of string forming the path.
 */
advgetopt::string_list_t const & uri::path_list() const
{
    return f_path;
}


/** \brief Set an option.
 *
 * This function is used to define the value of an option in a URI.
 * Remember that options only work for URIs that are clearly marked
 * as from this website.
 *
 * Setting the value to an empty string has the effect of deleting
 * the given option. You may also call the unset_option() function.
 *
 * \param[in] name  The name of the option to set.
 * \param[in] value  The new value for this option.
 *
 * \sa option();
 * \sa unset_option();
 */
void uri::set_option(std::string const& name, std::string const& value)
{
    if(value.empty())
    {
        auto it(f_options.find(name));
        if(it != f_options.end())
        {
            f_options.erase(it);
        }
    }
    else
    {
        f_options[name] = value;
    }
}

/** \brief Remove the specified option.
 *
 * This function is used to remove (delete) an option from the list
 * of options. For example, going to a page where the language is
 * neutral, you probably want to remove the language option.
 *
 * \param[in] name  The name of the option to remove.
 *
 * \sa set_option();
 */
void uri::unset_option(std::string const & name)
{
    auto it(f_options.find(name));
    if(it != f_options.end())
    {
        f_options.erase(it);
    }
}


/** \brief Retrieve the value of the named option.
 *
 * This function retrieves the current value of the named option.
 *
 * If the option is not defined, then the function returns an empty
 * string. The empty string always represents an undefined option.
 *
 * \param[in] name  The name of the option to retrieve.
 *
 * \return The value of the named option.
 *
 * \sa set_option();
 */
std::string uri::option(std::string const& name) const
{
    auto it(f_options.find(name));
    if(it != f_options.end())
    {
        return it->second;
    }
    return std::string();
}


/** \brief Retrieve the number of currently defined options.
 *
 * This function returns the number of options that can be retrieved
 * with the option() function using an index. If the function returns
 * zero, then no options are defined.
 *
 * \return The number of options defined in this URI.
 */
int uri::option_count() const
{
    return f_options.size();
}


/** \brief Retrieve an option by index.
 *
 * This function allows you to retrieve the name and value of an option
 * using its index. The index (\p part) must be a number between 0 and
 * option_count() - 1.
 *
 * \param[in] part  The index of the option to retrieve.
 * \param[out] name  The name of the option being retrieved.
 *
 * \return The value of the option being retrieved.
 *
 * \sa option();
 * \sa option_count();
 */
std::string uri::option(int part, std::string & name) const
{
    if(static_cast<std::size_t>(part) >= f_options.size())
    {
        throw edhttp_uri_exception_out_of_bounds(
              "no option "
            + std::to_string(part)
            + " available (range 0 to "
            + std::to_string(f_options.size())
            + ")");
    }
    auto it(f_options.begin());
    std::advance(it, part);
    name = it->first;
    return it->second;
}


/** \brief Retrieve the map of options.
 *
 * This function returns the map of options so one can use the begin()
 * and end() functions to go through the entire list without having to
 * use the option() function.
 *
 * \return A constant reference to the map of options.
 *
 * \sa option();
 */
uri::uri_options_t const& uri::options_list() const
{
    return f_options;
}


/** \brief Set a query string option.
 *
 * This function is used to change the named query string with the
 * specified value.
 *
 * A query string option with an empty string as a value is considered
 * undefined and is not shown on the final URI. So setting an option to
 * the empty string ("") is equivalent to unset_query_option().
 *
 * \param[in] name  The name of the query string option.
 * \param[in] value  The value of the query string option.
 */
void uri::set_query_option(std::string const& name, std::string const& value)
{
    if(name.empty())
    {
        // this happens if the name was not defined in the configuration file
        return;
    }

    // TODO: see whether we currently use this feature, because it is rather
    //       incorrect, it is possible to have an empty value in a query
    //       string (i.e. "...?logout")
    //
    //       we should use unset_query_option() instead
    //
    if(value.empty())
    {
        auto it(f_query_strings.find(name));
        if(it != f_query_strings.end())
        {
            f_query_strings.erase(it);
        }
    }
    else
    {
        f_query_strings[name] = value;
    }
}


/** \brief Unset the named query string option.
 *
 * This function ensures that the named query string option is deleted
 * and thus will not appear in the URI.
 *
 * \param[in] name  The name of the option to delete.
 */
void uri::unset_query_option(std::string const& name)
{
    if(name.empty())
    {
        // this happens if the name was not defined in the configuration file
        return;
    }

    auto it(f_query_strings.find(name));
    if(it != f_query_strings.end())
    {
        f_query_strings.erase(it);
    }
}


/** \brief Set the query string.
 *
 * This function can be used to reset the query string to the
 * parameters defined in this URI query string.
 *
 * The function does not clear all the existing query strings,
 * it only replaces existing entries. This means also means that
 * it does not detect whether the input includes the same option
 * more than once and only the last one sticks.
 *
 * The query string variable names and data gets URL decoded.
 *
 * \warning
 * This function does not clear the existing list of query
 * string options.
 *
 * \param[in] uri_query_string  The query string to add to the existing data.
 */
void uri::set_query_string(std::string const & uri_query_string)
{
    advgetopt::string_list_t value_pairs;
    advgetopt::split_string(uri_query_string, value_pairs, {"&"});
    for(auto const & name_value : value_pairs)
    {
        std::string::size_type const pos(name_value.find('='));
        if(pos == std::string::npos)
        {
            // no value
            f_query_strings[urldecode(name_value)] = std::string();
        }
        else if(pos == 0)
        {
            // name is missing, use "*" instead
            f_query_strings["*"] = urldecode(name_value.substr(1));
        }
        else
        {
            f_query_strings[urldecode(name_value.substr(0, pos))] = urldecode(name_value.substr(pos + 1));
        }
    }
}


/** \brief Clear all query option strings.
 *
 * This is useful if you want to "start fresh" with the base URI.
 */
void uri::clear_query_options()
{
    f_query_strings.clear();
}


/** \brief Generate the query string.
 *
 * This function goes through the list of defined query string options
 * and builds the resulting query string to generate the final URI.
 *
 * The result is already URL ecoded since you would otherwise not know
 * where/which equal and ampersand are legal.
 *
 * \return The URI query string.
 */
std::string uri::query_string() const
{
    std::string result;
    for(auto const & name_value : f_query_strings)
    {
        if(!result.empty())
        {
            result += '&';
        }
        result += urlencode(name_value.first);
        if(!name_value.second.empty())
        {
            // add the value only if not empty
            result += '=';
            // we now support commas in URIs because... well... it is
            // common and it won't break anything
            //
            result += urlencode(name_value.second, ",");
        }
    }
    return result;
}


/** \brief Retrieve whether a query option is defined.
 *
 * This function returns true if a query option is defined. Note that
 * an option may be the empty string ("") and that cannot be distinguish
 * from the empty string ("") returned when the query_option() function
 * is used against an undefined option.
 *
 * \param[in] name  The name of the option to query.
 *
 * \return true when the has_query_option() is defined.
 *
 * \sa query_option();
 */
bool uri::has_query_option(std::string const & name) const
{
    if(name.empty())
    {
        // this happens if the name was not defined in the configuration file
        return false;
    }

    return f_query_strings.find(name) != f_query_strings.end();
}

/** \brief Retrieve a query string option.
 *
 * This function can be used to retrieve the current value of a query
 * string option.
 *
 * Note that you cannot know whether an option is defined using this
 * function since the function returns an empty string whether it is
 * empty or undefined. Instead, use the has_query_option() function
 * to determine whether an option is defined.
 *
 * \param[in] name  Name of the query string option to return.
 *
 * \sa has_query_option();
 */
std::string uri::query_option(std::string const & name) const
{
    if(!name.empty())
    {
        auto const it(f_query_strings.find(name));
        if(it != f_query_strings.end())
        {
            return it->second;
        }
    }

    return std::string();
}

/** \brief Return the number of options are defined in the query string.
 *
 * This function returns the number of options currently defined in the
 * query string. This is useful to go over the list of options with the
 * query_option(int part, QString& name) function.
 *
 * \return The number of query string options currently defined.
 */
int uri::query_option_count() const
{
    return f_query_strings.size();
}

/** \brief Retrieve an option specifying its index.
 *
 * This function returns the name and value of the option defined at
 * index \p part.
 *
 * The index must be between 0 and the number of options available minus
 * 1 (i.e. query_options_count() - 1).
 *
 * \param[in] part  The index of the query string option to retrieve.
 * \param[out] name  The name of the option at that index.
 *
 * \return The value of the option at that index.
 *
 * \sa query_option_count();
 */
std::string uri::query_option(int part, std::string& name) const
{
    if(static_cast<std::size_t>(part) >= f_query_strings.size())
    {
        throw edhttp_uri_exception_out_of_bounds(
              "query-option "
            + std::to_string(part)
            + " does not exist (range 0 to "
            + std::to_string(f_query_strings.size())
            + ")");
    }
    auto it(f_query_strings.begin());
    std::advance(it, part);
    name = it->first;
    return it->second;
}

/** \brief Return the complete map of query strings.
 *
 * This function returns a reference to the internal map of query strings.
 * This is useful to use the begin()/end() and other functions to go through
 * the map.
 *
 * \return A constant reference to the internal query string map.
 */
const uri::uri_options_t& uri::query_string_list() const
{
    return f_query_strings;
}


/** \brief Define the anchor for this URI.
 *
 * This function is used to setup the anchor used in this URI.
 *
 * An anchor can be defined only if you don't plan to make use of
 * the hash bang feature (see get_uri() for more info) since both
 * features make use of the same technical option.
 *
 * The \p anchor parameter cannot include a '#' character.
 *
 * \note
 * The anchor string can start with a bang (!) since it is legal
 * in an anchor. If you are not using the hash bang feature, it
 * is fine, although it may confuse some search engines.
 *
 * \param[in] uri_anchor  The new value for the anchor.
 *
 * \sa get_uri()
 */
void uri::set_anchor(std::string const & uri_anchor)
{
    if(uri_anchor.find('#') != std::string::npos)
    {
        throw uri_exception_invalid_parameter(
              "anchor string \""
            + uri_anchor
            + "\" cannot include a '#' character");
    }
    f_anchor = uri_anchor;
}


/** \brief Retrieve the current anchor.
 *
 * This function returns a copy of the current anchor. The empty string
 * represents the fact that the anchor is not defined.
 *
 * \return A constant reference to the anchor.
 */
std::string const & uri::anchor() const
{
    return f_anchor;
}


/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if they are
 * equal. The URIs are tested using what the get_uri() function
 * generates which means not 100% of the information included
 * in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when both URIs are equal.
 */
bool uri::operator == (const uri& rhs) const
{
    return get_uri() == rhs.get_uri();
}


/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if they are
 * not equal. The URIs are tested using what the get_uri() function
 * generates which means not 100% of the information included
 * in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when both URIs differ.
 */
bool uri::operator != (uri const & rhs) const
{
    return !operator == (rhs);
}


/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if this is
 * smaller than the \p rhs parameter. The URIs are tested using
 * what the get_uri() function generates which means not 100% of
 * the information included in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when this is smaller than rhs.
 */
bool uri::operator < (uri const & rhs) const
{
    return get_uri() < rhs.get_uri();
}


/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if this is
 * smaller or equal to \p rhs. The URIs are tested using
 * what the get_uri() function generates which means not 100% of
 * the information included in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when this is smaller or equal to rhs.
 */
bool uri::operator <= (uri const & rhs) const
{
    return get_uri() <= rhs.get_uri();
}


/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if this is
 * larger than the \p rhs parameter. The URIs are tested using
 * what the get_uri() function generates which means not 100% of
 * the information included in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when this is larger than rhs.
 */
bool uri::operator > (uri const & rhs) const
{
    return !operator <= (rhs);
}


/** \brief Compare two URIs against each other.
 *
 * This function compares two URIs and returns true if this is
 * larger or equal to \p rhs. The URIs are tested using
 * what the get_uri() function generates which means not 100% of
 * the information included in the Snap URI object.
 *
 * \param[in] rhs  The right handside to compare this against.
 *
 * \return true when this is larger or equal to rhs.
 */
bool uri::operator >= (uri const & rhs) const
{
    return !operator < (rhs);
}


/** \brief Encode a URI so it is valid for HTTP.
 *
 * This function encodes all the characters that need to be encoded
 * for a URI to be valid for the HTTP protocol.
 *
 * WARNING: This encodes the entire string. Remember that the string
 * cannot include characters such as :, /, @, ?, =, &, #, ~ which at
 * times appear in fully qualified URIs. Instead, it must be built
 * piece by piece.
 *
 * Note that we do not encode underscores.
 *
 * The \p accepted parameter can be used to avoid converting certain
 * characters (such as / in an anchor and ~ in a path).
 *
 * \param[in] in  URI to encode.
 * \param[in] accepted  Extra characters accepted and not encoded. This
 * parameter cannot be set to nullptr. Use "" instead if no extra characters
 * are accepted.
 *
 * \return The encoded URI, it may be equal to the input.
 */
std::string uri::urlencode(std::string const & in, char const * accepted)
{
    std::string encoded;

    for(const char *u(in.data()); *u != '\0'; ++u)
    {
        if((*u >= 'A' && *u <= 'Z')
        || (*u >= 'a' && *u <= 'z')
        || (*u >= '0' && *u <= '9')
        || *u == '.' || *u == '-' || *u == '_'
        || strchr(accepted, *u) != nullptr)
        {
            encoded += *u;
        }
        else
        {
            // note that we are encoding space as %20 and not +
            // because the + should not be supported anymore
            encoded += '%';
            encoded += snapdev::int_to_hex(*u, true, 2);
        }
    }

    return encoded;
}


/** \brief Decode a URI so it can be used internally.
 *
 * This function decodes all the characters that need to be decoded
 * in a URI. In general, this is done to use URI components in a
 * query string, although it needs to be applied to the entire URI.
 *
 * The input is expected to be a valid ASCII string (i.e. A-Z,
 * 0-9, ., %, _, -, ~, and ! characters.) To enter UTF-8 characters,
 * use the % and UTF-8 encoded characters. At this point we do not
 * support the U+ syntax which MS Internet Explorer supports. It may
 * be necessary to add that support at some point.
 *
 * \exception uri_exception_invalid_uri
 * This exception is raised if an invalid character is found in the
 * input URI. This means the URI includes a character that should
 * have been encoded or a %XX is not a valid hexadecimal number.
 *
 * \param[in] in  The URI to encode.
 * \param[in] relax  Relax the syntax and accept otherwise invalid codes.
 *
 * \return The decoded URI, it may be equal to the input.
 */
std::string uri::urldecode(std::string const & in, bool relax)
{
    // Note that if the URI is properly encoded, then latin1 == UTF-8

    std::string out;
    for(char const * u(in.c_str()); *u != '\0'; ++u)
    {
        if(*u == '+')
        {
            out += ' ';
        }
        else if(*u == '%')
        {
            ++u;
            char c;
            if(u[0] >= '0' && u[0] <= '9')
            {
                c = static_cast<char>((u[0] - '0') * 16);
            }
            else if(u[0] >= 'A' && u[0] <= 'F')
            {
                c = static_cast<char>((u[0] - ('A' - 10)) * 16);
            }
            else if(u[0] >= 'a' && u[0] <= 'f')
            {
                c = static_cast<char>((u[0] - ('a' - 10)) * 16);
            }
            else
            {
                if(!relax)
                {
//#ifdef DEBUG
//SNAP_LOG_TRACE() << "url decode?! [" << uri << "]";
//#endif
                    throw uri_exception_invalid_uri(
                          "urldecode(\""
                        + in
                        + "\", "
                        + (relax ? "true" : "false")
                        + ") failed because of an invalid %xx character (digits are "
                        + std::to_string(u[0])
                        + " / "
                        + std::to_string(u[1])
                        + ")");
                }
                // use the % as is
                out += '%';
                --u;
                continue;
            }
            if(u[1] >= '0' && u[1] <= '9')
            {
                c = static_cast<char>(c + u[1] - '0');
            }
            else if(u[1] >= 'A' && u[1] <= 'F')
            {
                c = static_cast<char>(c + u[1] - ('A' - 10));
            }
            else if(u[1] >= 'a' && u[1] <= 'f')
            {
                c = static_cast<char>(c + u[1] - ('a' - 10));
            }
            else
            {
                if(!relax)
                {
//#ifdef DEBUG
//SNAP_LOG_TRACE() << "url decode?! [" << in << "] (2)";
//#endif
                    throw uri_exception_invalid_uri(
                          "urldecode(\""
                         + in
                         + "\", "
                         + (relax ? "true" : "false")
                         + ") failed because of an invalid %xx character (digits are "
                         + std::to_string(static_cast<int>(u[0]))
                         + " / "
                         + std::to_string(static_cast<int>(u[1]))
                         + ")");
                }
                // use the % as is
                out += c;
                --u;
                continue;
            }
            // skip one of the two characters here, the other
            // is skipped in the for() statement
            ++u;
            out += c;
        }
        else if(relax

                // these are the only characters allowed by the RFC
                || (*u >= 'A' && *u <= 'Z')
                || (*u >= 'a' && *u <= 'z')
                || (*u >= '0' && *u <= '9')
                || *u == '.' || *u == '-'
                || *u == '/' || *u == '_'

                // not legal in a URI considered 100% valid but most
                // systems accept the following as is so we do too
                || *u == '~' || *u == '!'
                || *u == '@' || *u == ','
                || *u == ';' || *u == ':'
                || *u == '(' || *u == ')'
        )
        {
            // The tilde (~), when used, is often to indicate a user a la
            // Unix (~<name>/... or just ~/... for the current user.)
            //
            // The exclamation point (!) is most often used with the hash
            // bang; if that appears in a query string variable, then we
            // need to accept at least the exclamation point (the hash has
            // to be encoded no matter what.)
            //
            // The at sign (@) is used in email addresses.
            //
            // The comma (,) is often used to separate elements; for example
            // the paging support uses "page=p3,s30" for show page 3 with
            // 30 elements per page.
            //
            // The semi-colon (;) may appear if you have an HTML entity in
            // a query string (i.e. "...?value=this+%26amp;+that".)
            //
            // The colon (:) can be used to separate values within a
            // parameter when the comma is not appropriate.
            //
            out += *u;
        }
        else
        {
//#ifdef DEBUG
//SNAP_LOG_TRACE() << "url decode?! found an invalid character [" << in << "] (3)";
//#endif
            throw uri_exception_invalid_uri(
                    "urldecode(\""
                  + in
                  + "\", "
                  + (relax ? "true" : "false")
                  + ") failed because of an invalid character ("
                  + std::to_string(static_cast<int>(*u))
                  + ")");
        }
    }

    return out;
}


/** \brief Return the port corresponding to a protocol.
 *
 * This function determines what port corresponds to a given protocol
 * assuming that the default is being used.
 *
 * It will handle common protocols internally, others make use of the
 * /etc/services file via the services function calls.
 *
 * \param[in] protocol  The protocol to convert to a port number.
 *
 * \return The corresponding port number or -1 if the function cannot
 *         determine that number.
 */
int uri::protocol_to_port(std::string const & protocol)
{
    if(protocol == "http") // 99% so put it first
    {
        return 80;
    }
    if(protocol == "https") // 0.9% so put it next
    {
        return 443;
    }
    if(protocol == "ftp")
    {
        return 21;
    }
    if(protocol == "ssh")
    {
        return 22;
    }
    if(protocol == "telnet")
    {
        return 23;
    }
    if(protocol == "smtp")
    {
        return 25;
    }
    if(protocol == "gopher")
    {
        return 70;
    }

    // not a common service, ask the system... (probably less than 0.01%)
    servent * s(getservbyname(protocol.c_str(), "tcp"));
    if(s == nullptr)
    {
        s = getservbyname(protocol.c_str(), "udp");
        if(s == nullptr)
        {
            // we don't know...
            return -1;
        }
    }
    return s->s_port;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
