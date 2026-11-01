#include "extract/internal.hpp"

#include <pugixml.hpp>
#include <zip.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace recall {
namespace {

constexpr zip_uint64_t kMaxEntryBytes = 32ull * 1024ull * 1024ull;

class ZipArchive {
 public:
  explicit ZipArchive(const std::filesystem::path& path) {
    int error = 0;
    archive_ = zip_open(path.string().c_str(), ZIP_RDONLY, &error);
  }

  ~ZipArchive() {
    if (archive_ != nullptr) {
      zip_close(archive_);
    }
  }

  ZipArchive(const ZipArchive&) = delete;
  ZipArchive& operator=(const ZipArchive&) = delete;

  bool ok() const { return archive_ != nullptr; }

  std::vector<std::string> names() const {
    std::vector<std::string> result;
    const zip_int64_t count = zip_get_num_entries(archive_, 0);
    result.reserve(static_cast<std::size_t>(std::max<zip_int64_t>(count, 0)));
    for (zip_int64_t index = 0; index < count; ++index) {
      const char* name = zip_get_name(archive_, index, 0);
      if (name != nullptr) {
        result.emplace_back(name);
      }
    }
    return result;
  }

  std::string read(const std::string& name) const {
    zip_stat_t stat;
    zip_stat_init(&stat);
    if (zip_stat(archive_, name.c_str(), 0, &stat) != 0 ||
        (stat.valid & ZIP_STAT_SIZE) == 0 || stat.size > kMaxEntryBytes) {
      return {};
    }
    zip_file_t* file = zip_fopen(archive_, name.c_str(), 0);
    if (file == nullptr) {
      return {};
    }
    std::string buffer(static_cast<std::size_t>(stat.size), '\0');
    const zip_int64_t got = zip_fread(file, buffer.data(), buffer.size());
    zip_fclose(file);
    if (got < 0) {
      return {};
    }
    buffer.resize(static_cast<std::size_t>(got));
    return buffer;
  }

 private:
  zip_t* archive_ = nullptr;
};

std::string lower_extension(const std::filesystem::path& path) {
  std::string extension = path.extension().string();
  for (char& ch : extension) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return extension;
}

bool is_slide_xml(std::string_view name) {
  constexpr std::string_view kPrefix = "ppt/slides/slide";
  constexpr std::string_view kSuffix = ".xml";
  if (!name.starts_with(kPrefix) || !name.ends_with(kSuffix)) {
    return false;
  }
  const std::string_view middle =
      name.substr(kPrefix.size(), name.size() - kPrefix.size() - kSuffix.size());
  return !middle.empty() && middle.find('/') == std::string_view::npos;
}

void collect_tagged_text(const pugi::xml_node& node, std::string_view tag,
                         std::string& out) {
  for (pugi::xml_node child = node.first_child(); child;
       child = child.next_sibling()) {
    if (tag == child.name()) {
      out.append(child.child_value());
      out.push_back('\n');
      continue;
    }
    collect_tagged_text(child, tag, out);
  }
}

bool only_whitespace(std::string_view text) {
  for (const unsigned char ch : text) {
    if (!std::isspace(ch)) {
      return false;
    }
  }
  return true;
}

}  // namespace

Extraction extract_office(const std::filesystem::path& path) {
  Extraction result;
  const std::string extension = lower_extension(path);
  const bool pptx = extension == ".pptx";
  const bool docx = extension == ".docx";
  if (!pptx && !docx) {
    result.skip_reason = "unsupported office file";
    return result;
  }

  const ZipArchive archive(path);
  if (!archive.ok()) {
    result.skip_reason = "unreadable office file";
    return result;
  }

  std::vector<std::string> parts;
  for (const std::string& name : archive.names()) {
    if (pptx && is_slide_xml(name)) {
      parts.push_back(name);
    } else if (docx && name == "word/document.xml") {
      parts.push_back(name);
    }
  }
  std::sort(parts.begin(), parts.end());

  const std::string_view tag = pptx ? "a:t" : "w:t";
  std::string text;
  for (const std::string& name : parts) {
    const std::string xml = archive.read(name);
    if (xml.empty()) {
      continue;
    }
    pugi::xml_document document;
    if (!document.load_buffer(xml.data(), xml.size())) {
      continue;
    }
    collect_tagged_text(document, tag, text);
  }

  if (only_whitespace(text)) {
    result.skip_reason = "no text in office file";
    return result;
  }
  result.ok = true;
  result.text = std::move(text);
  return result;
}

}  // namespace recall
