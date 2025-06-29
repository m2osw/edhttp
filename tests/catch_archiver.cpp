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

/** \file
 * \brief Verify the archiver classes.
 *
 * This file implements tests to verify that the archiver classes.
 */

// edhttp
//
#include    <edhttp/compression/archiver.h>

#include    <edhttp/exception.h>


// self
//
#include    "catch_main.h"


// snapdev
//
#include    <snapdev/string_replace_many.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C
//
#include    <tar.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{



std::uint32_t check_sum(unsigned char const * s)
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



} // no name namespace



CATCH_TEST_CASE("archiver_file", "[archiver]")
{
    CATCH_START_SECTION("archiver_file: verify archiver file defaults")
    {
        edhttp::archiver_file file;

        CATCH_REQUIRE(file.get_type() == edhttp::file_type_t::FILE_TYPE_REGULAR);
        CATCH_REQUIRE(file.get_data() == edhttp::buffer_t());
        CATCH_REQUIRE(file.get_filename() == std::string());
        CATCH_REQUIRE(file.get_user() == std::string());
        CATCH_REQUIRE(file.get_group() == std::string());
        CATCH_REQUIRE(file.get_uid() == 0);
        CATCH_REQUIRE(file.get_gid() == 0);
        CATCH_REQUIRE(file.get_mode() == 0);
        CATCH_REQUIRE(file.get_mtime() == snapdev::timespec_ex());
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver_file: verify set/get file metadata")
    {
        edhttp::archiver_file file;

        CATCH_REQUIRE(file.get_type() == edhttp::file_type_t::FILE_TYPE_REGULAR);
        file.set_type(edhttp::file_type_t::FILE_TYPE_DIRECTORY);
        CATCH_REQUIRE(file.get_type() == edhttp::file_type_t::FILE_TYPE_DIRECTORY);
        file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
        CATCH_REQUIRE(file.get_type() == edhttp::file_type_t::FILE_TYPE_REGULAR);

        CATCH_REQUIRE(file.get_data() == edhttp::buffer_t());
        auto const data(SNAP_CATCH2_NAMESPACE::random_buffer(1, 1024 * 16));
        file.set_data(data);
        CATCH_REQUIRE(file.get_data() == data);

        CATCH_REQUIRE(file.get_filename() == std::string());
        file.set_filename("/this/file/here");
        CATCH_REQUIRE(file.get_filename() == "/this/file/here");

        CATCH_REQUIRE(file.get_user() == std::string());
        CATCH_REQUIRE(file.get_uid() == 0);
        file.set_user("edhttp", 1'000);
        CATCH_REQUIRE(file.get_user() == "edhttp");
        CATCH_REQUIRE(file.get_uid() == 1'000);

        CATCH_REQUIRE(file.get_group() == std::string());
        CATCH_REQUIRE(file.get_gid() == 0);
        file.set_group("edhttp", 1'230);
        CATCH_REQUIRE(file.get_group() == "edhttp");
        CATCH_REQUIRE(file.get_gid() == 1'230);

        CATCH_REQUIRE(file.get_mode() == 0);
        file.set_mode(0750);
        CATCH_REQUIRE(file.get_mode() == 0750);

        snapdev::timespec_ex const now(snapdev::now());
        CATCH_REQUIRE(file.get_mtime() == snapdev::timespec_ex());
        file.set_mtime(now);
        CATCH_REQUIRE(file.get_mtime() == now);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("archiver_tar", "[archiver]")
{
    CATCH_START_SECTION("archiver_tar: verify tar archiver")
    {
        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        edhttp::archiver_archive archive;

        // add files to the tarball using randomly generated buffers
        // and metadata
        //
        constexpr std::size_t const FILE_COUNT = 15;
        std::vector<edhttp::buffer_t> file_data(FILE_COUNT);
        std::vector<snapdev::timespec_ex> file_mtime(FILE_COUNT);
        for(std::size_t i(0); i < FILE_COUNT; ++i)
        {
            file_data[i] = SNAP_CATCH2_NAMESPACE::random_buffer(1, 1024 * 64);
            file_mtime[i] = snapdev::now();

            edhttp::archiver_file file;
            file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
            file.set_data(file_data[i]);
            file.set_filename("this/file-" + std::to_string(i + 1));
            file.set_user("user-" + std::to_string(i + 1), 1000 + i);
            file.set_group("group-" + std::to_string(i + 1), 2000 + i);
            file.set_mode(0640);
            file.set_mtime(file_mtime[i]);

            tar->append_file(archive, file);
        }

        for(std::size_t i(0); i < FILE_COUNT; ++i)
        {
            edhttp::archiver_file file;
            CATCH_REQUIRE(tar->next_file(archive, file));

            CATCH_REQUIRE(file.get_type() == edhttp::file_type_t::FILE_TYPE_REGULAR);
            CATCH_REQUIRE(file.get_data() == file_data[i]);
            CATCH_REQUIRE(file.get_filename() == "this/file-" + std::to_string(i + 1));
            CATCH_REQUIRE(file.get_user() == "user-" + std::to_string(i + 1));
            CATCH_REQUIRE(file.get_uid() == 1000 + i);
            CATCH_REQUIRE(file.get_group() == "group-" + std::to_string(i + 1));
            CATCH_REQUIRE(file.get_gid() == 2000 + i);
            CATCH_REQUIRE(file.get_mode() == 0640);
            CATCH_REQUIRE(file.get_mtime().tv_sec == file_mtime[i].tv_sec);
            CATCH_REQUIRE(file.get_mtime().tv_nsec == 0);
        }

        {
            edhttp::archiver_file file;
            CATCH_REQUIRE_FALSE(tar->next_file(archive, file));
            CATCH_REQUIRE_FALSE(tar->next_file(archive, file));
            CATCH_REQUIRE_FALSE(tar->next_file(archive, file));
            CATCH_REQUIRE_FALSE(tar->next_file(archive, file));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver_tar: verify tar archiver with long filenames")
    {
        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        edhttp::archiver_archive archive;

        // add files to the tarball using randomly generated buffers
        // and metadata
        //
        constexpr std::size_t const FILE_COUNT = 15;
        std::size_t file_count(0);
        std::vector<edhttp::buffer_t> file_data;
        std::vector<snapdev::timespec_ex> file_mtime;
        std::vector<std::string> file_filename;
        for(std::size_t i(0); i < FILE_COUNT; ++i)
        {
            std::string filename;
            std::size_t max_segments(rand() % 10 + 5);
            std::vector<std::string> segments(max_segments);
            for(;;)
            {
                segments.clear();
                for(std::size_t j(0); j < max_segments; ++j)
                {
                    segments.push_back(snapdev::string_replace_many(SNAP_CATCH2_NAMESPACE::random_string(
                                      1
                                    , 24
                                    , SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_ASCII)
                                , {{"/", "-"}}));
                }
                segments[max_segments - 1] += '_';
                segments[max_segments - 1] += std::to_string(file_count + 1);

                std::size_t filename_len(0);
                std::size_t prefix_len(0);
                for(std::size_t j(max_segments); j > 0; )
                {
                    --j;
                    if(prefix_len == 0
                    && filename_len + segments[j].length() <= 100)
                    {
                        if(filename_len != 0)
                        {
                            ++filename_len; // for the '/'
                        }
                        filename_len += segments[j].length();
                    }
                    else
                    {
                        if(prefix_len != 0)
                        {
                            ++prefix_len; // for the '/'
                        }
                        prefix_len += segments[j].length();
                    }
                }

                if(prefix_len <= 155)
                {
                    filename = snapdev::join_strings(segments, "/");
                    break;
                }

                // prefix too long, try again
            }

            // do we have folders?
            //
            if(max_segments > 1)
            {
                for(std::size_t j(1); j < max_segments; ++j)
                {
                    std::string const dir_name(snapdev::join_strings(std::vector<std::string>(segments.begin(), segments.begin() + j), "/"));

                    // make the data buffer very small because we don't
                    // need it for directories (it gets ignored)
                    //
                    file_data.push_back(SNAP_CATCH2_NAMESPACE::random_buffer(0, 1024));
                    file_mtime.push_back(snapdev::now());
                    file_filename.push_back(dir_name);

                    edhttp::archiver_file dir;
                    dir.set_type(edhttp::file_type_t::FILE_TYPE_DIRECTORY);
                    dir.set_data(file_data[file_count]); // this must be ignored for directories
                    dir.set_filename(dir_name);
                    dir.set_user("user-" + std::to_string(file_count + 1), 1000 + file_count);
                    dir.set_group("group-" + std::to_string(file_count + 1), 2000 + file_count);
                    dir.set_mode(0750);
                    dir.set_mtime(file_mtime[file_count]);

                    tar->append_file(archive, dir);
                    ++file_count;
                }
            }

            {
                file_data.push_back(SNAP_CATCH2_NAMESPACE::random_buffer(1, 1024 * 64));
                file_mtime.push_back(snapdev::now());
                file_filename.push_back(filename);

                edhttp::archiver_file file;
                file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
                file.set_data(file_data[file_count]);
                file.set_filename(filename);
                file.set_user("user-" + std::to_string(file_count + 1), 1000 + file_count);
                file.set_group("group-" + std::to_string(file_count + 1), 2000 + file_count);
                file.set_mode(0640);
                file.set_mtime(file_mtime[file_count]);

                std::size_t const header_pos(archive.get().size());

                tar->append_file(archive, file);
                ++file_count;

                // once in a while, add a '/' at the end of the prefix
                //
                if(filename.length() > 100 && (rand() & 1) != 0)
                {
                    unsigned char * header(archive.get().data() + header_pos);
                    std::size_t const len(strlen(reinterpret_cast<char const *>(header) + 345));
                    if(len < 155)
                    {
                        header[345 + len] = '/';

                        // if this happens with a regular file, also test
                        // with CONTTYPE instead
                        //
                        if(header[156] == REGTYPE)
                        {
                            header[156] == CONTTYPE;
                        }

                        // adjust the checksum
                        //
                        std::uint32_t const checksum(check_sum(header));
                        std::stringstream ss;
                        ss << std::oct
                            << std::setw(6)
                            << std::setfill('0')
                            << checksum;
                        strncpy(reinterpret_cast<char *>(header) + 148, ss.str().c_str(), 6);
                    }
                }
            }
        }

#if 0
edhttp::buffer_t data(archive.get());
std::ofstream out("data.tar");
out.write(reinterpret_cast<char const *>(data.data()), data.size());
#endif

        for(int a(0); a < 2; ++a)
        {
            for(std::size_t i(0); i < file_count; ++i)
            {
                edhttp::archiver_file file;
                CATCH_REQUIRE(tar->next_file(archive, file));

                if(file.get_type() == edhttp::file_type_t::FILE_TYPE_DIRECTORY)
                {
                    CATCH_REQUIRE(file.get_data().size() == 0);
                    CATCH_REQUIRE(file.get_data() == edhttp::buffer_t());
                    CATCH_REQUIRE(file.get_filename() == file_filename[i]);
                    CATCH_REQUIRE(file.get_user() == "user-" + std::to_string(i + 1));
                    CATCH_REQUIRE(file.get_uid() == 1000 + i);
                    CATCH_REQUIRE(file.get_group() == "group-" + std::to_string(i + 1));
                    CATCH_REQUIRE(file.get_gid() == 2000 + i);
                    CATCH_REQUIRE(file.get_mode() == 0750);
                    CATCH_REQUIRE(file.get_mtime().tv_sec == file_mtime[i].tv_sec);
                    CATCH_REQUIRE(file.get_mtime().tv_nsec == 0);
                }
                else
                {
                    CATCH_REQUIRE(file.get_type() == edhttp::file_type_t::FILE_TYPE_REGULAR);
                    CATCH_REQUIRE(file.get_data().size() == file_data[i].size());
                    CATCH_REQUIRE(file.get_data() == file_data[i]);
                    CATCH_REQUIRE(file.get_filename() == file_filename[i]);
                    CATCH_REQUIRE(file.get_user() == "user-" + std::to_string(i + 1));
                    CATCH_REQUIRE(file.get_uid() == 1000 + i);
                    CATCH_REQUIRE(file.get_group() == "group-" + std::to_string(i + 1));
                    CATCH_REQUIRE(file.get_gid() == 2000 + i);
                    CATCH_REQUIRE(file.get_mode() == 0640);
                    CATCH_REQUIRE(file.get_mtime().tv_sec == file_mtime[i].tv_sec);
                    CATCH_REQUIRE(file.get_mtime().tv_nsec == 0);
                }
            }

            // once we reached the end, we get `false`
            {
                edhttp::archiver_file file;
                CATCH_REQUIRE_FALSE(tar->next_file(archive, file));
            }

            // append an empty header, which is legal at the end of tar files
            edhttp::buffer_t zeroes(512);
            archive.get().insert(archive.get().end(), zeroes.begin(), zeroes.end());

            // we can repeat the verification by rewinding
            //
            tar->rewind(archive);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver_tar: verify large checksum")
    {
        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        edhttp::archiver_archive archive;

        // create a filename that will make the checksum go over 32,767
        //
        // TODO: see whether a defined UTF-8 character can be used instead
        //       of 0x10FFFD which is considered valid but is not (yet)
        //       defined
        //
        std::string filename;
        std::string const last_utf8{  // 0x10FFFD
            static_cast<char>(0xF4),
            static_cast<char>(0x8F),
            static_cast<char>(0xBF),
            static_cast<char>(0xBD),
        };
        for(int i(0); i < 155 / 4; ++i)
        {
            filename += last_utf8;
        }
        //filename += std::string(155, '\xFF'); // prefix
        filename += '/';
        //filename += std::string(100, '\xFF'); // filename
        for(int i(0); i < 100 / 4; ++i)
        {
            filename += last_utf8;
        }

        edhttp::buffer_t const data(SNAP_CATCH2_NAMESPACE::random_buffer(1, 25));
        snapdev::timespec_ex const now(snapdev::now());

        {
            edhttp::archiver_file file;
            file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
            file.set_data(data);
            file.set_filename(filename);
            file.set_user("edhttp", 1000);
            file.set_group("edhttp", 1000);
            file.set_mode(0444);
            file.set_mtime(now);
            tar->append_file(archive, file);
        }

        {
            edhttp::archiver_file file;
            CATCH_REQUIRE(tar->next_file(archive, file));

            CATCH_REQUIRE(file.get_type() == edhttp::file_type_t::FILE_TYPE_REGULAR);
            CATCH_REQUIRE(file.get_data().size() == data.size());
            CATCH_REQUIRE(file.get_data() == data);
            CATCH_REQUIRE(file.get_filename() == filename);
            CATCH_REQUIRE(file.get_user() == "edhttp");
            CATCH_REQUIRE(file.get_uid() == 1000);
            CATCH_REQUIRE(file.get_group() == "edhttp");
            CATCH_REQUIRE(file.get_gid() == 1000);
            CATCH_REQUIRE(file.get_mode() == 0444);
            CATCH_REQUIRE(file.get_mtime().tv_sec == now.tv_sec);
            CATCH_REQUIRE(file.get_mtime().tv_nsec == 0);
        }

        {
            edhttp::archiver_file file;
            CATCH_REQUIRE_FALSE(tar->next_file(archive, file));
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("archiver", "[archiver]")
{
    CATCH_START_SECTION("archiver: verify list of archivers in our library")
    {
        advgetopt::string_list_t const list(edhttp::archiver_list());

        CATCH_REQUIRE(list.size() == 1);

        // internally it's in a map so it remains sorted
        //
        CATCH_REQUIRE(list[0] == "tar");

        auto const input(SNAP_CATCH2_NAMESPACE::random_buffer(1024, 1024 * 16));
        for(auto const & name : list)
        {
            edhttp::archiver * a(edhttp::get_archiver(name));
            CATCH_REQUIRE(a != nullptr);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver: verify the \"unknown\" archiver does not exist")
    {
        edhttp::archiver * unknown(edhttp::get_archiver("unknown"));
        CATCH_REQUIRE(unknown == nullptr);
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("archiver_error", "[archiver][error]")
{
    CATCH_START_SECTION("archiver_error: tar filename missing")
    {
        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        edhttp::buffer_t file_data(SNAP_CATCH2_NAMESPACE::random_buffer(1, 1024));

        edhttp::archiver_file file;
        file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
        file.set_data(file_data);
        // file.set_filename(...); -- this parameter is mandatory
        file.set_user("edhttp", 1000);
        file.set_group("www-data", 128);
        file.set_mode(0644);
        file.set_mtime(snapdev::now());

        edhttp::archiver_archive archive;

        CATCH_REQUIRE_THROWS_MATCHES(
                  tar->append_file(archive, file)
                , edhttp::missing_name
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: a filename is required for an archive file."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver_error: tar filename too long")
    {
        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        // generate a random filename (string)
        //
        std::string filename(SNAP_CATCH2_NAMESPACE::random_string(
                  101
                , 1024
                , SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_ASCII));
        filename = snapdev::string_replace_many(
              filename
            , {{"/", "-"}});

        edhttp::buffer_t file_data(SNAP_CATCH2_NAMESPACE::random_buffer(1, 1024));

        edhttp::archiver_file file;
        file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
        file.set_data(file_data);
        file.set_filename(filename);
        file.set_user("edhttp", 1000);
        file.set_group("www-data", 128);
        file.set_mode(0644);
        file.set_mtime(snapdev::now());

        edhttp::archiver_archive archive;

        CATCH_REQUIRE_THROWS_MATCHES(
                  tar->append_file(archive, file)
                , edhttp::name_too_large
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: this file cannot be added to a tar archive at this point (filename too long)."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver_error: tar prefix too long")
    {
        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        // generate a random filename of the perfect size +1
        //
        std::string filename(SNAP_CATCH2_NAMESPACE::random_string(
                  100 + 1 + 155 + 1
                , 100 + 1 + 155 + 1
                , SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_ASCII));

        // remove any slashes
        //
        filename = snapdev::string_replace_many(
              filename
            , {{"/", "-"}});

        // place the slash at the exact right position so the filename
        // is exactly 100 characters
        //
        filename[156] = '/';

        edhttp::buffer_t file_data(SNAP_CATCH2_NAMESPACE::random_buffer(1, 1024));

        edhttp::archiver_file file;
        file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
        file.set_data(file_data);
        file.set_filename(filename);
        file.set_user("edhttp", 1000);
        file.set_group("www-data", 128);
        file.set_mode(0644);
        file.set_mtime(snapdev::now());

        edhttp::archiver_archive archive;

        CATCH_REQUIRE_THROWS_MATCHES(
                  tar->append_file(archive, file)
                , edhttp::name_too_large
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: this prefix + file names cannot be added to a tar archive at this point (filename too long)."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver_error: tar user name too long")
    {
        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        // generate a random user_name (string)
        //
        std::string const user_name(SNAP_CATCH2_NAMESPACE::random_string(
                  33
                , 100
                , SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_ASCII));

        edhttp::buffer_t file_data(SNAP_CATCH2_NAMESPACE::random_buffer(1, 1024));

        edhttp::archiver_file file;
        file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
        file.set_data(file_data);
        file.set_filename("long-user-name.pdf");
        file.set_user(user_name, 1000);
        file.set_group("www-data", 128);
        file.set_mode(0644);
        file.set_mtime(snapdev::now());

        edhttp::archiver_archive archive;

        CATCH_REQUIRE_THROWS_MATCHES(
                  tar->append_file(archive, file)
                , edhttp::name_too_large
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: this file cannot be added to a tar archive at this point (user name too long)."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver_error: tar group name too long")
    {
        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        // generate a random group_name (string)
        //
        std::string const group_name(SNAP_CATCH2_NAMESPACE::random_string(
                  33
                , 100
                , SNAP_CATCH2_NAMESPACE::character_t::CHARACTER_ASCII));

        edhttp::buffer_t file_data(SNAP_CATCH2_NAMESPACE::random_buffer(1, 1024));

        edhttp::archiver_file file;
        file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
        file.set_data(file_data);
        file.set_filename("long-user-name.pdf");
        file.set_user("edhttp", 1000);
        file.set_group(group_name, 128);
        file.set_mode(0644);
        file.set_mtime(snapdev::now());

        edhttp::archiver_archive archive;

        CATCH_REQUIRE_THROWS_MATCHES(
                  tar->append_file(archive, file)
                , edhttp::name_too_large
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: this file cannot be added to a tar archive at this point (group name too long)."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver_error: tar file data missing")
    {
        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        edhttp::archiver_archive archive;

        // generate a random file
        //
        edhttp::buffer_t file_data(SNAP_CATCH2_NAMESPACE::random_buffer(1024, 1024));

        snapdev::timespec_ex const file_mtime(snapdev::now());

        {
            edhttp::archiver_file file;
            file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
            file.set_data(file_data);
            file.set_filename("document.pdf");
            file.set_user("edhttp", 1000);
            file.set_group("edhttp", 1001);
            file.set_mode(0664);
            file.set_mtime(file_mtime);

            tar->append_file(archive, file);
        }

        // re-reading this one works as expected at first
        {
            edhttp::archiver_file file;
            CATCH_REQUIRE(tar->next_file(archive, file));

            CATCH_REQUIRE(file.get_type() == edhttp::file_type_t::FILE_TYPE_REGULAR);
            CATCH_REQUIRE(file.get_data() == file_data);
            CATCH_REQUIRE(file.get_filename() == "document.pdf");
            CATCH_REQUIRE(file.get_user() == "edhttp");
            CATCH_REQUIRE(file.get_uid() == 1000);
            CATCH_REQUIRE(file.get_group() == "edhttp");
            CATCH_REQUIRE(file.get_gid() == 1001);
            CATCH_REQUIRE(file.get_mode() == 0664);
            CATCH_REQUIRE(file.get_mtime().tv_sec == file_mtime.tv_sec);
            CATCH_REQUIRE(file.get_mtime().tv_nsec == 0);
        }

        // now try again, but first let's destroy half the file
        {
            tar->rewind(archive);

            archive.get().resize(1024); // block is 512, file was 1024, now block is 512 and file is 512

            edhttp::archiver_file file;
            CATCH_REQUIRE_THROWS_MATCHES(
                      tar->next_file(archive, file)
                    , edhttp::out_of_range
                    , Catch::Matchers::ExceptionMessage(
                              "out_of_range: file data not available (archive too small)."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver_error: tar file invalid checksum & file type")
    {
        char const unsupported_types[] =
        {
            //REGTYPE,
            //AREGTYPE,
            LNKTYPE,
            SYMTYPE,
            CHRTYPE,
            BLKTYPE,
            //DIRTYPE,
            FIFOTYPE,
            //CONTTYPE,
        };

        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        edhttp::archiver_archive archive;

        // generate a random file
        //
        edhttp::buffer_t file_data(SNAP_CATCH2_NAMESPACE::random_buffer(1024, 1024));

        snapdev::timespec_ex const file_mtime(snapdev::now());

        {
            edhttp::archiver_file file;
            file.set_type(edhttp::file_type_t::FILE_TYPE_REGULAR);
            file.set_data(file_data);
            file.set_filename("document.pdf");
            file.set_user("edhttp", 1000);
            file.set_group("edhttp", 1001);
            file.set_mode(0664);
            file.set_mtime(file_mtime);

            tar->append_file(archive, file);
        }

        // re-reading this one works as expected at first
        {
            edhttp::archiver_file file;
            CATCH_REQUIRE(tar->next_file(archive, file));

            CATCH_REQUIRE(file.get_type() == edhttp::file_type_t::FILE_TYPE_REGULAR);
            CATCH_REQUIRE(file.get_data() == file_data);
            CATCH_REQUIRE(file.get_filename() == "document.pdf");
            CATCH_REQUIRE(file.get_user() == "edhttp");
            CATCH_REQUIRE(file.get_uid() == 1000);
            CATCH_REQUIRE(file.get_group() == "edhttp");
            CATCH_REQUIRE(file.get_gid() == 1001);
            CATCH_REQUIRE(file.get_mode() == 0664);
            CATCH_REQUIRE(file.get_mtime().tv_sec == file_mtime.tv_sec);
            CATCH_REQUIRE(file.get_mtime().tv_nsec == 0);
        }

        // now try again, but first let's destroy half the file
        for(auto const c : unsupported_types)
        {
            tar->rewind(archive);

            archive.get()[156] = c;

            std::uint32_t const new_checksum(check_sum(archive.get().data()));

            char buf[8];
            memcpy(buf, reinterpret_cast<char *>(archive.get().data()) + 148, 8);
            long const file_checksum(strtol(buf, nullptr, 8));

            // try "too soon" and the checksum breaks
            //
            edhttp::archiver_file file;
            CATCH_REQUIRE_THROWS_MATCHES(
                      tar->next_file(archive, file)
                    , edhttp::invalid_checksum
                    , Catch::Matchers::ExceptionMessage(
                              "edhttp_exception: ustar checksum code ("
                            + std::to_string(new_checksum)
                            + ") does not match what was expected ("
                            + std::to_string(file_checksum)
                            + ")."));

            // fix the checksum
            //
            std::stringstream ss;
            ss << std::oct
                << std::setw(6)
                << std::setfill('0')
                << new_checksum;
            strncpy(reinterpret_cast<char *>(archive.get().data()) + 148, ss.str().c_str(), 6);

            // next attempt bypasses the checksum check, but now fails on
            // the file type which we do not support
            //
            CATCH_REQUIRE_THROWS_MATCHES(
                      tar->next_file(archive, file)
                    , edhttp::incompatible
                    , Catch::Matchers::ExceptionMessage(
                              "edhttp_exception: file type in tarball not supported (we accept regular and directory files only)."));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("archiver_error: tar file invalid checksum & file type")
    {
        edhttp::archiver * tar(edhttp::get_archiver("tar"));
        CATCH_REQUIRE(tar != nullptr);
        CATCH_REQUIRE(strcmp(tar->get_name(), "tar") == 0);

        edhttp::archiver_archive archive;

        // generate a random header block
        //
        edhttp::buffer_t header;
        for(;;)
        {
            header = SNAP_CATCH2_NAMESPACE::random_buffer(512, 512);
            if(header[257] != 'u' || header[258] != 's' || header[259] != 't' || header[260] != 'a'
            || header[261] != 'r' || (header[262] != ' ' && header[262] != '\0'))
            {
                break;
            }
            // try again in the extremely remote chance the randomizer
            // generated the "ustar( |\0)" file magic
        }

        archive.set(header);
        CATCH_REQUIRE(const_cast<edhttp::archiver_archive const &>(archive).get() == header);

        edhttp::archiver_file file;
        CATCH_REQUIRE_THROWS_MATCHES(
                  tar->next_file(archive, file)
                , edhttp::incompatible
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: ustar magic code missing at position 0."));
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
