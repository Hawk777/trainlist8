#pragma once

#if !defined(MAIN_WINDOW_H)
#define MAIN_WINDOW_H

#include <atomic>
#include <bitset>
#include <optional>
#include <string>
#include <unordered_map>
#include "connection.h"
#include "territory.h"
#include "window.h"

namespace trainlist8 {
class MessagePump;
extern "C" using ListViewCompareCallback = int WINAPI(LPARAM, LPARAM, LPARAM);

namespace soap {
enum class EngineerType : int32_t;
}

class MainWindow final : public Window {
	public:
	// Information about a train that is saved persistently and made available for display.
	struct TrainInfo final {
		// The Windows list view ID number.
		unsigned int listViewID;

		// The type of driver.
		soap::EngineerType engineerType;

		// The name of the driver, if a player.
		std::wstring engineerName;

		// How many simulation state messages have been received since the last update message for this train.
		unsigned int age;

		// The lead unit name.
		std::wstring leadUnit;

		// The train symbol.
		std::wstring symbol;

		// The most recent train length.
		uint32_t length;

		// The most recent train weight.
		uint32_t weight;

		// The horsepower per ton.
		float horsepowerPerTon;

		// The most recent train speed.
		int speed;

		// The most recent territory, or an empty optional if the train is in an unsignalled location.
		std::optional<unsigned int> territory;

		// The current block ID.
		int32_t block = -1;

		// The block ID that the train most recently occupied that has a known name.
		int32_t lastNamedBlock = -1;
	};

	// Scratch buffers used internally during text formatting.
	struct ScratchBuffers final {
		// A string buffer.
		std::string string;

		// A wstring buffer.
		std::wstring wstring;

		// Another wstring buffer.
		std::wstring wstring2;
	};

	static const wchar_t windowClass[];

	explicit MainWindow(HWND handle, MessagePump &pump, Connection connection);

	protected:
	LRESULT windowProc(unsigned int message, WPARAM wParam, LPARAM lParam) override;

	private:
	enum class DateTimeFormat {
		LOCALE,
		ISO_8601,
	};

	std::unordered_map<uint32_t, TrainInfo> trains;
	std::unique_ptr<HIMAGELIST, util::ImageListDeleter> driverImageList;
	std::unique_ptr<HFONT, util::FontDeleter> font;
	HWND timeFrame, timeLabel, trainsView;
	ScratchBuffers getDispInfoBuffers;
	unsigned int sortColumn;
	int sortOrder;
	size_t maxTextSize;
	bool closing;
	Connection connection;
	winrt::Windows::Foundation::IAsyncAction receiveMessagesAction;
	std::bitset<territory::count> enabledTerritories;
	bool enabledUnknownTerritories;
	std::atomic<DateTimeFormat> dateTimeFormat;

	static HMENU findSubMenuContainingID(HMENU parent, unsigned int id);

	void handleClose();
	static ListViewCompareCallback rawCompareCallback;
	static int compareTrains(const TrainInfo &x, const TrainInfo &y, unsigned int column, int sortOrder);
	winrt::Windows::Foundation::IAsyncAction receiveMessages();
	void updateLayoutAndFont();
	void updateLayout();
	void updateColumnHeaderArrows();
	void updateDateTimeMenuItems();
};
}

#endif