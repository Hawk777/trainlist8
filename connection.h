#pragma once

#if !defined(CONNECTION_H)
#define CONNECTION_H

#include <memory>
#include <optional>
#include <variant>
#include "util.h"

namespace trainlist8 {
namespace soap {
struct SimulationState;
struct TrainData;
}

class Connection final {
	public:
	// The type of a received message.
	using Message = std::variant<const soap::SimulationState *, const soap::TrainData *>;

	explicit Connection();
	winrt::Windows::Foundation::IAsyncAction connect(winrt::hstring hostname);
	winrt::Windows::Foundation::IAsyncAction receiveMessage();
	std::optional<Message> lastMessage() const;

	private:
	// The Windows Web Services heap used for memory allocation related to the connection.
	std::unique_ptr<WS_HEAP, ::trainlist8::util::HeapDeleter> heap;

	// The duplex session TCP channel connected to Run 8.
	std::unique_ptr<WS_CHANNEL, ::trainlist8::util::ChannelDeleter> channel;

	// A message object used to decode incoming messages.
	std::unique_ptr<WS_MESSAGE, ::trainlist8::util::MessageDeleter> message;

	// An adapter used to run asynchronous operations on the channel.
	util::AsyncWSAdapter adapter;

	// The most recently received message.
	std::optional<Message> lastMessage_;
};
}

#endif