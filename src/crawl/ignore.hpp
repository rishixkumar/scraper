#pragma once

#include <filesystem>
#include <string_view>

namespace recall {

bool is_skipped_directory(std::string_view name);
bool is_secret_file(const std::filesystem::path& path);

}  // namespace recall
