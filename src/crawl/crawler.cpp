#include "crawl/crawler.hpp"

#include "crawl/ignore.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace recall {
namespace {

FileKind classify(const std::filesystem::path& path) {
  std::string ext = path.extension().string();
  for (char& ch : ext) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  if (ext == ".pdf") {
    return FileKind::Pdf;
  }
  if (ext == ".pptx" || ext == ".docx") {
    return FileKind::Office;
  }
  static constexpr std::string_view kText[] = {
      ".txt",  ".md",    ".rst", ".c",   ".cc",    ".cpp",  ".cxx", ".h",
      ".hpp",  ".hh",    ".py",  ".js",  ".ts",    ".tsx",  ".jsx", ".java",
      ".go",   ".rs",    ".rb",  ".swift", ".m",   ".mm",   ".cs",  ".kt",
      ".scala", ".sh",   ".sql", ".json", ".yaml", ".yml",  ".toml", ".cmake"};
  for (const std::string_view candidate : kText) {
    if (ext == candidate) {
      return FileKind::Text;
    }
  }
  return FileKind::Other;
}

}  // namespace

std::vector<CrawlEntry> crawl(const std::filesystem::path& root,
                              const CrawlOptions& options) {
  std::vector<CrawlEntry> entries;
  std::error_code ec;
  auto it = std::filesystem::recursive_directory_iterator(
      root, std::filesystem::directory_options::skip_permission_denied, ec);
  const auto end = std::filesystem::recursive_directory_iterator();
  for (; it != end; it.increment(ec)) {
    if (ec) {
      ec.clear();
      continue;
    }
    if (it.depth() > options.max_depth) {
      it.disable_recursion_pending();
      continue;
    }
    if (it->is_symlink(ec)) {
      it.disable_recursion_pending();
      continue;
    }
    if (it->is_directory(ec)) {
      if (is_skipped_directory(it->path().filename().string())) {
        it.disable_recursion_pending();
      }
      continue;
    }
    if (!it->is_regular_file(ec)) {
      continue;
    }
    const std::filesystem::path& path = it->path();
    if (is_secret_file(path)) {
      continue;
    }
    std::error_code size_ec;
    const auto size = std::filesystem::file_size(path, size_ec);
    if (size_ec || size > options.max_file_bytes) {
      continue;
    }
    const FileKind kind = classify(path);
    if (kind == FileKind::Other) {
      continue;
    }
    entries.push_back(CrawlEntry{path, kind});
  }
  std::sort(entries.begin(), entries.end(),
            [](const CrawlEntry& a, const CrawlEntry& b) {
              return a.path < b.path;
            });
  return entries;
}

}  // namespace recall
