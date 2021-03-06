#include "pch.h"
#include <algorithm>
#include <bitset>
#include <cassert>
#include <charconv>
#include <concepts>
#include <cstdlib>
#include <cwctype>
#include <limits>
#include <ranges>
#include "error.h"
#include "main_window.h"
#include "resource.h"
#include "soap.h"
#include "util.h"

using trainlist8::MainWindow;

namespace trainlist8 {
namespace {
// Obtains a string element of the current locale.
std::wstring getLocaleString(LCTYPE attribute) {
	int len = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, attribute, nullptr, 0);
	if(!len) {
		winrt::throw_last_error();
	}
	std::wstring buffer(len, L'\0');
	len = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, attribute, buffer.data(), buffer.size());
	if(!len) {
		winrt::throw_last_error();
	}
	buffer.resize(len);
	return buffer;
}

// Obtains an integer element of the current locale.
DWORD getLocaleInteger(LCTYPE attribute) {
	DWORD buffer;
	int len = GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, attribute | LOCALE_RETURN_NUMBER, reinterpret_cast<wchar_t *>(&buffer), sizeof(buffer) / sizeof(wchar_t));
	if(!len) {
		winrt::throw_last_error();
	}
	assert(len * sizeof(wchar_t) == sizeof(buffer));
	return buffer;
}

// Obtains the grouping integer for the current locale.
unsigned int getLocaleGrouping() {
	// Algorithm from <https://devblogs.microsoft.com/oldnewthing/20060418-11/?p=31493>.
	std::wstring str = getLocaleString(LOCALE_SGROUPING);
	std::erase(str, L';');
	unsigned int ui = static_cast<unsigned int>(std::stoul(str));
	if(ui % 10) {
		ui *= 10;
	} else {
		ui /= 10;
	}
	return ui;
}

// Returns a number format suitable for formatting integers.
const NUMBERFMTW &integerFormat() {
	static std::wstring decimalSeparator = getLocaleString(LOCALE_SDECIMAL);
	static std::wstring thousandsSeparator = getLocaleString(LOCALE_STHOUSAND);
	static NUMBERFMTW ret = {
		.NumDigits = 0,
		.LeadingZero = getLocaleInteger(LOCALE_ILZERO),
		.Grouping = getLocaleGrouping(),
		.lpDecimalSep = const_cast<wchar_t *>(decimalSeparator.c_str()),
		.lpThousandSep = const_cast<wchar_t *>(thousandsSeparator.c_str()),
		.NegativeOrder = getLocaleInteger(LOCALE_INEGNUMBER),
	};
	return ret;
}

// Metadata about a column of the list.
class Column {
	public:
	// Returns the ID of the string table entry that holds the title of this column.
	unsigned int stringID;

	// Updates a train object to hold a new value from a SOAP update message, returning whether or not the value changed.
	virtual bool update(MainWindow::TrainInfo &dest, const soap::TrainData &source) const = 0;

	// Formats the text for this column, given a scratch buffer which may (but need not) be used.
	virtual const std::wstring &text(const MainWindow::TrainInfo &train, std::wstring &scratch) const = 0;

	// Compares two trains based on the value in this column.
	virtual int compare(const MainWindow::TrainInfo &x, const MainWindow::TrainInfo &y) const = 0;

	protected:
	explicit constexpr Column(unsigned int stringID) :
		stringID(stringID) {
	}
};

// Metadata about a list column that holds a string value.
class StringColumn : public Column {
	public:
	const std::wstring &text(const MainWindow::TrainInfo &train, std::wstring &) const override final {
		return train.*member;
	}

	int compare(const MainWindow::TrainInfo &x, const MainWindow::TrainInfo &y) const override final {
		return (x.*member).compare(y.*member);
	}

	protected:
	explicit constexpr StringColumn(unsigned int stringID, std::wstring MainWindow::TrainInfo:: *member) :
		Column(stringID),
		member(member) {
	}

	private:
	// Which member of the TrainInfo holds the string.
	std::wstring MainWindow::TrainInfo:: *member;
};

// The lead unit column.
class LeadUnitColumn final : public StringColumn {
	public:
	// The only instance of this object.
	static const LeadUnitColumn instance;

	bool update(MainWindow::TrainInfo &dest, const soap::TrainData &source) const override {
		std::wostringstream oss;
		oss.imbue(std::locale::classic());
		oss << source.railroadInitials << source.locomotiveNumber;
		std::wstring newValue = std::move(oss).str();
		if(dest.leadUnit != newValue) {
			dest.leadUnit = std::move(newValue);
			return true;
		} else {
			return false;
		}
	}

	private:
	explicit constexpr LeadUnitColumn() :
		StringColumn(IDS_MAIN_COLUMN_LEAD_UNIT, &MainWindow::TrainInfo::leadUnit) {
	}
};
constexpr const LeadUnitColumn LeadUnitColumn::instance;

// The train symbol column.
class SymbolColumn final : public StringColumn {
	public:
	// The only instance of this object.
	static const SymbolColumn instance;

	bool update(MainWindow::TrainInfo &dest, const soap::TrainData &source) const override {
		if(dest.symbol != source.symbol) {
			dest.symbol = source.symbol;
			return true;
		} else {
			return false;
		}
	}

	private:
	explicit constexpr SymbolColumn() :
		StringColumn(IDS_MAIN_COLUMN_SYMBOL, &MainWindow::TrainInfo::symbol) {
	}
};
constexpr const SymbolColumn SymbolColumn::instance;

// Metadata about a list column that holds an integer value.
template<std::integral T, typename Source = T>
class IntegerColumn final : public Column {
	public:
	explicit constexpr IntegerColumn(unsigned int stringID, T MainWindow::TrainInfo:: *member, Source soap::TrainData:: *soapMember) :
		Column(stringID),
		member(member),
		soapMember(soapMember) {
	}

	bool update(MainWindow::TrainInfo &dest, const soap::TrainData &source) const {
		T newValue = static_cast<T>(source.*soapMember);
		bool ret = dest.*member != newValue;
		dest.*member = newValue;
		return ret;
	}

	const std::wstring &text(const MainWindow::TrainInfo &train, std::wstring &scratch) const {
		// Write the value, in locale-agnostic raw format, to a multibyte buffer.
		static constexpr size_t bufferSize = std::numeric_limits<T>::digits10 + 3;
		std::array<char, bufferSize> mbBuffer;
		std::to_chars_result result = std::to_chars(mbBuffer.data(), mbBuffer.data() + mbBuffer.size(), train.*member);
		size_t mbLen = result.ptr - mbBuffer.data();

		// Convert the multibyte string to a wide string.
		std::array<wchar_t, bufferSize> wideBuffer;
		int wideWritten = MultiByteToWideChar(CP_ACP, 0, mbBuffer.data(), mbLen, wideBuffer.data(), wideBuffer.size());
		if(!wideWritten) {
			winrt::throw_last_error();
		}
		wideBuffer[wideWritten] = '\0';

		// Add proper number formatting.
		int needed = GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, wideBuffer.data(), &integerFormat(), nullptr, 0);
		if(!needed) {
			winrt::throw_last_error();
		}
		scratch.resize(needed);
		int written = GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, wideBuffer.data(), &integerFormat(), scratch.data(), scratch.size());
		if(!written) {
			winrt::throw_last_error();
		}
		scratch.resize(written);
		return scratch;
	}

	int compare(const MainWindow::TrainInfo &x, const MainWindow::TrainInfo &y) const {
		T xValue = x.*member;
		T yValue = y.*member;
		return xValue < yValue ? -1 : xValue > yValue ? 1 : 0;
	}

	private:
	// Which member of the TrainInfo holds the integer.
	T MainWindow::TrainInfo:: *member;

	// Which member of the SOAP update message holds the integer.
	Source soap::TrainData:: *soapMember;
};

// The train length column.
constexpr const IntegerColumn<uint32_t> lengthColumn(IDS_MAIN_COLUMN_LENGTH, &MainWindow::TrainInfo::length, &soap::TrainData::length);

// The train weight column.
constexpr const IntegerColumn<uint32_t> weightColumn(IDS_MAIN_COLUMN_WEIGHT, &MainWindow::TrainInfo::weight, &soap::TrainData::weight);

// The train speed column.
constexpr const IntegerColumn<int, float> speedColumn(IDS_MAIN_COLUMN_SPEED, &MainWindow::TrainInfo::speed, &soap::TrainData::speed);

// The columns.
static constinit const std::array columnMetadata{
	static_cast<const Column *>(&LeadUnitColumn::instance),
	static_cast<const Column *>(&SymbolColumn::instance),
	static_cast<const Column *>(&lengthColumn),
	static_cast<const Column *>(&weightColumn),
	static_cast<const Column *>(&speedColumn),
};

// The age threshold above which trains are removed.
constexpr unsigned int ageThreshold = 5;
}
}

constinit const wchar_t MainWindow::windowClass[] = L"main";

MainWindow::MainWindow(HWND handle, MessagePump &pump, Connection connection) :
	Window(handle, pump),
	trains(),
	font(nullptr),
	timeFrame(util::createWindowEx(0, WC_BUTTONW, util::loadString(instance(), IDS_MAIN_TIME_FRAME).c_str(), BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, *this, nullptr, instance(), nullptr)),
	timeLabel(util::createWindowEx(0, WC_STATICW, L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, timeFrame, nullptr, instance(), nullptr)),
	trainsView(util::createWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", LVS_REPORT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, *this, nullptr, instance(), nullptr)),
	getDispInfoBuffer(),
	sortColumn(0),
	sortOrder(1),
	maxTextSize(0),
	closing(false),
	connection(std::move(connection)),
	receiveMessagesAction(receiveMessages()) {
	// Register for notification of completion of the receive-messages action.
	auto uiThread = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();
	receiveMessagesAction.Completed([this, uiThread](const winrt::Windows::Foundation::IAsyncAction &action, winrt::Windows::Foundation::AsyncStatus status) {
		// Capture the HRESULT, if there was one.
		winrt::hresult_error error;
		try {
			action.GetResults();
		} catch(const winrt::hresult_error &err) {
			error = err;
		}

		// Dispatch on the UI thread to deal with the result.
		uiThread.TryEnqueue([this, status, error]() {
			switch(status) {
				case winrt::Windows::Foundation::AsyncStatus::Completed:
					// This should never happen; the receiveMessages function never returns normally.
					std::abort();

				case winrt::Windows::Foundation::AsyncStatus::Canceled:
					// This is fine. This happens when the user clicks the close button.
					break;

				case winrt::Windows::Foundation::AsyncStatus::Error:
					// We should show information about the error before terminating.
					if(error.code() == error::noDispatcherPermission) {
						MessageBoxW(*this, util::loadString(instance(), IDS_MAIN_PERMISSION_RESCINDED).c_str(), util::loadString(instance(), IDS_APP_NAME).c_str(), MB_OK | MB_ICONHAND);
					} else {
						MessageBoxW(*this, util::loadAndFormatString(instance(), IDS_MAIN_CONNECTION_ERROR, error.message().c_str()).c_str(), util::loadString(instance(), IDS_APP_NAME).c_str(), MB_OK | MB_ICONHAND);
					}
					break;
			}

			// The application should now terminate.
			DestroyWindow(*this);
			});
		});

	// Set up the train list view.
	{
		static constexpr DWORD styles = LVS_EX_AUTOSIZECOLUMNS | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP;
		ListView_SetExtendedListViewStyleEx(trainsView, styles, styles);
	}
	{
		for(size_t i = 0; i != columnMetadata.size(); ++i) {
			const std::wstring &label = util::loadString(instance(), columnMetadata[i]->stringID);
			LVCOLUMNW col = {
				.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM,
				.cx = 150,
				.pszText = const_cast<wchar_t *>(label.c_str()),
				.iSubItem = static_cast<int>(i),
			};
			ListView_InsertColumn(trainsView, i, &col);
		}
	}

	// Initialize UI layout.
	updateLayoutAndFont();
	updateColumnHeaderArrows();
}

LRESULT MainWindow::windowProc(unsigned int message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_CLOSE:
			if(!closing) {
				closing = true;
				receiveMessagesAction.Cancel();
			}
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_DPICHANGED:
		{
			const RECT &rect = *reinterpret_cast<const RECT *>(lParam);
			SetWindowPos(*this, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
			updateLayoutAndFont();
		}
		return 0;

		case WM_NOTIFY:
		{
			const NMHDR &header = *reinterpret_cast<const NMHDR *>(lParam);
			if(header.hwndFrom == trainsView) {
				switch(header.code) {
					case LVN_COLUMNCLICK:
					{
						const NMLISTVIEW &info = *reinterpret_cast<const NMLISTVIEW *>(lParam);
						unsigned int clickedColumn = info.iSubItem;
						if(clickedColumn == sortColumn) {
							sortOrder *= -1;
						} else {
							sortColumn = clickedColumn;
							sortOrder = 1;
						}
						updateColumnHeaderArrows();
						ListView_SortItems(trainsView, &MainWindow::rawCompareCallback, reinterpret_cast<LPARAM>(this));
					}
					return 0;

					case LVN_GETDISPINFO:
					{
						NMLVDISPINFO &info = *reinterpret_cast<NMLVDISPINFO *>(lParam);
						const TrainInfo &train = *reinterpret_cast<const TrainInfo *>(info.item.lParam);
						if(info.item.mask & LVIF_TEXT) {
							info.item.pszText = const_cast<wchar_t *>(columnMetadata[info.item.iSubItem]->text(train, getDispInfoBuffer).c_str());
						}
					}
					return 0;
				}
			}
		}
		break;

		case WM_SIZE:
			updateLayout();
			return 0;

	}
	return DefWindowProc(*this, message, wParam, lParam);
}

int WINAPI MainWindow::rawCompareCallback(LPARAM param1, LPARAM param2, LPARAM extra) {
	MainWindow &self = *reinterpret_cast<MainWindow *>(extra);
	const TrainInfo &train1 = *reinterpret_cast<const TrainInfo *>(param1);
	const TrainInfo &train2 = *reinterpret_cast<const TrainInfo *>(param2);
	return compareTrains(train1, train2, self.sortColumn, self.sortOrder);
}

int MainWindow::compareTrains(const TrainInfo &x, const TrainInfo &y, unsigned int sortColumn, int sortOrder) {
	return sortOrder * columnMetadata[sortColumn]->compare(x, y);
}

winrt::Windows::Foundation::IAsyncAction MainWindow::receiveMessages() {
	auto uiThread = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();
	auto cancelToken = co_await winrt::get_cancellation_token();
	cancelToken.enable_propagation();

	for(;;) {
		co_await connection.receiveMessage();
		Connection::Message message = *connection.lastMessage();
		if(const soap::SimulationState **statePointer = std::get_if<const soap::SimulationState *>(&message)) {
			const soap::SimulationState *state = *statePointer;

			// Format the current date and time.
			FILETIME ft;
			winrt::check_hresult(WsDateTimeToFileTime(&state->time, &ft, nullptr));
			SYSTEMTIME st;
			winrt::check_bool(FileTimeToSystemTime(&ft, &st));
			int dateLen = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_AUTOLAYOUT | DATE_SHORTDATE, &st, nullptr, nullptr, 0, nullptr);
			if(!dateLen) {
				winrt::throw_last_error();
			}
			int timeLen = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, nullptr, nullptr, 0);
			if(!timeLen) {
				winrt::throw_last_error();
			}
			std::wstring buffer(dateLen + 1 /* space */ + timeLen, L'\0');
			dateLen = GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_AUTOLAYOUT | DATE_SHORTDATE, &st, nullptr, buffer.data(), buffer.size(), nullptr);
			if(!dateLen) {
				winrt::throw_last_error();
			}
			while(dateLen && buffer[dateLen - 1] == '\0') {
				--dateLen;
			}
			assert(static_cast<unsigned int>(dateLen) + 1 < buffer.size());
			buffer[dateLen] = L' ';
			size_t timeStartPos = dateLen + 1;
			timeLen = GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, nullptr, buffer.data() + timeStartPos, buffer.size() - timeStartPos);
			if(!timeLen) {
				winrt::throw_last_error();
			}
			buffer.resize(timeStartPos + timeLen);
			while(!buffer.empty() && buffer.back() == L'\0') {
				buffer.pop_back();
			}

			// Move to the UI thread to update UI controls and modify the trains map.
			co_await uiThread;

			// Show the current date and time.
			winrt::check_bool(SetWindowTextW(timeLabel, buffer.c_str()));

			// Age the trains, removing those over the threshold.
			for(std::pair<const uint32_t, TrainInfo> &i : trains) {
				++i.second.age;
				if(i.second.age > ageThreshold) {
					ListView_DeleteItem(trainsView, ListView_MapIDToIndex(trainsView, i.second.listViewID));
				}
			}
			std::erase_if(trains, [](const std::pair<const uint32_t, TrainInfo> &i) { return i.second.age > ageThreshold; });
		} else if(const soap::TrainData **dataPointer = std::get_if<const soap::TrainData *>(&message)) {
			const soap::TrainData *data = *dataPointer;

			// Move to the UI thread to update UI controls and modify the trains map.
			co_await uiThread;

			// Add an element to the trains map.
			auto [element, added] = trains.emplace(data->id, TrainInfo{});

			// Zero the age of the train, because we just saw an update so it obviously still exists.
			element->second.age = 0;

			// Fill the data provided by Run 8, keeping track of which fields changed.
			std::bitset<columnMetadata.size()> columnsChanged;
			for(size_t i = 0; i != columnMetadata.size(); ++i) {
				columnsChanged[i] = columnMetadata[i]->update(element->second, *data);
			}

			// Calculate where in the list the train should appear.
			int oldIndex = added ? -1 : ListView_MapIDToIndex(trainsView, element->second.listViewID);
			int newIndex;
			if(added || columnsChanged[sortColumn]) {
				// This is a new train or the value in the column by which the list is sorted has changed. A new position needs to be calculated.
				using std::begin;
				auto indexRange = std::ranges::views::iota(0, ListView_GetItemCount(trainsView));
				newIndex = static_cast<int>(std::ranges::lower_bound(
					indexRange,
					element->second,
					[this](const TrainInfo &candidate, const TrainInfo &newTrain) -> bool {
						return compareTrains(candidate, newTrain, sortColumn, sortOrder) < 0;
					},
					[this](const int &candidateIndex) -> const TrainInfo & {
						LVITEMW candidateItem = {.mask = LVIF_PARAM, .iItem = candidateIndex};
						ListView_GetItem(trainsView, &candidateItem);
						return *reinterpret_cast<const TrainInfo *>(candidateItem.lParam);
					}) - begin(indexRange));
			} else {
				// This is an existing train whose sorting key has not changed. It will not move.
				newIndex = oldIndex;
			}

			if(newIndex != oldIndex) {
				// This is a new train, or else the value of the column used for sorting has changed such that it must be repositioned in the list. Insert a row in the proper place, deleting the old row if applicable.
				if(oldIndex >= 0) {
					ListView_DeleteItem(trainsView, oldIndex);
					if(newIndex > oldIndex) {
						--newIndex;
					}
				}
				LVITEMW item = {
					.mask = LVIF_PARAM,
					.iItem = newIndex,
					.lParam = reinterpret_cast<LPARAM>(&element->second),
				};
				ListView_InsertItem(trainsView, &item);
				element->second.listViewID = ListView_MapIndexToID(trainsView, newIndex);

				// All columns need to be updated.
				columnsChanged.set();
			}

			// Update all the columns that changed.
			for(size_t i = 0; i != columnMetadata.size(); ++i) {
				if(columnsChanged[i]) {
					ListView_SetItemText(trainsView, newIndex, i, LPSTR_TEXTCALLBACK);
				}
			}
		}
	}
}

void MainWindow::updateLayoutAndFont() {
	// Set a good font.
	std::unique_ptr<HFONT, util::FontDeleter> newFont = util::createMessageBoxFont(12, dpi());
	for(HWND window : {timeFrame, timeLabel, trainsView}) {
		SendMessage(window, WM_SETFONT, reinterpret_cast<WPARAM>(newFont.get()), TRUE);
	}
	font = std::move(newFont);

	// Lay out the controls.
	updateLayout();
}

void MainWindow::updateLayout() {
	// Lay out the controls.
	int margin = MulDiv(10, dpi(), USER_DEFAULT_SCREEN_DPI);
	int rowHeight = MulDiv(25, dpi(), USER_DEFAULT_SCREEN_DPI);
	RECT clientRect;
	winrt::check_bool(GetClientRect(*this, &clientRect));
	int clientWidth = clientRect.right - clientRect.left;
	int clientHeight = clientRect.bottom - clientRect.top;
	int y = margin;
	MoveWindow(timeFrame, margin, y, clientWidth - 2 * margin, rowHeight * 2, TRUE);
	MoveWindow(timeLabel, margin, margin * 2, clientWidth - 4 * margin, rowHeight, TRUE);
	y += rowHeight * 2 + margin;
	MoveWindow(trainsView, margin, y, clientWidth - 2 * margin, clientHeight - y - margin, TRUE);
}

void MainWindow::updateColumnHeaderArrows() {
	HWND header = ListView_GetHeader(trainsView);
	for(size_t i = 0; i != columnMetadata.size(); ++i) {
		HDITEMW item = {.mask = HDI_FORMAT};
		Header_GetItem(header, i, &item);
		int fmt = item.fmt;
		fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		if(sortColumn == i) {
			if(sortOrder < 0) {
				fmt |= HDF_SORTDOWN;
			} else {
				fmt |= HDF_SORTUP;
			}
		}
		if(fmt != item.fmt) {
			item.fmt = fmt;
			Header_SetItem(header, i, &item);
		}
	}
}