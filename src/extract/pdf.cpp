#include "extract/internal.hpp"

#include <poppler-document.h>
#include <poppler-page.h>

#include <cctype>
#include <memory>
#include <string>
#include <string_view>

namespace recall {
namespace {

bool only_whitespace(std::string_view text) {
  for (const unsigned char ch : text) {
    if (!std::isspace(ch)) {
      return false;
    }
  }
  return true;
}

}  // namespace

Extraction extract_pdf(const std::filesystem::path& path) {
  Extraction result;
  const std::unique_ptr<poppler::document> document(
      poppler::document::load_from_file(path.string()));
  if (!document) {
    result.skip_reason = "unreadable pdf";
    return result;
  }
  if (document->is_locked()) {
    result.skip_reason = "encrypted pdf";
    return result;
  }

  std::string text;
  const int page_count = document->pages();
  for (int index = 0; index < page_count; ++index) {
    const std::unique_ptr<poppler::page> page(document->create_page(index));
    if (!page) {
      continue;
    }
    const poppler::byte_array utf8 = page->text().to_utf8();
    text.append(utf8.begin(), utf8.end());
    text.push_back('\n');
  }

  if (only_whitespace(text)) {
    result.skip_reason = "no text layer";
    return result;
  }
  result.ok = true;
  result.text = std::move(text);
  return result;
}

}  // namespace recall
