#include "pch.h"
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include "connection.h"
#include "error.h"
#include "soap.h"

namespace error = trainlist8::error;
namespace soap = trainlist8::soap;
namespace util = trainlist8::util;
using trainlist8::Connection;

namespace {
std::unique_ptr<WS_HEAP, util::HeapDeleter> createHeap() {
	WS_HEAP *raw;
	winrt::check_hresult(WsCreateHeap(std::numeric_limits<size_t>::max(), 0, nullptr, 0, &raw, nullptr));
	return std::unique_ptr<WS_HEAP, util::HeapDeleter>(raw, util::HeapDeleter());
}
// Create a channel.
std::unique_ptr<WS_CHANNEL, util::ChannelDeleter> createChannel() {
	WS_CHANNEL *raw;
	winrt::check_hresult(WsCreateChannel(WS_CHANNEL_TYPE_DUPLEX_SESSION, WS_TCP_CHANNEL_BINDING, nullptr, 0, nullptr, &raw, nullptr));
	return std::unique_ptr<WS_CHANNEL, util::ChannelDeleter>(raw, util::ChannelDeleter());
}

// Create a message.
std::unique_ptr<WS_MESSAGE, util::MessageDeleter> createMessage(WS_CHANNEL *channel) {
	WS_MESSAGE *raw;
	winrt::check_hresult(WsCreateMessageForChannel(channel, nullptr, 0, &raw, nullptr));
	return std::unique_ptr<WS_MESSAGE, util::MessageDeleter>(raw, util::MessageDeleter());
}
}

// Constructs an unconnected Connection object.
//
// The connect function must be called before this object can be used to receive messages.
Connection::Connection() :
	heap(createHeap()),
	channel(createChannel()),
	message(createMessage(channel.get())),
	adapter(),
	lastMessage_() {
}

// Connects to the Run 8 instance running on the specified computer.
winrt::Windows::Foundation::IAsyncAction Connection::connect(winrt::hstring hostname) {
	auto cancelToken = co_await winrt::get_cancellation_token();
	cancelToken.enable_propagation();

	// Build a URL.
	std::wstring url;
	{
		using namespace std::literals;
		static constinit const std::wstring_view portString = L"15192"sv;
		static constinit const std::wstring_view path = L"/Run8";
		WS_NETTCP_URL urlStruct = {
			.url = {.scheme = WS_URL_NETTCP_SCHEME_TYPE},
			.host = {.length = hostname.size(), .chars = const_cast<wchar_t *>(hostname.data())},
			.port = 15192,
			.portAsString = {.length = portString.size(), .chars = const_cast<wchar_t *>(portString.data())},
			.path = {.length = path.size(), .chars = const_cast<wchar_t *>(path.data())},
			.query = {.length = 0, .chars = nullptr},
			.fragment = {.length = 0, .chars = nullptr},
		};
		WS_STRING urlString;
		winrt::check_hresult(WsEncodeUrl(&urlStruct.url, 0, heap.get(), &urlString, nullptr));
		url = std::wstring(urlString.chars, urlString.length);
		winrt::check_hresult(WsResetHeap(heap.get(), nullptr));
	}

	// Connect to Run8.
	WS_ENDPOINT_ADDRESS endpoint{
		.url = {
			.length = url.size(),
			.chars = url.data(), },
	};
	co_await adapter.checkChannelOperation(WsOpenChannel(channel.get(), &endpoint, adapter, nullptr), channel.get());

	// Send the DispatcherConnected message.
	auto getMessageState = [this]() {
		WS_MESSAGE_STATE state;
		winrt::check_hresult(WsGetMessageProperty(message.get(), WS_MESSAGE_PROPERTY_STATE, &state, sizeof(state), nullptr));
		return state;
	};

	co_await adapter.checkChannelOperation(WsSendMessage(channel.get(), message.get(), &soap::dispatcherConnectedMessage, WS_WRITE_REQUIRED_VALUE, nullptr, 0, adapter, nullptr), channel.get());
	winrt::check_hresult(WsResetMessage(message.get(), nullptr));

	// Receive a PermissionUpdate message.
	soap::DispatcherPermission permission;
	{
		static constinit const WS_MESSAGE_DESCRIPTION *const descriptionPointer = &soap::permissionUpdateMessage;
		co_await adapter.checkChannelOperation(WsReceiveMessage(channel.get(), message.get(), const_cast<const WS_MESSAGE_DESCRIPTION **>(&descriptionPointer), 1, WS_RECEIVE_REQUIRED_MESSAGE, WS_READ_REQUIRED_VALUE, heap.get(), &permission, sizeof(permission), nullptr, adapter, nullptr), channel.get());
		winrt::check_hresult(WsResetMessage(message.get(), nullptr));
		winrt::check_hresult(WsResetHeap(heap.get(), nullptr));
	}
	if(permission.permission == soap::DispatcherPermissionLevel::RESCINDED) {
		throw winrt::hresult_error(error::noDispatcherPermission);
	}
}

// Waits until the next TrainData message is received from the connected Run 8 instance.
winrt::Windows::Foundation::IAsyncAction Connection::receiveMessage() {
	auto cancelToken = co_await winrt::get_cancellation_token();
	cancelToken.enable_propagation();

	for(;;) {
		// Clean up any previous message.
		lastMessage_.reset();
		winrt::check_hresult(WsResetMessage(message.get(), nullptr));
		winrt::check_hresult(WsResetHeap(heap.get(), nullptr));

		// Receive some kind of message.
		static constinit std::array descriptionPointers = {
			// Index 0 is simulation state.
			&soap::sendSimulationStateMessage,
			// Index 1 is train data.
			&soap::updateTrainDataMessage,
			// Index 2 is permission update.
			&soap::permissionUpdateMessage,
			// Indices 3+ are ignored.
			&soap::dtmfMessage,
			&soap::radioTextMessage,
			&soap::setInterlockErrorSwitchesMessage,
			&soap::setOccupiedBlocksMessage,
			&soap::setOccupiedSwitchesMessage,
			&soap::setReversedSwitchesMessage,
			&soap::setSignalsMessage,
			&soap::setUnlockedSwitchesMessage,
		};
		void *body = nullptr;
		unsigned long index;
		co_await adapter.checkChannelOperation(WsReceiveMessage(channel.get(), message.get(), const_cast<const WS_MESSAGE_DESCRIPTION **>(descriptionPointers.data()), descriptionPointers.size(), WS_RECEIVE_REQUIRED_MESSAGE, WS_READ_REQUIRED_POINTER, heap.get(), &body, sizeof(body), &index, adapter, nullptr), channel.get());
		if(index == 0) {
			lastMessage_ = static_cast<soap::SimulationState *>(body);
			break;
		} else if(index == 1) {
			lastMessage_ = static_cast<soap::TrainData *>(body);
			break;
		} else if(index == 2) {
			const soap::DispatcherPermission *permission = static_cast<const soap::DispatcherPermission *>(body);
			if(permission->permission == soap::DispatcherPermissionLevel::RESCINDED) {
				throw winrt::hresult_error(error::noDispatcherPermission);
			}
		}
	}
}

// Returns the last message received by receiveMessage.
//
// If no message has been received yet, an empty optional is returned.
//
// The returned pointer is valid until the next call to receiveMessage.
std::optional<Connection::Message> Connection::lastMessage() const {
	return lastMessage_;
}