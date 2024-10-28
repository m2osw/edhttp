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

#include    "edhttp/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/tokenize_string.h>


// C++
//
#include    <fstream>


// C
//
#include    <tar.h>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



class tar
    : public archiver
{
public:
                            tar();

    virtual char const *    get_name() const override;
    virtual void            append_file(archiver_archive & archive, archiver_file const & file) override;
    virtual bool            next_file(archiver_archive & archive, archiver_file & file) const override;
    virtual void            rewind(archiver_archive & archive) override;

private:
    void                    append_int(char * header, int value, unsigned int length, int base, char fill);
    std::uint32_t           read_int(char const * header, int length, int base) const;
    std::uint32_t           check_sum(unsigned char const * s) const;
};


tar::tar()
    : archiver("tar")
{
}


char const * tar::get_name() const
{
    return "tar";
}


/** \brief Create a tar header and copy the file content to the archive.
 *
 * This function creates a tar header (See definition in `/usr/include/tar.h`
 * for offset, length, and more).
 *
 * \param[in] archive  The archive being managed by this archiver.
 * \param[in] file  The archiver file to be added to this tarball.
 */
void tar::append_file(archiver_archive & archive, archiver_file const & file)
{
    // initialize header
    //
    std::vector<char> header(512);
    header[257] = 'u'; // magic "ustar "
    header[258] = 's';
    header[259] = 't';
    header[260] = 'a';
    header[261] = 'r';

    // POSIX tar version
    header[262] = '\0';
    header[263] = '0';
    header[264] = '0';

    // the GNU tar magic is "ustar  \n" -- however, that does not support the prefix
    // and that would support mtime and other times and that can include nano-seconds
    // but many of those entries require support of handling multiple blocks
    //header[262] = ' ';
    //header[263] = ' '; // version " \0"
    //header[264] = '\0'; -- already '\0'

    // filename
    //
    std::string str(file.get_filename());
    if(str.empty())
    {
        throw missing_name("a filename is required for an archive file.");
    }
    if(str.size() > 100)
    {
        std::vector<std::string> segments;
        snapdev::tokenize_string(segments, str, "/", true);
        str.clear();
        std::size_t pos(segments.size());
        for(;;)
        {
#ifdef _DEBUG
            if(pos == 0)
            {
                throw logic_error("pos cannot be reach zero in this loop."); // LCOV_EXCL_LINE
            }
#endif
            --pos;
            if(str.empty())
            {
                if(segments[pos].length() > 100)
                {
                    throw name_too_large("this file cannot be added to a tar archive at this point (filename too long).");
                }
                str = segments[pos];
            }
            else
            {
                if(segments[pos].length() + 1 + str.length() > 100)
                {
                    break;
                }
                str = segments[pos] + '/' + str;
            }
        }
        std::copy(str.data(), str.data() + str.size(), header.begin() + 0);

        // check the prefix
        //
        str = snapdev::join_strings(std::vector<std::string>(segments.begin(), segments.begin() + pos + 1), "/");
        if(str.length() > 155)
        {
            throw name_too_large("this prefix + file names cannot be added to a tar archive at this point (filename too long).");
        }
        std::copy(str.data(), str.data() + str.size(), header.begin() + 345);
    }
    else
    {
        std::copy(str.data(), str.data() + str.size(), header.begin() + 0);
    }

    // mode, uid, gid, mtime
    //
    append_int(&header[100], file.get_mode(), 7, 8, '0');
    append_int(&header[108], file.get_uid(),  7, 8, '0');
    append_int(&header[116], file.get_gid(),  7, 8, '0');

    // TODO: add support for the nanoseconds
    //
    append_int(&header[136], static_cast<int>(file.get_mtime().tv_sec), 11, 8, '0');

    // user/group names
    //
    str = file.get_user();
    if(str.length() > 32)
    {
        throw name_too_large("this file cannot be added to a tar archive at this point (user name too long).");
    }
    std::copy(str.data(), str.data() + str.size(), header.begin() + 265);

    str = file.get_group();
    if(str.length() > 32)
    {
        throw name_too_large("this file cannot be added to a tar archive at this point (group name too long).");
    }
    std::copy(str.data(), str.data() + str.size(), header.begin() + 297);

    // type, size
    //
    switch(file.get_type())
    {
    case file_type_t::FILE_TYPE_REGULAR:
        header[156] = REGTYPE; // regular (tar type)
        append_int(&header[124], file.get_data().size(), 11, 8, '0');
        break;

    case file_type_t::FILE_TYPE_DIRECTORY:
        header[156] = DIRTYPE; // directory (tar type)
        append_int(&header[124], 0, 11, 8, '0'); // size must be zero in ASCII
        break;

    //default: ... we could throw but here the compiler fails if we
    //             are missing some types
    }

    std::uint32_t const checksum(check_sum(reinterpret_cast<unsigned char const *>(&header[0])));
    if(checksum > 32767)
    {
        // no null in this case (rather rare with "normal" filenames)
        //
        append_int(&header[148], checksum, 7, 8, '0');
    }
    else
    {
        append_int(&header[148], checksum, 6, 8, '0');
    }
    header[155] = ' ';

    archive.get().insert(archive.get().end(), header.begin(), header.end());

    switch(file.get_type())
    {
    case file_type_t::FILE_TYPE_REGULAR:
        archive.get().insert(archive.get().end(), file.get_data().begin(), file.get_data().end());
        {
            // padding to next 512 bytes
            //
            std::uint32_t size(file.get_data().size());
            size &= 511;
            if(size != 0)
            {
                std::vector<char> pad(512 - size);
                archive.get().insert(archive.get().end(), pad.begin(), pad.end());
            }
        }
        break;

    default:
        // no data for that type
        break;

    }
}


/** \brief Read one file from \p archive.
 *
 * This function reads the next file from a tar archive. If the file pointer
 * is at the end of the file, no file is read and the function returns false.
 *
 * \exception incompatible
 * At the moment, the tar file can only contain directories and files. Any
 * other type raises this exception. This exception is also raised if the
 * file does not look like a tar file.
 *
 * \exception invalid_checksum
 * If the block being checked includes "ustar" but the checksum is invalid,
 * then this error is raised instead of the \c incompatible exception.
 *
 * \exception out_of_range
 * If the tarball header says we have a file of X bytes and the archive
 * buffer is not large enough to return X bytes of data, then this exception
 * is raised.
 *
 * \param[in,out] archive  The archive being read. It is not constant because
 * the file pointer inside the archive gets updated to the start of the next
 * file.
 * \param[out] file  The file object receiving the data from the next file.
 *
 * \return true if a file was read, false otherwise.
 */
bool tar::next_file(archiver_archive & archive, archiver_file & file) const
{
    // any more files?
    // (make sure there is at least a header for now)
    //
    if(archive.get_pos() + 512 > archive.get().size())
    {
        return false;
    }

    // read the header
    //
    std::span<char const> header(reinterpret_cast<char const *>(archive.get().data()) + archive.get_pos(), 512UL);

    // MAGIC
    //
    if(header[257] != 'u' || header[258] != 's' || header[259] != 't' || header[260] != 'a'
    || header[261] != 'r' || (header[262] != ' ' && header[262] != '\0'))
    {
        // if no MAGIC we may have empty blocks (which are legal at the
        // end of the file)
        //
        for(int i(0); i < 512; ++i)
        {
            if(header[i] != '\0')
            {
                throw incompatible(
                      "ustar magic code missing at position "
                    + std::to_string(archive.get_pos())
                    + ".");
            }
        }
        archive.advance_pos(512);

        // TODO: test all the following blocks as they all should be null
        //       (as you cannot find an empty block within the tarball)
        //
        return false;
    }
    // TODO: check the "version" since that defines the type of archive

    std::uint32_t const file_checksum(read_int(&header[148], 8, 8));
    std::uint32_t const comp_checksum(check_sum(reinterpret_cast<unsigned char const *>(&header[0])));
    if(file_checksum != comp_checksum)
    {
        throw invalid_checksum(
                "ustar checksum code ("
              + std::to_string(comp_checksum)
              + ") does not match what was expected ("
              + std::to_string(file_checksum)
              + ").");
    }

    std::string filename(&header[0], strnlen(&header[0], 100));
    // Prefix is only in POSIX archives (version "00")
    if(header[345] != '\0')
    {
        // this one has a prefix (long filename)
        //
        std::string const prefix(&header[345], strnlen(&header[345], 155));
        if(prefix.back() == '/')
        {
            // I think this case is considered a bug in a tarball...
            //
            filename = prefix + filename;
        }
        else
        {
            filename = prefix + '/' + filename;
        }
    }
    file.set_filename(filename);

    switch(header[156])
    {
    case AREGTYPE: // alternate code for regular file
    case REGTYPE:  // regular file
    case CONTTYPE: // continuous file
        file.set_type(file_type_t::FILE_TYPE_REGULAR);
        break;

    case DIRTYPE:
        file.set_type(file_type_t::FILE_TYPE_DIRECTORY);
        break;


    default:
        throw incompatible("file type in tarball not supported (we accept regular and directory files only).");

    }

    file.set_mode(read_int(&header[100],  8, 8));

    // TODO: see to add the nanonseconds (I think it's available?)
    //
    file.set_mtime(snapdev::timespec_ex(read_int(&header[136], 12, 8), 0));

    uid_t uid(read_int(&header[108],  8, 8));
    std::size_t const len_user(strnlen(&header[265], 32));
    file.set_user (std::string(&header[265], len_user), uid);

    gid_t gid(read_int(&header[116],  8, 8));
    std::size_t const len_group(strnlen(&header[297], 32));
    file.set_group(std::string(&header[297], len_group), gid);

    archive.advance_pos(512);

    if(file.get_type() == file_type_t::FILE_TYPE_REGULAR)
    {
        std::uint32_t const size(read_int(&header[124], 12, 8));
        int const total_size((size + 511) & -512);
        if(archive.get_pos() + total_size > archive.get().size())
        {
            throw out_of_range("file data not available (archive too small).");
        }
        std::span<buffer_t::value_type const> data(archive.get().data() + archive.get_pos(), size);
        file.set_data(data);

        archive.advance_pos(total_size);
    }

    return true;
}


void tar::rewind(archiver_archive & archive)
{
    archive.set_pos(0);
}


void tar::append_int(char * header, int value, unsigned int length, int base, char fill)
{
    // save the number (minimum 1 digit)
    //
    do
    {
        // base is 8 or 10 (although at the moment it's always 8)
        //
        --length;
        header[length] = static_cast<char>((value % base) + '0');
        value /= base;
    }
    while(length > 0 && value != 0);

    // fill the left side with 'fill'
    //
    while(length > 0)
    {
        --length;
        header[length] = fill;
    }
}


std::uint32_t tar::read_int(char const * header, int length, int base) const
{
    std::uint32_t result(0);
    for(; length > 0 && *header >= '0' && *header < '0' + base; --length, ++header)
    {
        result = result * base + (*header - '0');
    }
    return result;
}


std::uint32_t tar::check_sum(unsigned char const * s) const
{
    std::uint32_t result = 8 * ' '; // the checksum field

    // name + mode + uid + gid + size + mtime = 148 bytes
    //
    for(int n(148); n > 0; --n, ++s)
    {
        result += *s;
    }

    s += 8; // skip the checksum field

    // everything after the checksum is another 356 bytes
    //
    for(int n(356); n > 0; --n, ++s)
    {
        result += *s;
    }

    return result;
}


tar g_tar; // declare statically



} // namespace edhttp
// vim: ts=4 sw=4 et
