#pragma once

#include "extract/extractor.hpp"

#include <filesystem>

namespace recall {

Extraction extract_pdf(const std::filesystem::path& path);
Extraction extract_office(const std::filesystem::path& path);

}  // namespace recall
