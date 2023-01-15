#include "pch.h"
#include <cassert>
#include <exception>
#include <stdexcept>
#include <utility>
#include "message_pump.h"
#include "resource.h"
#include "util.h"
#include "window.h"

using trainlist8::Window;

namespace {
struct CreateInfo final {
	std::function<Window *(HWND)> factory;
	std::exception_ptr exception;
};

extern "C" using GetDpiForWindowFunction = unsigned int WINAPI(HWND);
using GetDpiForWindowPointer = GetDpiForWindowFunction *;

GetDpiForWindowPointer loadGetDpiForWindow() {
	HMODULE module = LoadLibraryW(L"shcore");
	if(!module) {
		return nullptr;
	}
	return reinterpret_cast<GetDpiForWindowPointer>(GetProcAddress(module, "GetDpiForWindow"));
}

unsigned int getDPIForWindow(HWND handle) {
	static GetDpiForWindowPointer ptr = loadGetDpiForWindow();
	if(ptr) {
		return ptr(handle);
	} else {
		HDC screenDC = GetDC(nullptr);
		unsigned int dpi = GetDeviceCaps(screenDC, LOGPIXELSX);
		ReleaseDC(nullptr, screenDC);
		return dpi;
	}
}
}

HWND Window::create(DWORD exStyle, const wchar_t *className, const wchar_t *windowName, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, std::function<Window *(HWND)> factory) {
	CreateInfo ci{};
	ci.factory = std::move(factory);
	HWND handle = CreateWindowExW(exStyle, className, windowName, style, x, y, width, height, parent, menu, instance, &ci);
	if(!handle) {
		if(ci.exception) {
			std::rethrow_exception(std::move(ci.exception));
		} else {
			winrt::throw_last_error();
		}
	}
	return handle;
}

LRESULT WINAPI Window::windowProcThunk(HWND window, unsigned int message, WPARAM wParam, LPARAM lParam) {
	Window *win = reinterpret_cast<Window *>(GetWindowLongPtrW(window, GWLP_USERDATA));
	if(!win && message == WM_CREATE) {
		const CREATESTRUCTW &cs = *reinterpret_cast<const CREATESTRUCTW *>(lParam);
		CreateInfo &ci = *static_cast<CreateInfo *>(cs.lpCreateParams);
		try {
			win = ci.factory(window);
			assert(win);
		} catch(...) {
			ci.exception = std::current_exception();
			return (message == WM_CREATE) ? -1 : 0;
		}
	}
	if(win && message == WM_DPICHANGED) {
		win->dpi_ = LOWORD(wParam);
	}
	int ret;
	if(win) {
		ret = win->windowProc(message, wParam, lParam);
	} else {
		ret = DefWindowProcW(window, message, wParam, lParam);
	}
	if(message == WM_NCDESTROY) {
		delete win;
	}
	return ret;
}

Window::Window(HWND handle, MessagePump &pump) :
	pump(pump),
	handle_(handle),
	dpi_(getDPIForWindow(handle)),
	largeIcon(NULL),
	smallIcon(NULL) {
	SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	pump.windows.insert(this);
}

Window::~Window() {
	pump.windows.erase(this);
	for(HICON i : {largeIcon, smallIcon}) {
		if(i) {
			DestroyIcon(i);
		}
	}
}

Window::operator HWND() const {
	assert(handle_);
	return handle_;
}

unsigned int Window::dpi() const {
	return dpi_;
}

HINSTANCE Window::instance() const {
	return reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(handle_, GWLP_HINSTANCE));
}

void Window::updateIcon() {
	struct Params {
		HICON Window::*handle;
		int widthMetric, heightMetric, iconType;
	};
	static constinit const Params params[] = {
		{&Window::largeIcon, SM_CXICON, SM_CYICON, ICON_BIG},
		{&Window::smallIcon, SM_CXSMICON, SM_CYSMICON, ICON_SMALL},
	};
	HINSTANCE inst = instance();
	for(const Params &i : params) {
		int width = GetSystemMetricsForDpi(i.widthMetric, dpi_);
		if(!width) {
			winrt::throw_last_error();
		}
		int height = GetSystemMetricsForDpi(i.heightMetric, dpi_);
		if(!height) {
			winrt::throw_last_error();
		}
		HICON oldIcon = this->*i.handle;
		this->*i.handle = util::loadIconWithScaleDown(inst, MAKEINTRESOURCE(IDI_TRAINLIST8), width, height);
		SendMessage(handle_, WM_SETICON, i.iconType, reinterpret_cast<LPARAM>(this->*i.handle));
		if(oldIcon) {
			DestroyIcon(oldIcon);
		}
	}
}