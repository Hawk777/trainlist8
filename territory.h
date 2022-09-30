#pragma once

#if !defined(TERRITORY_H)
#define TERRITORY_H

#include <memory>
#include <optional>
#include <string>

namespace trainlist8 {
namespace territory {
constexpr size_t count = 7;
void init(HINSTANCE instance);
std::optional<unsigned int> idByBlock(int32_t block);
unsigned int idByIndex(size_t index);
std::optional<size_t> indexByID(unsigned int territory);
const std::wstring *nameByIndex(size_t index);
const std::wstring *nameByID(unsigned int territory);
}
}

#endif