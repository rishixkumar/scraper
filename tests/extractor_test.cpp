#include "extract/extractor.hpp"
#include "tokenize/tokenizer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <string>

namespace {

std::filesystem::path sample(const char* relative) {
  return std::filesystem::path(RECALL_SOURCE_DIR) / relative;
}

recall::Extraction extract_file(const char* relative, recall::FileKind kind) {
  return recall::extract(recall::CrawlEntry{sample(relative), kind});
}

}  // namespace

TEST(Extractor, ReadsPdfTextLayer) {
  const recall::Extraction extracted =
      extract_file("samples/pdf/page_tables.pdf", recall::FileKind::Pdf);
  ASSERT_TRUE(extracted.ok) << extracted.skip_reason;
  const std::vector<std::string> tokens = recall::tokenize(extracted.text);
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "page"), tokens.end());
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "table"), tokens.end());
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "entry"), tokens.end());
}

TEST(Extractor, SkipsPdfWithoutTextLayer) {
  const recall::Extraction extracted =
      extract_file("samples/pdf/blank.pdf", recall::FileKind::Pdf);
  EXPECT_FALSE(extracted.ok);
  EXPECT_EQ(extracted.skip_reason, "no text layer");
}

TEST(Extractor, ReadsPptxSlideText) {
  const recall::Extraction extracted =
      extract_file("samples/slides/page_tables.pptx", recall::FileKind::Office);
  ASSERT_TRUE(extracted.ok) << extracted.skip_reason;
  const std::vector<std::string> tokens = recall::tokenize(extracted.text);
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "page"), tokens.end());
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "table"), tokens.end());
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "entry"), tokens.end());
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "frame"), tokens.end());
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "number"), tokens.end());
}

TEST(Extractor, ReadsDocxBodyText) {
  const recall::Extraction extracted =
      extract_file("samples/docs/page_tables.docx", recall::FileKind::Office);
  ASSERT_TRUE(extracted.ok) << extracted.skip_reason;
  const std::vector<std::string> tokens = recall::tokenize(extracted.text);
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "page"), tokens.end());
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "table"), tokens.end());
  EXPECT_NE(std::find(tokens.begin(), tokens.end(), "entry"), tokens.end());
}
