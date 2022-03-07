#pragma once

#if !defined(WELCOME_WINDOW_H)
#define WELCOME_WINDOW_H

#include <memory>
#include <optional>
#include "connection.h"
#include "util.h"
#include "window.h"

namespace trainlist8 {
class MessagePump;

class WelcomeWindow final : public Window {
	public:
	static const wchar_t windowClass[];

	explicit WelcomeWindow(HWND handle, MessagePump &pump);
	~WelcomeWindow() = default;

	protected:
	LRESULT windowProc(unsigned int message, WPARAM wParam, LPARAM lParam) override;

	private:
	std::unique_ptr<HFONT, util::FontDeleter> font;
	HWND label;
	HWND localhostRadio;
	HWND otherComputerRadio;
	HWND hostnameEdit;
	HWND connectButton;
	HWND cancelButton;
	std::optional<Connection> connection;
	winrt::Windows::Foundation::IAsyncAction connecting;
	bool cancelling, closePending, quitOnDestroy;

	void updateLayoutAndFont();
	void updateControlsEnabled();
	void connect();
};
}

#endif