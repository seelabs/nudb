//
// Copyright (c) 2015-2016 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "test_util.hpp"
#include <cmath>
#include <iomanip>
#include <memory>
#include <random>
#include <utility>

// Basic, single threaded test that verifies the
// correct operation of the store. Load factor is
// set high to ensure that spill records are created,
// exercised, and split.
//
void
do_test (std::size_t N,
    std::size_t block_size, float load_factor)
{
    using api =
        nudb::api<nudb::test::xxhasher, nudb::identity, nudb::native_file>;

    nudb::test::temp_dir td;

    auto const dp = td.file ("nudb.dat");
    auto const kp = td.file ("nudb.key");
    auto const lp = td.file ("nudb.log");
    nudb::test::Sequence seq;
    api::store db;
    try
    {
        api::create (dp, kp, lp, nudb::test::appnum,
            nudb::test::salt, sizeof(nudb::test::key_type), block_size,
                load_factor);
        db.open(dp, kp, lp,
            nudb::test::arena_alloc_size);
        nudb::test::Storage s;
        // insert
        for(std::size_t i = 0; i < N; ++i)
        {
            auto const v = seq[i];
            db.insert(
                &v.key, v.data, v.size);
        }
        // fetch
        for(std::size_t i = 0; i < N; ++i)
        {
            auto const v = seq[i];
            bool const found = db.fetch (&v.key, s);
        }
        // insert duplicates
        for(std::size_t i = 0; i < N; ++i)
        {
            auto const v = seq[i];
            db.insert(&v.key,
                v.data, v.size);
        }
        // insert/fetch
        for(std::size_t i = 0; i < N; ++i)
        {
            auto v = seq[i];
            bool const found = db.fetch (&v.key, s);
            v = seq[i + N];
            db.insert(&v.key, v.data, v.size);
        }
        db.close();
        //auto const stats = test_api::verify(dp, kp);
        auto const stats = nudb::verify<api::hash_type>(
            dp, kp, 1 * 1024 * 1024);
        nudb::test::print(std::cout, stats);
    }
    catch (nudb::store_error const& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
    }
    api::file_type::erase(dp);
    api::file_type::erase(kp);
    api::file_type::erase(lp);
}

int main()
{
    std::cout << "store_test:" << std::endl;
    enum
    {
        N =             50000,
        block_size =    256
    };

    float const load_factor = 0.95f;

    do_test (N, block_size, load_factor);
}
