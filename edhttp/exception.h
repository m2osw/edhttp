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

// libexcept
//
#include    <libexcept/exception.h>



namespace edhttp
{



DECLARE_LOGIC_ERROR(logic_error);
DECLARE_LOGIC_ERROR(not_implemented);

DECLARE_OUT_OF_RANGE(out_of_range);

DECLARE_MAIN_EXCEPTION(edhttp_exception);

DECLARE_EXCEPTION(edhttp_exception, exclusive_parameters);
DECLARE_EXCEPTION(edhttp_exception, expected_token);
DECLARE_EXCEPTION(edhttp_exception, incompatible);
DECLARE_EXCEPTION(edhttp_exception, invalid_checksum);
DECLARE_EXCEPTION(edhttp_exception, invalid_parameter);
DECLARE_EXCEPTION(edhttp_exception, invalid_path);
DECLARE_EXCEPTION(edhttp_exception, invalid_token);
DECLARE_EXCEPTION(edhttp_exception, invalid_uri);
DECLARE_EXCEPTION(edhttp_exception, missing_name);
DECLARE_EXCEPTION(edhttp_exception, name_too_large);
DECLARE_EXCEPTION(edhttp_exception, too_many_names);
DECLARE_EXCEPTION(edhttp_exception, unquotable_string);

DECLARE_EXCEPTION(edhttp_exception, cookie_parse_exception);

DECLARE_EXCEPTION(edhttp_exception, link_parse_exception);
DECLARE_EXCEPTION(edhttp_exception, link_parameter_exception);

DECLARE_EXCEPTION(edhttp_exception, client_server_error);
DECLARE_EXCEPTION(edhttp_exception, client_request_mixed_port_or_hostname);
DECLARE_EXCEPTION(edhttp_exception, client_no_addresses);
DECLARE_EXCEPTION(edhttp_exception, client_io_error);

DECLARE_EXCEPTION(edhttp_exception, mime_type_no_magic);



} // namespace edhttp
// vim: ts=4 sw=4 et
