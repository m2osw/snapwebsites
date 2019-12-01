// Copyright (c) 2019  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapdatabase
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// Bloomfilter evaluation

#include    <iostream>
#include    <unordered_map>
#include    <vector>

constexpr   size_t          BLOOM_FILTER_SIZE = 1000000;
constexpr   size_t          DATA_SIZE =           100000000;
constexpr   size_t          HASH_COUNT =               13;

typedef uint32_t                                hash_t;
typedef std::vector<uint8_t>                    filter_t;
typedef std::unordered_map<std::string, int>    data_t;

filter_t                    g_bloom_filter[HASH_COUNT + 1];
data_t                      g_data;
hash_t                      g_filter_seeds[HASH_COUNT];



void init()
{
    for(size_t idx(0); idx <= HASH_COUNT; ++idx)
    {
        g_bloom_filter[idx].reserve(BLOOM_FILTER_SIZE);
    }

    for(size_t h(0); h < HASH_COUNT; ++h)
    {
        g_filter_seeds[h] = rand();
    }
}


// hash function taken from: https://github.com/ArashPartow/bloom
//
hash_t hash(uint8_t const * v, std::size_t size, hash_t hash)
{
    hash_t loop(0);

    for(; size >= 8; v += 8, size -= 8)
    {
        hash_t i1((v[0] << 24) + (v[1] << 16) + (v[2] << 8) + v[3]);
        hash_t i2((v[4] << 24) + (v[5] << 16) + (v[6] << 8) + v[7]);

        hash ^= (hash <<  7) ^  i1 * (hash >> 3) ^
             (~((hash << 11) + (i2 ^ (hash >> 5))));
    }

    if(size >= 4)
    {
        hash_t i((v[0] << 24) + (v[1] << 16) + (v[2] << 8) + v[3]);
        hash ^= (~((hash << 11) + (i ^ (hash >> 5))));
        ++loop;
        size -= 4;
        v += 4;
    }

    if(size >= 2)
    {
        hash_t i((v[0] << 8) + v[1]);
        if(loop != 0)
        {
            hash ^=    (hash <<  7) ^  i * (hash >> 3);
        }
        else
        {
            hash ^= (~((hash << 11) + (i ^ (hash >> 5))));
        }
        ++loop;
        //size -= 2; -- not necessary
        v += 2;
    }

    if(size > 0)
    {
        hash += (v[0] ^ (hash * 0xA5A5A5A5)) + loop;
    }

    return hash;
}


void fill()
{
    for(size_t idx(0); idx < DATA_SIZE; ++idx)
    {
        if(idx % 1000000 == 999999)
        {
            std::cerr << (100.0 * idx / DATA_SIZE) << "%\33[K\r";
        }

        int sz(rand() % 10 + 5);
        std::string key;
        for(int j(0); j < sz; ++j)
        {
            int c(rand() % (26 + 26 + 10));
            if(c >= 26 + 26)
            {
                c = '0' + (c - 26);
            }
            else if(c >= 26)
            {
                c += 'A';
            }
            else
            {
                c += 'a';
            }
            key += c;
        }
        int value(rand());
        g_data[key] = value;

        // add "bits" to bloom filter
        //
        for(size_t h(0); h < HASH_COUNT; ++h)
        {
            int pos(hash(reinterpret_cast<uint8_t const *>(key.data()), key.length(), g_filter_seeds[h]) % BLOOM_FILTER_SIZE);
            if(g_bloom_filter[h][pos] < 255)
            {
                ++g_bloom_filter[h][pos];               // seperate filter for each hash
            }
            if(g_bloom_filter[HASH_COUNT][pos] < 255)
            {
                ++g_bloom_filter[HASH_COUNT][pos];      // one filter for all the hashes
            }
        }
    }
    std::cerr << "\n";
}


void verify()
{
    int count(0);
    int errcnt(0);
    for(auto const & d : g_data)
    {
        if(count % 1000000 == 999999)
        {
            std::cerr << (100.0 * count / g_data.size()) << "%\33[K\r";
        }
        ++count;

        for(size_t h(0); h < HASH_COUNT; ++h)
        {
            int pos(hash(reinterpret_cast<uint8_t const *>(d.first.data()), d.first.length(), g_filter_seeds[h]) % BLOOM_FILTER_SIZE);

            if(g_bloom_filter[h][pos] == 0)
            {
                std::cerr << "error: found missing data!? (distinct filter)\33[K\n";
                ++errcnt;
            }
            if(g_bloom_filter[HASH_COUNT][pos] == 0)
            {
                std::cerr << "error: found missing data!? (common filter)\33[K\n";
                ++errcnt;
            }
        }
    }
    std::cerr << "\n";
}


void fill_ratio()
{
    int counts[HASH_COUNT + 1] = { 0 };
    for(size_t r(0); r < BLOOM_FILTER_SIZE; ++r)
    {
        for(size_t h(0); h <= HASH_COUNT; ++h)
        {
            if(g_bloom_filter[h][r] != 0)
            {
                ++counts[h];
            }
        }
    }
    for(size_t h(0); h <= HASH_COUNT; ++h)
    {
        std::cerr << "  "
                  << h
                  << ". "
                  << counts[h]
                  << " => "
                  << (100.0 * counts[h] / BLOOM_FILTER_SIZE)
                  << "%\n";
    }
}


void search()
{
    size_t false_positive_max(0);
    int separated_false_positive(0);
    int merged_false_positive(0);
    for(size_t idx(0); idx < DATA_SIZE * 10; ++idx)
    {
        if(idx % 100000 == 99999)
        {
            std::cerr << (100.0 * idx / false_positive_max)
                      << "% (mp="
                      << (static_cast<double>(merged_false_positive) / static_cast<double>(false_positive_max))
                      << ", sp="
                      << (static_cast<double>(separated_false_positive) / static_cast<double>(false_positive_max))
                      << ")\33[K\r";
        }

        int sz(rand() % 10 + 5);
        std::string key;
        for(int j(0); j < sz; ++j)
        {
            int c(rand() % (26 + 26 + 10));
            if(c >= 26 + 26)
            {
                c = '0' + (c - 26);
            }
            else if(c >= 26)
            {
                c += 'A';
            }
            else
            {
                c += 'a';
            }
            key += c;
        }

        if(g_data.find(key) == g_data.end())
        {
            // not present in source, check the bloom filters for
            // a false positive
            //
            ++false_positive_max;
            bool found_separated(true);
            bool found_merged(true);
            for(size_t h(0); h < HASH_COUNT && (found_separated || found_merged); ++h)
            {
                int pos(hash(reinterpret_cast<uint8_t const *>(key.data()), key.length(), g_filter_seeds[h]) % BLOOM_FILTER_SIZE);
                if(g_bloom_filter[h][pos] == 0)
                {
                    found_separated = false;
                }
                if(g_bloom_filter[HASH_COUNT][pos] == 0)
                {
                    found_merged = false;
                }
            }
            if(found_separated)
            {
                ++separated_false_positive;
            }
            if(found_merged)
            {
                ++merged_false_positive;
            }
        }
    }
    std::cerr << "\n";

    std::cout << "merged false positive: "
              << merged_false_positive
              << " (p="
              << (static_cast<double>(merged_false_positive) / static_cast<double>(false_positive_max))
              << ")\n";

    std::cout << "separated false positive: "
              << separated_false_positive
              << " (p="
              << (static_cast<double>(separated_false_positive) / static_cast<double>(false_positive_max))
              << ")\n";
}



int main()
{
    std::cerr << "info: init...\n";
    init();
    std::cerr << "info: fill...\n";
    fill();
    std::cerr << "info: verify...\n";
    verify();
    std::cerr << "info: fill ratio...\n";
    fill_ratio();
    std::cerr << "info: search...\n";
    search();

    return 0;
}

// vim: ts=4 sw=4 et
