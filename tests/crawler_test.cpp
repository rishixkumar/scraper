#include "crawl/crawler.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

TEST(Crawler, SkipsIgnoredDirectoriesAndSecretFiles) {
  const std::filesystem::path root =
      std::filesystem::temp_directory_path() / "recall-crawl-test";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root / "notes");
  std::filesystem::create_directories(root / "node_modules");

  {
    std::ofstream note(root / "notes" / "page_tables.md");
    note << "page tables\n";
    std::ofstream hidden(root / "node_modules" / "package.md");
    hidden << "should not index\n";
    std::ofstream cookies(root / "cookies.txt");
    cookies << "session=secret\n";
    std::ofstream env(root / ".env");
    env << "TOKEN=abc\n";
  }

  const std::vector<recall::CrawlEntry> entries = recall::crawl(root);
  ASSERT_EQ(entries.size(), 1u);
  EXPECT_EQ(entries[0].path.filename(), "page_tables.md");
  EXPECT_EQ(entries[0].kind, recall::FileKind::Text);

  std::filesystem::remove_all(root);
}
