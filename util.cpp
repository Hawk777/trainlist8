#include "pch.h"
#include "util.h"
#include <cstdarg>

namespace util = trainlist8::util;

namespace trainlist8::util {
namespace {
WS_CHANNEL_STATE getChannelState(WS_CHANNEL *channel) {
	WS_CHANNEL_STATE state;
	winrt::check_hresult(WsGetChannelProperty(channel, WS_CHANNEL_PROPERTY_STATE, &state, sizeof(state), nullptr));
	return state;
}
}
}

util::AsyncWSAdapter::AsyncWSAdapter() :
	event(CreateEventW(nullptr, FALSE, FALSE, nullptr)),
	context{
	.callback = &rawCallback,
	.callbackState = this},
	asyncResult() {
}

util::AsyncWSAdapter::AsyncWSAdapter(AsyncWSAdapter &&moveref) :
	event(std::move(moveref.event)),
	context{
	.callback=&rawCallback,
	.callbackState = this},
	asyncResult(moveref.asyncResult) {
}

util::AsyncWSAdapter::operator WS_ASYNC_CONTEXT *() {
	return &context;
}

winrt::Windows::Foundation::IAsyncAction util::AsyncWSAdapter::checkChannelOperation(HRESULT immediateResult, WS_CHANNEL *channel) {
	if(immediateResult != WS_S_ASYNC) {
		winrt::check_hresult(immediateResult);
		co_return;
	}
	(co_await winrt::get_cancellation_token()).callback([channel]() {
		WsAbortChannel(channel, nullptr);
	});
	co_await winrt::resume_on_signal(event.get());
	if(asyncResult == WS_E_OPERATION_ABORTED) {
		asyncResult = ERROR_CANCELLED;
	}
	winrt::check_hresult(asyncResult);
}

void WINAPI util::AsyncWSAdapter::rawCallback(HRESULT result, WS_CALLBACK_MODEL, void *state) {
	AsyncWSAdapter &adapter = *static_cast<AsyncWSAdapter *>(state);
	adapter.asyncResult = result;
	winrt::check_bool(SetEvent(adapter.event.get()));
}

void util::ChannelDeleter::operator()(WS_CHANNEL *channel) const {
	WS_CHANNEL_STATE state = getChannelState(channel);
	if(state == WS_CHANNEL_STATE_OPEN || state == WS_CHANNEL_STATE_FAULTED) {
		// A channel must be closed before it can be freed. Close, then immediately abort so the close completes immediately.
		winrt::handle event(CreateEventW(nullptr, FALSE, FALSE, nullptr));
		WS_ASYNC_CONTEXT context = {
			.callback = &rawCallback,
			.callbackState = event.get(),
		};
		if(WsCloseChannel(channel, &context, nullptr) == WS_S_ASYNC) {
			WsAbortChannel(channel, nullptr);
			WaitForSingleObject(event.get(), INFINITE);
		}
	}
	WsFreeChannel(channel);
}

void WINAPI util::ChannelDeleter::rawCallback(HRESULT, WS_CALLBACK_MODEL, void *state) {
	HANDLE event = reinterpret_cast<HANDLE>(state);
	SetEvent(event);
}

void util::FontDeleter::operator()(HFONT font) const {
	DeleteObject(font);
}

void util::HeapDeleter::operator()(WS_HEAP *heap) const {
	WsFreeHeap(heap);
}

void util::MessageDeleter::operator()(WS_MESSAGE *message) const {
	WsFreeMessage(message);
}

util::WindowClassRegistration::WindowClassRegistration(const WNDCLASSEXW &wc) :
	className(wc.lpszClassName),
	instance(wc.hInstance) {
	winrt::check_bool(RegisterClassExW(&wc));
}

util::WindowClassRegistration::~WindowClassRegistration() {
	UnregisterClassW(className, instance);
}

std::unique_ptr<HFONT, util::FontDeleter> util::createMessageBoxFont(unsigned int size, unsigned int dpi) {
	NONCLIENTMETRICSW ncm{};
	ncm.cbSize = sizeof(ncm);
	winrt::check_bool(SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0));
	LOGFONT fontSpec = ncm.lfMessageFont;
	fontSpec.lfHeight = -MulDiv(size, dpi, 72);
	fontSpec.lfWidth = 0;
	HFONT font = CreateFontIndirectW(&fontSpec);
	if(!font) {
		winrt::throw_last_error();
	}
	return std::unique_ptr<HFONT, util::FontDeleter>(font);
}

HWND util::createWindowEx(DWORD exStyle, const wchar_t *className, const wchar_t *windowName, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, void *param) {
	HWND ret = CreateWindowEx(exStyle, className, windowName, style, x, y, width, height, parent, menu, instance, param);
	if(!ret) {
		winrt::throw_last_error();
	}
	return ret;
}

HICON util::loadIconWithScaleDown(HINSTANCE instance, const wchar_t *name, int width, int height) {
	HICON ret = nullptr;
	HRESULT result = LoadIconWithScaleDown(instance, name, width, height, &ret);
	if(SUCCEEDED(result)) {
		return ret;
	} else {
		winrt::throw_hresult(result);
	}
}

HANDLE util::loadImage(HINSTANCE instance, const wchar_t *name, unsigned int type, int width, int height, unsigned int options) {
	HANDLE ret = LoadImageW(instance, name, type, width, height, options);
	if(!ret) {
		winrt::throw_last_error();
	}
	return ret;
}

std::wstring util::loadString(HINSTANCE instance, unsigned int id) {
	return std::wstring(loadStringView(instance, id));
}

std::wstring_view util::loadStringView(HINSTANCE instance, unsigned int id) {
	wchar_t *ptr;
	int len = LoadStringW(instance, id, reinterpret_cast<wchar_t *>(&ptr), 0);
	if(!len) {
		winrt::throw_last_error();
	}
	return std::wstring_view(ptr, ptr + len);
}

std::wstring util::loadAndFormatString(HINSTANCE instance, unsigned int id, ...) {
	const std::wstring &messageTemplate = loadString(instance, id);
	std::wstring ret(65536, '\0');
	std::va_list args;
	va_start(args, id);
	DWORD len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, messageTemplate.c_str(), 0, 0, ret.data(), ret.size(), &args);
	va_end(args);
	if(len) {
		ret.resize(len);
		ret.shrink_to_fit();
		return ret;
	} else {
		winrt::throw_last_error();
	}
}