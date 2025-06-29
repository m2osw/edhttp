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
#include    "edhttp/mime_type.h"

#include    "edhttp/exception.h"


// C
//
#include    <magic.h>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{

namespace
{
magic_t         g_magic = nullptr;
}


/** \brief Generate a MIME type from a buffer.
 *
 * This function determines the MIME type of a buffer (std::string)
 * using the magic library. The magic library is likely to
 * understand all the files that a user is to upload on a
 * website (within reason, of course).
 *
 * This function runs against the specified buffer (\p data) from
 * memory.
 *
 * The function returns the computed MIME type such as text/html or
 * image/png.
 *
 * \note
 * Compressed files get their type determined after decompression.
 *
 * \exception mime_type_no_magic
 * The function generates an exception if it cannot access the
 * magic library (the magic_open() function fails). If the magic
 * library is available but the type of the buffer can't be
 * determine, then a default MIME type is returned.
 *
 * \param[in] data  The buffer to be transformed in a MIME type.
 *
 * \return The MIME type of the input buffer.
 */
std::string get_mime_type(std::string const & data)
{
    if(g_magic == nullptr)
    {
        g_magic = magic_open(MAGIC_COMPRESS | MAGIC_MIME);
        if(g_magic == nullptr)
        {
            throw mime_type_no_magic("Magic MIME type cannot be opened (magic_open() failed)");
        }

        // load the default magic database
        //
        magic_load(g_magic, nullptr);
    }

    return magic_buffer(g_magic, data.data(), data.length());
}



} // namespace edhttp
// vim: ts=4 sw=4 et
