#include "tokenize/tokenizer.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

void expect_tokens(std::string_view text, std::vector<std::string> expected) {
  EXPECT_EQ(recall::tokenize(text), expected);
}

}  // namespace

TEST(Tokenizer, SplitsCamelCase) {
  expect_tokens("pageTableEntry", {"page", "table", "entry"});
  expect_tokens("frameNumber", {"frame", "number"});
  expect_tokens("getURLValue", {"get", "url", "value"});
  expect_tokens("IOError", {"io", "error"});
}

TEST(Tokenizer, SplitsSnakeCaseAndPunctuation) {
  expect_tokens("page_table_entry", {"page", "table", "entry"});
  expect_tokens("Hello, world!", {"hello", "world"});
}

TEST(Tokenizer, KeepsCourseNumbersAndDropsSingleLetters) {
  expect_tokens("CS 2110", {"cs", "2110"});
  expect_tokens("A cat", {"cat"});
  expect_tokens("x", {});
  expect_tokens("ab", {"ab"});
  expect_tokens("", {});
}
