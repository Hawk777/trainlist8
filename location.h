#pragma once

#if !defined(LOCATION_H)
#define LOCATION_H

#include <string>

namespace trainlist8 {
namespace location {
void init(HINSTANCE instance);
const std::wstring *nameByBlock(int32_t block);
}
}

#endif