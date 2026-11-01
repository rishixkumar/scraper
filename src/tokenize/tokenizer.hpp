#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace recall {

// Lowercase, split on punctuation, and split camelCase / snake_case.
// "pageTableEntry" and "page_table_entry" both yield page, table, entry.
std::vector<std::string> tokenize(std::string_view text);

}  // namespace recall
