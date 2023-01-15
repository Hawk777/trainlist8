#pragma once

#if !defined(WINDOW_H)
#define WINDOW_H

#include <functional>

namespace trainlist8 {
class MessagePump;
extern "C" using WindowProc = LRESULT WINAPI(HWND, unsigned int, WPARAM, LPARAM);

// The base class of a C++-based window.
class Window {
	public:
	static HWND create(DWORD exStyle, const wchar_t *className, const wchar_t *windowName, DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, std::function<Window *(HWND)> factory);
	static WindowProc windowProcThunk;

	explicit Window(const Window &) = delete;

	void operator=(const Window &) = delete;

	protected:
	MessagePump &pump;

	explicit Window(HWND handle, MessagePump &pump);
	virtual ~Window();

	operator HWND() const;

	unsigned int dpi() const;
	HINSTANCE instance() const;
	void updateIcon();
	virtual LRESULT windowProc(unsigned int message, WPARAM wParam, LPARAM lParam) = 0;

	private:
	friend class MessagePump;

	HWND handle_;
	unsigned int dpi_;
	HICON largeIcon, smallIcon;
};
}

#endif