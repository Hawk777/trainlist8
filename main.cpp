#include "pch.h"
#include <locale>
#include "location.h"
#include "main_window.h"
#include "message_pump.h"
#include "resource.h"
#include "territory.h"
#include "util.h"
#include "welcome_window.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, wchar_t *commandLine, int showCommand) {
	// Set up locale.
	std::locale::global(std::locale());

	// Bring up WinRT.
	struct WinRTInitializer final {
		explicit WinRTInitializer() {
			winrt::init_apartment();
		}
		~WinRTInitializer() {
			winrt::clear_factory_cache();
			winrt::uninit_apartment();
		}
	};
	WinRTInitializer winRTInitializer;

	// Create a dispatcher queue for long running tasks.
	winrt::Windows::System::DispatcherQueueController dispatcherQueueController = []() {
		static constinit const auto dqcOptions = []() {
			DispatcherQueueOptions ret{};
			ret.dwSize = sizeof(ret);
			ret.threadType = DQTYPE_THREAD_CURRENT;
			ret.apartmentType = DQTAT_COM_NONE;
			return ret;
		}();
		ABI::Windows::System::IDispatcherQueueController *raw;
		winrt::check_hresult(CreateDispatcherQueueController(dqcOptions, &raw));
		return winrt::Windows::System::DispatcherQueueController(raw, winrt::take_ownership_from_abi);
	}();

	// Load territory and location name strings.
	trainlist8::territory::init(instance);
	trainlist8::location::init(instance);

	// Initialize common controls.
	{
		INITCOMMONCONTROLSEX icc{};
		icc.dwSize = sizeof(icc);
		icc.dwICC = ICC_STANDARD_CLASSES;
		winrt::check_bool(InitCommonControlsEx(&icc));
	}

	// Register window class.
	const auto &welcomeClassRegistration = [instance]() {
		WNDCLASSEXW windowClass{};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.lpfnWndProc = &trainlist8::Window::windowProcThunk;
		windowClass.hInstance = instance;
		windowClass.hCursor = static_cast<HCURSOR>(trainlist8::util::loadImage(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
		windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		windowClass.lpszClassName = trainlist8::WelcomeWindow::windowClass;
		return trainlist8::util::WindowClassRegistration(windowClass);
	}();
	welcomeClassRegistration;
	const auto &mainClassRegistration = [instance]() {
		WNDCLASSEXW windowClass{};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.lpfnWndProc = &trainlist8::Window::windowProcThunk;
		windowClass.hInstance = instance;
		windowClass.hCursor = static_cast<HCURSOR>(trainlist8::util::loadImage(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
		windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		windowClass.lpszMenuName = MAKEINTRESOURCE(IDR_MAIN_MENU);
		windowClass.lpszClassName = trainlist8::MainWindow::windowClass;
		return trainlist8::util::WindowClassRegistration(windowClass);
	}();
	mainClassRegistration;

	// Create message pump.
	trainlist8::MessagePump pump;

	// Create welcome window.
	HWND welcomeHandle = trainlist8::Window::create(WS_EX_WINDOWEDGE, trainlist8::WelcomeWindow::windowClass, trainlist8::util::loadString(instance, IDS_APP_NAME).c_str(), WS_CAPTION | WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, CW_USEDEFAULT, 0, 500, 285, nullptr, nullptr, instance, [&pump, commandLine](HWND handle) { return new trainlist8::WelcomeWindow(handle, pump, commandLine); });
	winrt::check_bool(ShowWindowAsync(welcomeHandle, showCommand));

	// Run message pump.
	return pump.run();
}