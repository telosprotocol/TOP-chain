#pragma once

#include <string>
#include <vector>

struct key_value_t {
  std::string m_key;
  std::string m_value;
};

std::string get_random_string(int size);
void key_value_pairs_generate(std::vector<key_value_t>& pairs, uint64_t number_of, size_t key_len, size_t value_len);