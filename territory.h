#pragma once

#if !defined(TERRITORY_H)
#define TERRITORY_H

#include <memory>
#include <string>

namespace trainlist8 {
namespace territory {
constexpr size_t count = 7;
void init(HINSTANCE instance);
const std::wstring *nameByID(unsigned int territory);
}
}

#endif