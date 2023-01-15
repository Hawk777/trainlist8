#include "pch.h"
#include <cassert>
#include <string>
#include "error.h"
#include "main_window.h"
#include "resource.h"
#include "util.h"
#include "welcome_window.h"

using trainlist8::WelcomeWindow;
using trainlist8::util::operator ""_as_xml;

constinit const wchar_t WelcomeWindow::windowClass[] = L"welcome";

WelcomeWindow::WelcomeWindow(HWND handle, MessagePump &pump) :
	Window(handle, pump),
	font(nullptr),
	label(util::createWindowEx(0, WC_STATICW, util::loadString(instance(), IDS_WELCOME_LABEL).c_str(), SS_LEFT | SS_NOPREFIX | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, *this, nullptr, instance(), nullptr)),
	localhostRadio(util::createWindowEx(0, WC_BUTTONW, util::loadString(instance(), IDS_WELCOME_LOCALHOST).c_str(), BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, *this, nullptr, instance(), nullptr)),
	otherComputerRadio(util::createWindowEx(0, WC_BUTTONW, util::loadString(instance(), IDS_WELCOME_OTHER_COMPUTER).c_str(), BS_AUTORADIOBUTTON | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, *this, nullptr, instance(), nullptr)),
	hostnameEdit(util::createWindowEx(0, WC_EDITW, L"", ES_AUTOHSCROLL | ES_LEFT | ES_LOWERCASE | WS_BORDER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, *this, nullptr, instance(), nullptr)),
	connectButton(util::createWindowEx(0, WC_BUTTONW, util::loadString(instance(), IDS_WELCOME_CONNECT).c_str(), BS_CENTER | BS_DEFPUSHBUTTON | BS_PUSHBUTTON | BS_TEXT | BS_VCENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, *this, nullptr, instance(), nullptr)),
	cancelButton(util::createWindowEx(0, WC_BUTTONW, util::loadString(instance(), IDS_CANCEL).c_str(), BS_CENTER | BS_PUSHBUTTON | BS_TEXT | BS_VCENTER | WS_CHILD | WS_TABSTOP | WS_VISIBLE, 0, 0, 0, 0, *this, nullptr, instance(), nullptr)),
	connection(),
	connecting(),
	cancelling(false),
	closePending(false),
	quitOnDestroy(true) {
	Button_SetCheck(localhostRadio, BST_CHECKED);
	updateIcon();
	updateLayoutAndFont();
	updateControlsEnabled();
}

LRESULT WelcomeWindow::windowProc(unsigned int message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_CLOSE:
			if(connecting) {
				if(cancelling) {
					closePending = true;
				} else {
					closePending = true;
					connecting.Cancel();
					cancelling = true;
					updateControlsEnabled();
				}
			} else {
				DestroyWindow(*this);
			}
			return 0;

		case WM_COMMAND:
		{
			HWND control = reinterpret_cast<HWND>(lParam);
			if(control == localhostRadio || control == otherComputerRadio) {
				if(HIWORD(wParam) == BN_CLICKED) {
					updateControlsEnabled();
					return 0;
				}
			} else if(control == hostnameEdit) {
				if(HIWORD(wParam) == EN_CHANGE) {
					updateControlsEnabled();
					return 0;
				}
			} else if(control == connectButton || LOWORD(wParam) == IDOK) {
				if(HIWORD(wParam) == BN_CLICKED) {
					connect();
					return 0;
				}
			} else if(control == cancelButton) {
				if(HIWORD(wParam) == BN_CLICKED) {
					assert(connecting);
					connecting.Cancel();
					cancelling = true;
					updateControlsEnabled();
					return 0;
				}
			}
		}
		break;

		case WM_DESTROY:
			if(quitOnDestroy) {
				PostQuitMessage(0);
			}
			return 0;

		case WM_DPICHANGED:
		{
			const RECT &rect = *reinterpret_cast<const RECT *>(lParam);
			SetWindowPos(*this, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
			updateIcon();
			updateLayoutAndFont();
		}
		return 0;
	}
	return DefWindowProcW(*this, message, wParam, lParam);
}

void WelcomeWindow::updateLayoutAndFont() {
	// Set a good font.
	std::unique_ptr<HFONT, util::FontDeleter> newFont = util::createMessageBoxFont(12, dpi());
	for(HWND window : {label, localhostRadio, otherComputerRadio, hostnameEdit, connectButton, cancelButton}) {
		SendMessage(window, WM_SETFONT, reinterpret_cast<WPARAM>(newFont.get()), TRUE);
	}
	font = std::move(newFont);

	// Lay out the controls.
	int margin = MulDiv(10, dpi(), USER_DEFAULT_SCREEN_DPI);
	int rowHeight = MulDiv(25, dpi(), USER_DEFAULT_SCREEN_DPI);
	RECT clientRect;
	winrt::check_bool(GetClientRect(*this, &clientRect));
	int clientWidth = clientRect.right - clientRect.left;
	int y = margin;
	MoveWindow(label, margin, y, clientWidth - 2 * margin, rowHeight, TRUE);
	y += rowHeight + margin;
	MoveWindow(localhostRadio, margin, y, clientWidth - 2 * margin, rowHeight, TRUE);
	y += rowHeight + margin;
	MoveWindow(otherComputerRadio, margin, y, clientWidth - 2 * margin, rowHeight, TRUE);
	y += rowHeight + margin;
	MoveWindow(hostnameEdit, margin * 3, y, clientWidth - 4 * margin, rowHeight, TRUE);
	y += rowHeight + margin * 4;
	int buttonsWidth = clientWidth / 2;
	int buttonWidth = (buttonsWidth - margin) / 2;
	int buttonsX = (clientWidth - buttonsWidth) / 2;
	MoveWindow(connectButton, buttonsX, y, buttonWidth, rowHeight, TRUE);
	MoveWindow(cancelButton, buttonsX + buttonWidth + margin, y, buttonWidth, rowHeight, TRUE);
}

void WelcomeWindow::updateControlsEnabled() {
	EnableWindow(localhostRadio, !connecting);
	EnableWindow(otherComputerRadio, !connecting);
	EnableWindow(hostnameEdit, !connecting && Button_GetCheck(otherComputerRadio) == BST_CHECKED);
	EnableWindow(connectButton, !connecting && (Button_GetCheck(localhostRadio) == BST_CHECKED || Edit_GetTextLength(hostnameEdit) != 0));
	EnableWindow(cancelButton, !!connecting && !cancelling);
}

void WelcomeWindow::connect() {
	auto uiThread = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();

	// Sanity check: the connect button should be disabled while a connection is in progress.
	assert(!connecting);

	// Determine the host the user wants to connect to.
	winrt::hstring hostname;
	if(Button_GetCheck(localhostRadio) == BST_CHECKED) {
		hostname = L"localhost";
	} else {
		SetLastError(0);
		int len = GetWindowTextLengthW(hostnameEdit);
		if(!len) {
			// len could also be zero if the field is empty, but if the field is empty we disable the Connect button, so that is impossible.
			winrt::throw_last_error();
		}
		std::wstring buffer(len + 1, '\0');
		len = GetWindowTextW(hostnameEdit, buffer.data(), buffer.size());
		if(!len) {
			winrt::throw_last_error();
		}
		buffer.resize(len);
		hostname = buffer;
	}

	// Start the background operation.
	connection.emplace();
	connecting = connection->connect(std::move(hostname));

	// Disable the controls while the connection operation is in progress.
	updateControlsEnabled();

	// Register for notification of completion of the operation.
	connecting.Completed([this, uiThread](const winrt::Windows::Foundation::IAsyncAction &action, winrt::Windows::Foundation::AsyncStatus status) {
		// See what happened.
		winrt::hresult_error error;
		try {
			action.GetResults();
		} catch(const winrt::hresult_canceled &) {
			// The connection request was cancelled by the user, by clicking either the cancel button or the window close button.
		} catch(const winrt::hresult_error &err) {
			// The connection attempt failed.
			error = err;
		}

		// Dispatch on the UI thread to deal with the result.
		winrt::check_bool(uiThread.TryEnqueue([this, status, error]() {
			// Report the error (if appropriate) and clean up state.
			switch(status) {
				case winrt::Windows::Foundation::AsyncStatus::Completed:
				{
					// Callables pointed to by std::function must be copyable. Connection is not copyable, nor is unique_ptr<Connection>.
					std::shared_ptr<Connection> connection = std::make_shared<Connection>(std::move(*this->connection));
					HWND mainWindowHandle = trainlist8::Window::create(WS_EX_WINDOWEDGE, MainWindow::windowClass, trainlist8::util::loadString(instance(), IDS_APP_NAME).c_str(), WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_OVERLAPPED | WS_SIZEBOX | WS_SYSMENU, CW_USEDEFAULT, 0, 1000, 500, nullptr, nullptr, instance(), [connection = std::move(connection), &pump = pump](HWND handle) mutable {
						assert(connection.use_count() == 1);
						return new trainlist8::MainWindow(handle, pump, std::forward<Connection &&>(*connection));
					});
					winrt::check_bool(ShowWindowAsync(mainWindowHandle, SW_SHOWDEFAULT));
					quitOnDestroy = false;
					DestroyWindow(*this);
				}
				return;

				case winrt::Windows::Foundation::AsyncStatus::Canceled:
					break;

				case winrt::Windows::Foundation::AsyncStatus::Error:
					if(error.code() == error::noDispatcherPermission) {
						MessageBoxW(*this, util::loadString(instance(), IDS_WELCOME_NO_PERMISSION).c_str(), util::loadString(instance(), IDS_APP_NAME).c_str(), MB_OK | MB_ICONHAND);
					} else {
						MessageBoxW(*this, util::loadAndFormatString(instance(), IDS_WELCOME_CONNECTION_ERROR, error.message().c_str()).c_str(), util::loadString(instance(), IDS_APP_NAME).c_str(), MB_OK | MB_ICONHAND);
					}
					break;
			}
			connection.reset();
			connecting = {};
			cancelling = false;
			updateControlsEnabled();
			if(closePending) {
				DestroyWindow(*this);
			}
			}));
		});
}