#include "pch.h"
#include "error.h"

namespace error = trainlist8::error;

namespace trainlist8::error {
namespace {
constexpr unsigned long customerBit = 1UL << 29;

enum Code {
	CODE_NO_DISPATCHER_PERMISSION = 1,
};
}
}

const winrt::hresult error::noDispatcherPermission = customerBit | MAKE_HRESULT(SEVERITY_ERROR, 0, CODE_NO_DISPATCHER_PERMISSION);