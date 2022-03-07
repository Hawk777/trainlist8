#pragma once

#if !defined(MESSAGE_PUMP_H)
#define MESSAGE_PUMP_H

#include <unordered_set>

namespace trainlist8 {
class Window;

// A message pump.
class MessagePump final {
	public:
	explicit MessagePump() = default;
	int run();

	private:
	friend class Window;

	std::unordered_set<Window *> windows;
};
}

#endif