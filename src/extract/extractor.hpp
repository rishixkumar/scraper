#pragma once

#include "crawl/crawler.hpp"

#include <string>

namespace recall {

struct Extraction {
  bool ok = false;
  std::string text;
  std::string skip_reason;
};

Extraction extract(const CrawlEntry& entry);

}  // namespace recall
