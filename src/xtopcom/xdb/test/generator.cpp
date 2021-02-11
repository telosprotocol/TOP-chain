#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

#include "generator.h"

/* Use this for string generation */
static const char alpha_numerics[]=
  "0123456789ABCDEFGHIJKLMNOPQRSTWXYZabcdefghijklmnopqrstuvwxyz";

#define alpha_numerics_size (sizeof(alpha_numerics) - 1)

static size_t get_alpha_num(void)
{
    return (size_t)random() % alpha_numerics_size;
}

std::string get_random_string( int size)
{
    char buffer[size];
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < size; ++i) {
        buffer[i]= alpha_numerics[get_alpha_num()];
    }
    return std::string(buffer, size);
}

void key_value_pairs_generate(std::vector<key_value_t>& pairs, uint64_t number_of, size_t key_len, size_t value_len)
{
    assert(key_len > 0 && value_len > 0);
    for (uint64_t x = 0; x < number_of; x++) {
        std::string key = get_random_string(key_len);
        std::string value = get_random_string(value_len);

        if (key.size() == 0 || value.size() == 0) {
            assert(0);
        }

        pairs.push_back({key, value});
    }
}
