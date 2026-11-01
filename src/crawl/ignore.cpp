#include "crawl/ignore.hpp"

#include <initializer_list>
#include <string>

namespace recall {
namespace {

bool equals_any(std::string_view name,
                std::initializer_list<std::string_view> names) {
  for (const std::string_view candidate : names) {
    if (name == candidate) {
      return true;
    }
  }
  return false;
}

}  // namespace

bool is_skipped_directory(std::string_view name) {
  return equals_any(name, {".git",         "node_modules", ".venv", "venv",
                           "build",        "dist",         ".idea", ".vscode",
                           "cmake-build-debug"});
}

bool is_secret_file(const std::filesystem::path& path) {
  const std::string name = path.filename().string();
  if (name == ".env" || name.rfind(".env.", 0) == 0 || name == ".DS_Store" ||
      name == "id_rsa" || name == "id_ed25519" || name == "known_hosts" ||
      name == "cookies.txt") {
    return true;
  }
  const std::string ext = path.extension().string();
  return ext == ".pem" || ext == ".key" || ext == ".p12" || ext == ".pfx";
}

}  // namespace recall
