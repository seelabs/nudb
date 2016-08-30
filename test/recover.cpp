//
// Copyright (c) 2015-2016 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Test that header file is self-contained
#include <nudb/recover.hpp>

#include "test_util.hpp"

#include <nudb/test/test_store.hpp>
#include <beast/unit_test/suite.hpp>
#include <cmath>
#include <cstring>
#include <memory>
#include <random>
#include <utility>

namespace nudb {
namespace test {

class basic_recover_test : public beast::unit_test::suite
{
public:
    void
    test_ok()
    {
        std::size_t const keySize = 8;
        std::size_t const blockSize = 256;
        float const loadFactor = 0.5f;
        
        error_code ec;
        test_store ts{keySize, blockSize, loadFactor};
        ts.create(ec);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        recover<xxhasher>(ts.dp, ts.kp, ts.lp, ec);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
    }

    // Creates and opens a database, performs a bunch
    // of inserts, then fetches all of them to make sure
    // they are there. Uses a fail_file that causes the n-th
    // I/O to fail, causing an exception.
    void
    do_work(std::size_t count, float load_factor,
        nudb::path_type const& path, fail_counter& c,
            error_code& ec)
    {
        auto const dp = path + ".dat";
        auto const kp = path + ".key";
        auto const lp = path + ".log";
        create<xxhasher>(dp, kp, lp,
            appnumValue, saltValue, sizeof(key_type),
                block_size(path), load_factor, ec);
        if(ec)
            return;
        fail_store db;
        db.open(dp, kp, lp, arenaAllocSize, ec, c);
        if(ec)
            return;
        expect(db.appnum() == appnumValue, "appnum");
        Sequence seq;
        for(std::size_t i = 0; i < count; ++i)
        {
            auto const v = seq[i];
            db.insert(&v.key, v.data, v.size, ec);
            expect(ec != error::key_exists, ec.message());
            if(ec)
                return;
        }
        Storage s;
        for(std::size_t i = 0; i < count; ++i)
        {
            auto const v = seq[i];
            db.fetch(&v.key, s, ec);
            expect(ec != error::key_not_found, ec.message());
            if(ec)
                return;
            if(! expect(s.size() == v.size))
                return;
            if(! expect(std::memcmp(s.get(),
                    v.data, v.size) == 0, "data"))
                return;
        }
        db.close(ec);
        if(ec)
            return;
        verify_info info;
        verify<xxhasher>(
            info, dp, kp, 0, no_progress{}, ec);
        {
            error_code ev;
            nudb::native_file::erase(dp, ev);
            expect(! ev, ev.message());
            ev = {};
            nudb::native_file::erase(kp, ev);
            expect(! ev, ev.message());
            ev = {};
            nudb::native_file::erase(lp, ev);
        }
        if(ec)
        {
            log << info;
            return;
        }
    }

    void
    do_recover(path_type const& path,
        fail_counter& c, error_code& ec)
    {
        auto const dp = path + ".dat";
        auto const kp = path + ".key";
        auto const lp = path + ".log";
        recover<xxhasher, fail_file<native_file>>(dp, kp, lp, ec, c);
        if(ec)
            return;
        verify_info info;
        verify<xxhasher>(
            info, dp, kp, 0, no_progress{}, ec);
        if(ec)
            return;
        error_code ec2;
        native_file::erase(dp, ec2);
        native_file::erase(kp, ec2);
        native_file::erase(lp, ec2);
    }

    void
    test_recover(float load_factor, std::size_t count)
    {
        testcase(std::to_string(count) + " inserts",
            beast::unit_test::abort_on_fail);
        temp_dir td;
        auto const path = td.path();
        for(std::size_t n = 1;; ++n)
        {
            {
                error_code ec;
                fail_counter c{n};
                do_work (count, load_factor, path, c, ec);
                if(! ec)
                    break;
                if(! expect(ec == test::test_error::failure, ec.message()))
                    return;
            }
            for(std::size_t m = 1;; ++m)
            {
                error_code ec;
                fail_counter c{m};
                do_recover (path, c, ec);
                if(! ec)
                    break;
                if(! expect(ec == test::test_error::failure, ec.message()))
                    return;
            }
        }
    }
};

class recover_test : public basic_recover_test
{
public:
    void
    run() override
    {
        float loadFactor = 0.55f;
        test_ok();
        for(auto const N : { 0, 10, 100, 1000 })
            test_recover(loadFactor, N);
    }
};

class recover_big_test : public basic_recover_test
{
public:
    void
    run() override
    {
        float loadFactor = 0.90f;
        test_recover(loadFactor, 10000);
        test_recover(loadFactor, 100000);
    }
};

BEAST_DEFINE_TESTSUITE(recover, test, nudb);
//BEAST_DEFINE_TESTSUITE_MANUAL(recover_big, test, nudb);

} // test
} // nudb
