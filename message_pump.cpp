#include "pch.h"
#include "message_pump.h"
#include "window.h"

int trainlist8::MessagePump::run() {
	MSG msg;
	for(;;) {
		BOOL ret = GetMessage(&msg, nullptr, 0, 0);
		if(ret == -1) {
			winrt::throw_last_error();
		}
		if(!ret) {
			break;
		}
		bool stolen = false;
		for(Window *i : windows) {
			if(IsDialogMessageW(*i, &msg)) {
				stolen = true;
				break;
			}
		}
		if(!stolen) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}