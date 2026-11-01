#include "extract/extractor.hpp"

#include "extract/internal.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string_view>

namespace recall {
namespace {

bool looks_binary(std::string_view text) {
  const std::size_t n = std::min(text.size(), std::size_t{8192});
  const std::size_t nul = text.find('\0');
  return nul != std::string_view::npos && nul < n;
}

}  // namespace

Extraction extract(const CrawlEntry& entry) {
  if (entry.kind == FileKind::Pdf) {
    return extract_pdf(entry.path);
  }
  if (entry.kind == FileKind::Office) {
    return extract_office(entry.path);
  }

  Extraction result;

  std::ifstream input(entry.path, std::ios::binary);
  if (!input) {
    result.skip_reason = "unreadable";
    return result;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  result.text = buffer.str();
  if (looks_binary(result.text)) {
    result.text.clear();
    result.skip_reason = "binary";
    return result;
  }
  result.ok = true;
  return result;
}

}  // namespace recall
