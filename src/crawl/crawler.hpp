#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace recall {

enum class FileKind { Text, Pdf, Office, Other };

struct CrawlEntry {
  std::filesystem::path path;
  FileKind kind = FileKind::Other;
};

struct CrawlOptions {
  std::uintmax_t max_file_bytes = 32ull * 1024ull * 1024ull;
  int max_depth = 64;
};

std::vector<CrawlEntry> crawl(const std::filesystem::path& root,
                              const CrawlOptions& options = {});

}  // namespace recall
