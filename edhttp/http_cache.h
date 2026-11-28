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
#pragma once

// self
//
#include    <edhttp/http_request.h>
#include    <edhttp/http_response.h>


// C++
//
//#include    <map>
//#include    <memory>
//#include    <string>
//#include    <unordered_map>



namespace edhttp
{
namespace cache
{



void                        set(http_response::pointer_t response);
http_response::pointer_t    get(http_request::pointer_t request);
void                        invalidate(http_request::pointer_t request);



} // namespace cache
} // namespace edhttp
// vim: ts=4 sw=4 et
