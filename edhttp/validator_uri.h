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
#pragma once

/** \file
 * \brief Declaration of a URI validator to verify a parameter is a URI.
 *
 * The advgetopt library offers a validation system to verify command line
 * parameters. This addition allows to verify that a parameter is considered
 * a valid URI.
 */

// self
//
#include    <advgetopt/validator.h>



namespace edhttp
{



class validator_uri
    : public advgetopt::validator
{
public:
                                validator_uri(advgetopt::string_list_t const & data);

    // validator implementation
    //
    virtual std::string         name() const override;
    virtual bool                validate(std::string const & value) const override;

private:
    bool                        f_accept_ip = false;
    bool                        f_accept_path = false;
};



}   // namespace edhttp
// vim: ts=4 sw=4 et
