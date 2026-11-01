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
#include    <edhttp/utils.h>



// magpie
//
#include    <magpie/cache.h>


// C++
//
//#include    <string>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{
namespace
{



magpie::cache<std::string, magpie::empty_t>::pointer_t  g_accept_h3_cache = magpie::cache<std::string, magpie::empty_t>::pointer_t();



} // no name namespace



void accept_h3(std::string const & domain, magpie::duration_t ttl)
{
    if(g_accept_h3_cache == nullptr)
    {
        magpie::options opts;
        opts.set_default_ttl(86400);
        opts.set_cleanup_interval(3600);
        opts.set_count_limit(1'000);
        g_accept_h3_cache = std::make_shared<magpie::cache<std::string, magpie::empty_t>>(opts);
    }

    g_accept_h3_cache->set(domain, magpie::empty_t(), ttl);
}


bool accept_h3(std::string const & domain)
{
    if(g_accept_h3_cache == nullptr)
    {
        return false;
    }

    std::optional<magpie::empty_t> v(g_accept_h3_cache->get(domain));
    return v.has_value();
}



} // namespace edhttp
// vim: ts=4 sw=4 et
