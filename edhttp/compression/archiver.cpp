// Copyright (c) 2013-2024  Made to Order Software Corp.  All Rights Reserved
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
#include    "edhttp/compression/archiver.h"



// C++
//
#include    <ranges>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{


namespace
{



typedef std::map<std::string, archiver *>     archiver_map_t;

// IMPORTANT NOTE:
// This list only makes use of bare pointers for many good reasons.
// (i.e. all archivers are defined statitcally, not allocated)
// Do not try to change it! Thank you.
//
archiver_map_t * g_archivers;



} // no name namespace



archiver::archiver(char const * name)
{
    if(g_archivers == nullptr)
    {
        g_archivers = new archiver_map_t;
    }
    (*g_archivers)[name] = this;
}


archiver::~archiver()
{
    for(auto it(g_archivers->begin()); it != g_archivers->end(); ++it)
    {
        if(it->second == this)
        {
            g_archivers->erase(it);
            break;
        }
    }

    if(g_archivers->empty())
    {
        delete g_archivers;
        g_archivers = nullptr;
    }
}


advgetopt::string_list_t archiver_list()
{
    if(g_archivers == nullptr)
    {
        return advgetopt::string_list_t(); // LCOV_EXCL_LINE
    }

    auto kv = std::views::keys(*g_archivers);
    return advgetopt::string_list_t(kv.begin(), kv.end());
}


archiver * get_archiver(std::string const & archiver_name)
{
    if(g_archivers != nullptr)
    {
        auto it(g_archivers->find(archiver_name));
        if(it != g_archivers->end())
        {
            return it->second;
        }
    }

    return nullptr;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
