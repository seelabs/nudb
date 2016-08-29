//
// Copyright (c) 2015-2016 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef NUDB_TEST_TEST_STORE_HPP
#define NUDB_TEST_TEST_STORE_HPP

#include <nudb/test/temp_dir.hpp>
#include <nudb/test/xor_shift_engine.hpp>
#include <nudb/create.hpp>
#include <nudb/native_file.hpp>
#include <nudb/store.hpp>
#include <nudb/xxhasher.hpp>

namespace nudb {
namespace test {

// Interface to facilitate tests
template<class File>
class basic_test_store
{
    using Hasher = xxhasher;

    temp_dir td_;
    std::size_t keySize_;
    std::size_t blockSize_;
    float loadFactor_;
    std::function<std::size_t(xor_shift_engine&)> sizef_;

    std::unique_ptr<std::uint8_t[]> p_;
    std::size_t capacity_ = 0;

public:
    struct item_type
    {
        std::uint8_t* key;
        std::uint8_t* data;
        std::size_t size;
    };

    path_type const dp;
    path_type const kp;
    path_type const lp;
    static std::uint64_t constexpr appnum = 1;
    static std::uint64_t constexpr salt = 42;
    basic_store<xxhasher, File> db;

    template<class... Args>
    basic_test_store(std::size_t keySize,
        std::size_t blockSize, float loadFactor,
            Args&&... args);

    ~basic_test_store();

    template<class SizeFunction>
    void
    size_function(SizeFunction const& sf)
    {
        sizef_ = sf;
    }

    item_type
    operator[](std::size_t i);

    void
    create(error_code& ec);

    void
    open(error_code& ec);

    void
    close(error_code& ec)
    {
        db.close(ec);
    }

private:
    template<class Generator>
    static
    void
    rngfill(
        void* dest, std::size_t size, Generator& g);
};

template<class File>
template<class... Args>
basic_test_store<File>::
basic_test_store(std::size_t keySize, std::size_t blockSize,
        float loadFactor, Args&&... args)
    : keySize_(keySize)
    , blockSize_(blockSize)
    , loadFactor_(loadFactor)
    , dp(td_.file("nudb.dat"))
    , kp(td_.file("nudb.key"))
    , lp(td_.file("nudb.log"))
{
    using dist =
        std::uniform_int_distribution<std::size_t>;
    size_function(dist{250, 2500});
}

template<class File>
basic_test_store<File>::
~basic_test_store()
{
    erase_file(dp);
    erase_file(kp);
    erase_file(lp);
}

template<class File>
auto
basic_test_store<File>::
operator[](std::size_t i) ->
    item_type
{
    xor_shift_engine g;
    g.seed(i + 1);
    item_type item;
    item.size = sizef_(g);
    auto const needed =
        keySize_ + item.size;
    if(capacity_ < needed)
    {
        p_.reset(new std::uint8_t[needed]);
        capacity_ = needed;
    }
    rngfill(p_.get(), needed, g);
    // put key last so we can get some unaligned
    // keys, this increases coverage of xxhash.
    item.data = p_.get();
    item.key = p_.get() + item.size;
    return item;
}

template<class File>
void
basic_test_store<File>::
create(error_code& ec)
{
    nudb::create<Hasher, File>(
        dp, kp, lp, appnum, salt,
            keySize_, blockSize_, loadFactor_, ec);
}

template<class File>
void
basic_test_store<File>::
open(error_code& ec)
{
    db.open(dp, kp, lp, 16 * 1024 * 1024, ec);
    if(ec)
        return;
    if(db.key_size() != keySize_)
        ec = error::invalid_key_size;
    else if(db.block_size() != blockSize_)
        ec = error::invalid_block_size;
}

template<class File>
template<class Generator>
void
basic_test_store<File>::
rngfill(
    void* dest, std::size_t size, Generator& g)
{
    using result_type =
        typename Generator::result_type;
    while(size >= sizeof(result_type))
    {
        auto const v = g();
        std::memcpy(dest, &v, sizeof(v));
        dest = reinterpret_cast<
            std::uint8_t*>(dest) + sizeof(v);
        size -= sizeof(v);
    }
    if(size > 0)
    {
        auto const v = g();
        std::memcpy(dest, &v, size);
    }
}

using test_store = basic_test_store<native_file>;

//------------------------------------------------------------------------------

template<class = void>
class Buffer_t
{
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
    std::unique_ptr<std::uint8_t[]> p_;

public:
    Buffer_t() = default;

    Buffer_t(Buffer_t&& other);

    Buffer_t(Buffer_t const& other);

    Buffer_t& operator=(Buffer_t&& other);

    Buffer_t& operator= (Buffer_t const& other);

    std::size_t
    size() const
    {
        return size_;
    }

    bool
    empty() const
    {
        return size_ == 0;
    }

    std::uint8_t*
    data()
    {
        return p_.get();
    }

    std::uint8_t const*
    data() const
    {
        return p_.get();
    }

    void
    clear();

    void
    shrink_to_fit();

    std::uint8_t*
    resize(std::size_t size);

    std::uint8_t*
    operator()(void const* data, std::size_t size);
};

template<class _>
Buffer_t<_>::
Buffer_t(Buffer_t&& other)
    : size_(other.size_)
    , capacity_(other.capacity_)
    , p_(std::move(other.p_))
{
    other.size_ = 0;
    other.capacity_ = 0;
}

template<class _>
Buffer_t<_>::
Buffer_t(Buffer_t const& other)
{
    std::memcpy(resize(other.size()),
        other.data(), other.size());
}

template<class _>
auto
Buffer_t<_>::
operator=(Buffer_t&& other) ->
    Buffer_t&
{
    size_ = other.size_;
    capacity_ = other.capacity_;
    p_ = std::move(other.p_);
    other.size_ = 0;
    other.capacity_ = 0;
    return *this;
}

template<class _>
auto
Buffer_t<_>::
operator= (Buffer_t const& other) ->
    Buffer_t&
{
    clear();
    std::memcpy(resize(other.size()),
        other.data(), other.size());
    return *this;
}

template<class _>
void
Buffer_t<_>::
clear()
{
    size_ = 0;
    capacity_ = 0;
    p_.reset();
}

template<class _>
void
Buffer_t<_>::
shrink_to_fit()
{
    if(empty())
        return;
    std::unique_ptr<std::uint8_t[]> p{
        new std::uint8_t[size_]};
    std::memcpy(p.get(), p_.get(), size_);
    std::swap(p, p_);
}

template<class _>
std::uint8_t*
Buffer_t<_>::
resize(std::size_t size)
{
    if(capacity_ < size)
    {
        capacity_ = size;
        p_.reset(
            new std::uint8_t[capacity_]);
    }
    size_ = size;
    return p_.get();
}

template<class _>
std::uint8_t*
Buffer_t<_>::
operator()(void const* data, std::size_t size) 
{
    resize(size);
    std::memcpy(p_.get(), data, size);
    return p_.get();
}

using Buffer = Buffer_t<>;

} // test
} // nudb

#endif

