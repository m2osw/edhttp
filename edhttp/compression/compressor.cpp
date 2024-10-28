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
#include    "edhttp/compression/compressor.h"

#include    "edhttp/exception.h"
#include    "edhttp/token.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>


// C++
//
#include    <cstring>
#include    <ranges>


// C
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include    <zlib.h>
#pragma GCC diagnostic pop


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{


namespace
{



typedef std::map<std::string, compressor *>   compressor_map_t;

// IMPORTANT NOTE:
// This list only makes use of bare pointers for many good reasons.
// (i.e. all compressors are defined statitcally, not allocated)
// Do not try to change it! Thank you.
compressor_map_t * g_compressors;



} // no name namespace


/** \brief Special compressor name returned in some cases.
 *
 * When trying to compress a buffer, there are several reasons why the
 * compression may "fail". When that happens the result is the same
 * as the input, meaning that the data is not going to be compressed
 * at all.
 *
 * You should always verify whether the compression worked by testing
 * the compressor name variable on return (the second parameter in the
 * returned pair of result).
 */
char const * compressor::NO_COMPRESSION = "none";


/** \brief Register the compressor.
 *
 * Whenever you implement a compressor, the constructor must call
 * this constructor with the name of the compressor. Remember that
 * the get_name() function is NOT ready from this constructor which
 * is why we require you to specify the name in the constructor.
 *
 * This function registers the compressor in the internal list of
 * compressors and then returns.
 *
 * \note
 * The \p name of a compressor must be a valid token. Further, it
 * cannot be one of the special names such as NO_COMPRESSION.
 *
 * \param[in] name  The name of the compressor.
 */
compressor::compressor(char const * name)
{
    if(name == nullptr || *name == '\0')
    {
        throw invalid_token("the name of a compressor cannot be empty.");
    }

    std::string const n(name);
    if(n == compressor::NO_COMPRESSION)
    {
        throw incompatible("name \"" + n + "\" is not available as a compressor name.");
    }

    if(!is_token(n))
    {
        throw invalid_token("a compressor name (\"" + n + "\") must be a valid HTTP token.");
    }

    if(g_compressors == nullptr)
    {
        g_compressors = new compressor_map_t;
    }
    (*g_compressors)[name] = this;
}


/** \brief Clean up the compressor.
 *
 * This function unregisters the compressor.
 *
 * \note
 * It is expected that compressors get destroyed on exit only as they are
 * expected to be implemented and defined statically. However, for test
 * purposes, it can be practical to add & remove test compressors.
 */
compressor::~compressor()
{
    for(auto it(g_compressors->begin()); it != g_compressors->end(); ++it)
    {
        if(it->second == this)
        {
            g_compressors->erase(it);
            break;
        }
    }

    if(g_compressors->empty())
    {
        delete g_compressors;
        g_compressors = nullptr;
    }
}


/** \brief Return a list of names of the available compressors.
 *
 * In case you have more than one `Accept-Encoding` this list may end up being
 * helpful to know whether a compression is available or not.
 *
 * \return A list of names of all the available compressors.
 */
advgetopt::string_list_t compressor_list()
{
    if(g_compressors == nullptr)
    {
        return advgetopt::string_list_t(); // LCOV_EXCL_LINE
    }

    auto kv = std::views::keys(*g_compressors);
    return advgetopt::string_list_t{ kv.begin(), kv.end() };
}


/** \brief Return a pointer to the named compressor.
 *
 * This function checks whether a compressor named \p compressor_name
 * exists, if so it gets returned, otherwise a null pointer is returned.
 *
 * \param[in] compressor_name  The name of the concerned compressor.
 *
 * \return A pointer to a compressor_t object or nullptr.
 */
compressor * get_compressor(std::string const & compressor_name)
{
    if(g_compressors != nullptr)
    {
        auto it(g_compressors->find(compressor_name));
        if(it != g_compressors->end())
        {
            return it->second;
        }
    }

    return nullptr;
}


/** \brief Compress the \p input buffer.
 *
 * This function compresses the \p input buffer using the specified
 * \p compressor_names parameter and returns the result in a pair of
 * results: the compressed buffer and the compressor used to compress it.
 *
 * The \p compressor_names can be empty. In this case, all the available
 * compressors get tested and the best (i.e. smallest) result is returned.
 * On exit, the string in the pair is set with the name of the compressor
 * used.
 *
 * Otherwise, the \p compressor_names is expected to be set to a specific
 * list of compressor names. In this case, those specific compressors are
 * used and the best result returned. To compress with one specific
 * compressor, specify that one compressor's name only.
 *
 * \b IMPORTANT \b NOTE:
 *
 * There are several reasons why the compress() function may refuse
 * compressing your \p input buffer and return said \p input as is.
 * When this happens the name of the compressor in the resulting pair
 * is set to NO_COMPRESSION.
 *
 * \li The \p input is empty.
 * \li The \p input buffer is too small for that compressor.
 * \li The compression \p level is set to a value under 5%.
 * \li The buffer is way too large and allocating the compression buffer
 *     failed (this should never happen on a serious server!)
 * \li The named compressor does not exist.
 * \li The resulting compressed buffer is larger or equal to the size of
 *     the input buffer.
 *
 * Again, if the compression fails for whatever reason, the name of the
 * compressor in the resulting pair is set to NO_COMPRESSION. You have to
 * make sure to test that name on return to know whether it worked or not.
 *
 * \param[in] compressor_names  The name of the compressors to try.
 * \param[in] input  The input buffer which has to be compressed.
 * \param[in] level  The level of compression (0 to 100).
 * \param[in] text  Whether the input is text, set to false if not sure.
 *
 * \return A byte array with the compressed input data and a string with
 * the name of the compressor used or NO_COMPRESSION if still uncompressed.
 */
result_t compress(advgetopt::string_list_t const & compressor_names, buffer_t const & input, level_t level, bool text)
{
    // nothing to compress if empty or too small a level
    //
    level = std::clamp(level, static_cast<level_t>(0), static_cast<level_t>(100));
    if(input.size() > 0 && level >= 5)
    {
        buffer_t result_buffer;
        std::string result_name;

        // create a lambda of common code
        //
        auto select_best = [&result_buffer, &result_name, &input, level, text](compressor * c)
        {
            if(result_name.empty())
            {
                result_buffer = c->compress(input, level, text);
                if(result_buffer.size() < input.size())
                {
                    result_name = c->get_name();
                }
            }
            else
            {
                buffer_t test_buffer(c->compress(input, level, text));
                if(test_buffer.size() < result_buffer.size())
                {
                    result_buffer.swap(test_buffer);
                    result_name = c->get_name();
                }
            }
        };

        // if no names were specified, try with all available compressors
        //
        if(compressor_names.empty())
        {
            for(auto const & c : *g_compressors)
            {
                select_best(c.second);
            }
        }
        else
        {
            for(auto const & name : compressor_names)
            {
                auto it(g_compressors->find(name));
                if(it != g_compressors->end())
                {
                    select_best(it->second);
                }
            }
        }

        // no good result?
        //
        if(!result_name.empty())
        {
            return result_t(result_buffer, result_name);
        }
    }

    return result_t(input, compressor::NO_COMPRESSION);
}


/** \brief Decompress a buffer.
 *
 * This function checks the specified input buffer and decompresses it if
 * a compressor recognized its magic signature.
 *
 * If none of the compressors were compatible then the input is returned
 * as is. The compressor_name is set to NO_COMPRESSION in this case. This
 * does not really mean the buffer is not compressed, although it is likely
 * correct.
 *
 * \param[in] input  The input to decompress.
 *
 * \return The decompressed buffer (first) and the name of the compressor
 * (second).
 */
result_t decompress(buffer_t const & input)
{
    // nothing to decompress if empty
    //
    if(!input.empty())
    {
        for(auto const & c : *g_compressors)
        {
            if(c.second->compatible(input))
            {
                return result_t(c.second->decompress(input), c.second->get_name());
            }
        }
    }

    return result_t(input, compressor::NO_COMPRESSION);
}



} // namespace edhttp
// vim: ts=4 sw=4 et
