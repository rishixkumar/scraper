#include "tokenize/tokenizer.hpp"

#include <cctype>

namespace recall {
namespace {

void push_token(std::string token, std::vector<std::string>& out) {
  if (token.size() >= 2) {
    out.push_back(std::move(token));
  }
}

void split_camel(std::string_view raw, std::vector<std::string>& out) {
  std::string current;
  auto flush = [&]() {
    for (char& ch : current) {
      ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    push_token(std::move(current), out);
    current.clear();
  };

  for (std::size_t i = 0; i < raw.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(raw[i]);
    if (!current.empty() && std::isupper(ch)) {
      const unsigned char prev = static_cast<unsigned char>(raw[i - 1]);
      const bool next_lower =
          (i + 1 < raw.size()) &&
          std::islower(static_cast<unsigned char>(raw[i + 1]));
      const bool boundary =
          std::islower(prev) ||
          (next_lower && current.size() >= 2 &&
           std::isupper(static_cast<unsigned char>(current.back())));
      if (boundary) {
        flush();
      }
    }
    current.push_back(static_cast<char>(ch));
  }
  flush();
}

}  // namespace

std::vector<std::string> tokenize(std::string_view text) {
  std::vector<std::string> out;
  std::size_t i = 0;
  while (i < text.size()) {
    if (!std::isalnum(static_cast<unsigned char>(text[i]))) {
      ++i;
      continue;
    }
    const std::size_t start = i;
    while (i < text.size() &&
           std::isalnum(static_cast<unsigned char>(text[i]))) {
      ++i;
    }
    split_camel(text.substr(start, i - start), out);
  }
  return out;
}

}  // namespace recall
