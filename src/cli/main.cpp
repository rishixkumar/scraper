#include "crawl/crawler.hpp"
#include "extract/extractor.hpp"
#include "tokenize/tokenizer.hpp"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  if (argc != 3 || std::string(argv[1]) != "dump") {
    std::cerr << "usage: recall dump <directory>\n";
    return 2;
  }

  const std::filesystem::path root{argv[2]};
  std::error_code ec;
  if (!std::filesystem::is_directory(root, ec)) {
    std::cerr << "not a directory: " << root << '\n';
    return 2;
  }

  for (const recall::CrawlEntry& entry : recall::crawl(root)) {
    std::cout << "# " << entry.path.generic_string() << '\n';
    const recall::Extraction extracted = recall::extract(entry);
    if (!extracted.ok) {
      std::cout << "! skipped: " << extracted.skip_reason << '\n';
      continue;
    }
    for (const std::string& token : recall::tokenize(extracted.text)) {
      std::cout << token << '\n';
    }
  }
  return 0;
}
