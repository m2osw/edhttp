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

// C++
//
#include    <string>



namespace edhttp
{



class token
    : public std::string
{
public:
    using std::string::string;
    using std::string::operator =;

                                token(token const & rhs) = default;
                                token(token && rhs) = default;

                                token(std::string const & s);
                                token(std::string && s);

    std::strong_ordering        operator <=> (token const & rhs);

    bool                        is_valid() const;

private:
};



bool        is_token(std::string const & token);
bool        is_restricted_token(std::string const & token);
int         compare_tokens(std::string const & lhs, std::string const & rhs, bool underscore_equal_dash = false);



} // namespace edhttp
// vim: ts=4 sw=4 et
