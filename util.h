#pragma once

#if !defined(UTIL_H)
#define UTIL_H

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace trainlist8 {
namespace util {
extern "C" using WSAsyncCallback = void WINAPI(HRESULT, WS_CALLBACK_MODEL, void *);

// An adapter that allows a Windows Web Services asynchronous operation to be co_awaited in WinRT.
class AsyncWSAdapter final {
	public:
	explicit AsyncWSAdapter();
	AsyncWSAdapter(AsyncWSAdapter &&);
	operator WS_ASYNC_CONTEXT *();
	winrt::Windows::Foundation::IAsyncAction checkChannelOperation(HRESULT immediateResult, WS_CHANNEL *channel);

	private:
	winrt::handle event;
	WS_ASYNC_CONTEXT context;
	HRESULT asyncResult;

	static WSAsyncCallback rawCallback;
};

// A delete for channels that can be used with unique_ptr.
class ChannelDeleter final {
	public:
	void operator()(WS_CHANNEL *channel) const;

	private:
	static WSAsyncCallback rawCallback;
};

// A deleter for fonts that can be used with unique_ptr.
class FontDeleter final {
	public:
	using pointer = HFONT;
	void operator()(HFONT font) const;
};

// A deleter for Windows Web Services heaps that can be used with unique_ptr.
class HeapDeleter final {
	public:
	void operator()(WS_HEAP *heap) const;
};

// A deleter for image lists that can be used with unique_ptr.
class ImageListDeleter final {
	public:
	using pointer = HIMAGELIST;
	void operator()(HIMAGELIST imageList) const;
};

// A deleter for messages that can be used with unique_ptr.
class MessageDeleter final {
	public:
	void operator()(WS_MESSAGE *message) const;
};

// An RAII-managed registration of a window class.
class WindowClassRegistration final {
	public:
	explicit WindowClassRegistration(const WNDCLASSEXW &wc);
	explicit WindowClassRegistration(const WindowClassRegistration &) = delete;
	~WindowClassRegistration();

	void operator=(const WindowClassRegistration &) = delete;

	private:
	const wchar_t *className;
	HINSTANCE instance;
};

namespace impl {
template<std::size_t n>
class char8_literal final {
	public:
	char8_t chars[n];

	private:
	template<std::size_t ... i>
	constexpr char8_literal(const char8_t(&s)[n], std::index_sequence<i...>) :
		chars{s[i]...} {
	}

	public:
	constexpr char8_literal(const char8_t(&s)[n]) :
		char8_literal(s, std::make_index_sequence<n>()) {
	}
};

template<char8_literal literal, std::size_t ... i>
constexpr const std::array<unsigned char, sizeof...(i)> bytes_of_string{literal.chars[i] ...};

template<char8_literal literal, std::size_t ... i>
constexpr const std::array<unsigned char, sizeof...(i)> &make_bytes_of_string(std::index_sequence<i...>) {
	return bytes_of_string<literal, i...>;
}
}

// An XML string pointing at a string literal.
template<impl::char8_literal literal>
constexpr WS_XML_STRING operator""_as_xml() {
	const std::array<unsigned char, std::size(literal.chars)> &a = impl::make_bytes_of_string<literal>(std::make_index_sequence<std::size(literal.chars)>());
	WS_XML_STRING ret = {
		.length = a.size() - 1,
		.bytes = const_cast<unsigned char *>(a.data()),
	};
	return ret;
}

// Constructs the font used in message boxes, but at a specified point size scaled for a screen of a specified DPI.
std::unique_ptr<HFONT, FontDeleter> createMessageBoxFont(unsigned int size, unsigned int dpi);

// Create a window, throwing an exception on failure.
HWND createWindowEx(DWORD exStyle, const wchar_t *className, const wchar_t *windowName, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, void *param);

// Loads an icon, throwing an exception on failure.
HICON loadIconWithScaleDown(HINSTANCE instance, const wchar_t *name, int width, int height);

// Loads an image, throwing an exception on failure.
HANDLE loadImage(HINSTANCE instance, const wchar_t *name, unsigned int type, int width, int height, unsigned int options);

// Loads a string, throwing an exception on failure.
std::wstring loadString(HINSTANCE instance, unsigned int id);

// Loads a string, throwing an exception on failure.
std::wstring_view loadStringView(HINSTANCE instance, unsigned int id);

// Loads a string and formats it with inserts, throwing an exception on failure.
std::wstring loadAndFormatString(HINSTANCE instance, unsigned int id, ...);
}
}

#endif