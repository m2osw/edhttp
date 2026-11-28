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
 * \brief Cache handling for HTTP clients.
 *
 * The key used by browsers include the following parameters:
 *
 * * method (GET and HEAD)
 * * full target URI
 *   - origin (this is what we set the Origin header to)
 *     + scheme
 *     + host
 *     + port
 *   - path
 *   - query string (as is)
 * * any request headers specified by the response's Vary header
 *
 * On out end, though, we really only cache GET requests and use HEAD
 * requests to update the data of a GET if it exists. Anything else,
 * except OPTIONS, generates a call to the invalidate() function instead.
 * The OPTIONS does nothing (not cached and no invalidation).
 *
 * Further, the cache data is isolated by the top-level origin (cache
 * partitioning). i.e. data from domain A cannot be mixed with data
 * from domain B based on the origin (a.k.a. assuming www.example.com
 * and app.example.com are two different websites with different origins,
 * then the cache from one cannot be used by the other). Note that
 * partitioning is particularly important if you execute the JavaScript
 * of a page. If not, then having a single central cache is still very
 * safe.
 *
 * Note that the query string is used as is. That means "?a=1&b=2" is
 * not the same as "?b=2&a=1".
 *
 * Of course, the fragment is totally ignored by caches. It only defines
 * a position within the page and it doesn't get sent to the server anyway.
 *
 * If the server response says "Vary: *" then nothing can be cached.
 *
 * About the method, the PATCH, POST, PUT, and DELETE actually invalidate the
 * cache.
 */

// self
//
#include    <edhttp/http_cache.h>

#include    <edhttp/cache_control.h>
#include    <edhttp/exception.h>
#include    <edhttp/http_request.h>
#include    <edhttp/names.h>
#include    <edhttp/parametrized_value_list.h>
#include    <edhttp/token.h>


// snaplogger
//
//#include    <snaplogger/message.h>


// magpie
//
#include    <magpie/cache.h>


// libaddr
//
//#include    <libaddr/addr_parser.h>


// snapdev
//
#include    <snapdev/compare_switch_string.h>
//#include    <snapdev/not_reached.h>


// C++
//
#include    <algorithm>
//#include    <iostream>
//#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{
namespace cache
{
namespace
{



// one cache entry can have multiple variants
//
class page_variants
{
public:
    typedef std::shared_ptr<page_variants>      pointer_t;

    static constexpr std::size_t const          MAX_VARIANTS = 3;

    static std::string          uri_to_key(uri const & u);
    void                        add_response(http_response::pointer_t response);
    http_response::pointer_t    find_response(http_request::pointer_t request);

private:
    http_response::vector_t     f_responses = http_response::vector_t();
};


std::string page_variants::uri_to_key(uri const & u)
{
    std::string key;

    std::string const & scheme(u.scheme());
    int const port(u.get_port());

    // scheme
    //
    key += scheme;
    key += "://";

    // full domain name (should be origin?)
    //
    key += u.full_domain();

    // port if not default for HTTP or HTTPS
    //
    if((scheme != "http" || port != 80)
    && (scheme != "https" || port != 443))
    {
        key += ':';
        key += std::to_string(port);
    }

    // path if not empty
    //
    std::string const path(u.path());
    if(!path.empty())
    {
        key += '/';
        key += path;
    }

    // query string if not empty
    //
    std::string const query_string(u.query_string());
    if(!query_string.empty())
    {
        key += '?';
        key += query_string;
    }

    return key;
}


void page_variants::add_response(http_response::pointer_t response)
{
    std::string const vary_header(response->get_header(g_name_edhttp_field_vary));

    parametrized_value_list vary(vary_header, PARAMETRIZED_VALUE_MODE_VARY);
    parametrized_value::vector_t & vary_fields(vary.get_parts());
    if(vary_fields.empty())
    {
        f_responses.clear();
        f_responses.push_back(response);
        return;
    }

    http_request::pointer_t new_request(response->get_request());

    time_t oldest_date(0);
    std::size_t oldest_idx(0);
    for(std::size_t idx(0); idx < f_responses.size(); ++idx)
    {
        http_response::pointer_t r(f_responses[idx]);
        http_request::pointer_t old_request(r->get_request());
        bool found(true);
        for(auto const & p : vary_fields)
        {
            std::string const & name(p.get_name());
            if(snapdev::compare_exact_string<"*">(name))
            {
                // something that our client cache cannot compute varies
                // on the server side so we cannot cache this one
                //
                return;
            }
            if(new_request->get_header(name) != old_request->get_header(name))
            {
                found = false;
                break;
            }
        }
        if(found)
        {
            f_responses[idx] = response;
            return;
        }
        if(oldest_date == time_t())
        {
            oldest_date = f_responses[idx]->get_date();
            oldest_idx = idx;
        }
        else
        {
            time_t const date(f_responses[idx]->get_date());
            if(date < oldest_date)
            {
                oldest_date = date;
                oldest_idx = idx;
            }
        }
    }

    if(f_responses.size() >= MAX_VARIANTS)
    {
        f_responses.erase(f_responses.begin() + oldest_idx);
    }
    f_responses.push_back(response);
}


http_response::pointer_t page_variants::find_response(http_request::pointer_t request)
{
    if(f_responses.empty())
    {
        throw logic_error("logically the cache page_variants f_responses cannot ever be empty.");
    }

    // the request is new so it has no responses so we need to use one of
    // the responses found here
    //
    std::string const vary_header(f_responses[0]->get_header(g_name_edhttp_field_vary));

    parametrized_value_list vary(vary_header, PARAMETRIZED_VALUE_MODE_VARY);
    parametrized_value::vector_t & vary_fields(vary.get_parts());
    if(vary_fields.empty())
    {
        if(f_responses.size() != 1)
        {
            throw logic_error("logically the cache page_variants f_responses cannot ever be more than 1 if there is no Vary: ... header.");
        }

        return f_responses[0];
    }

    for(std::size_t idx(0); idx < f_responses.size(); ++idx)
    {
        http_response::pointer_t r(f_responses[idx]);
        http_request::pointer_t old_request(r->get_request());
        bool found(true);
        for(auto const & p : vary_fields)
        {
            std::string const & name(p.get_name());
            if(request->get_header(name) != old_request->get_header(name))
            {
                found = false;
                break;
            }
        }
        if(found)
        {
            return r;
        }
    }

    return http_response::pointer_t();
}



typedef magpie::cache<std::string, page_variants::pointer_t>    cache_t;

cache_t::pointer_t      g_http_cache = cache_t::pointer_t();


cache_t::pointer_t get_cache()
{
    if(g_http_cache == nullptr)
    {
        // TODO: look at given edhttp user the ability to set these parameters
        //
        magpie::options opts;
        opts.set_default_ttl(cache_control_settings::AGE_MAXIMUM);
        opts.set_cleanup_interval(3'600);
        opts.set_count_limit(100'000);
        g_http_cache = std::make_shared<cache_t>(opts);
    }

    return g_http_cache;
}



} // no name namespace



void set(http_response::pointer_t response)
{
    http_request::pointer_t request(response->get_request());
    std::string const method(request->get_method());

    if(snapdev::compare_upper_exact_string<"GET">(method))
    //|| snapdev::compare_upper_exact_string<"HEAD">(method)) -- TODO: also add support for HEAD requests
    {
        std::string const cache_control_header(response->get_header(g_name_edhttp_field_cache_control));
        if(!cache_control_header.empty())
        {
            cache_control_settings cache_control(cache_control_header, false);
            if(cache_control.get_no_store())
            {
                invalidate(request);
                return;
            }
        }

        // to make sure we do not depend on a `uri` class function which
        // could change under our feet, we gather the data we need and
        // build our key manually
        //
        // also since at the moment we limit our cache to just and only GET
        // and HEAD responses, we do not include the method in the key
        //
        page_variants::pointer_t variants;
        std::string const key(page_variants::uri_to_key(request->get_uri()));
        cache_t::pointer_t c(get_cache());
        cache_t::optional_t v(c->get(key));
        if(v.has_value())
        {
            variants = v.value();
        }
        else
        {
            variants = std::make_shared<page_variants>();
            c->set(key, variants);
        }
        variants->add_response(response);
        return;
    }

    if(snapdev::compare_upper_exact_string<"OPTIONS">(method))
    {
        // OPTIONS has no effects on caches and it is not expected to get
        // cached either
        //
        return;
    }

    invalidate(request);
}


http_response::pointer_t get(http_request::pointer_t request)
{
    std::string const key(page_variants::uri_to_key(request->get_uri()));
    cache_t::pointer_t c(get_cache());
    cache_t::optional_t v(c->get(key));
    if(!v.has_value())
    {
        return http_response::pointer_t();
    }

    return v.value()->find_response(request);
}


void invalidate(http_request::pointer_t request)
{
    std::string const key(page_variants::uri_to_key(request->get_uri()));
    cache_t::pointer_t c(get_cache());
    c->erase(key);
}



} // namespace cache
} // namespace edhttp
// vim: ts=4 sw=4 et
