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
#include "location.h"
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

// Returns a number format suitable for formatting numbers.
NUMBERFMTW numberFormat(unsigned int decimalPlaces) {
	static std::wstring decimalSeparator = getLocaleString(LOCALE_SDECIMAL);
	static std::wstring thousandsSeparator = getLocaleString(LOCALE_STHOUSAND);
	static NUMBERFMTW base = {
		.NumDigits = 0,
		.LeadingZero = getLocaleInteger(LOCALE_ILZERO),
		.Grouping = getLocaleGrouping(),
		.lpDecimalSep = const_cast<wchar_t *>(decimalSeparator.c_str()),
		.lpThousandSep = const_cast<wchar_t *>(thousandsSeparator.c_str()),
		.NegativeOrder = getLocaleInteger(LOCALE_INEGNUMBER),
	};
	NUMBERFMTW ret = base;
	ret.NumDigits = decimalPlaces;
	return ret;
}

// A type that is either integral or floating-point.
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

// Formats a number according to the current locale into a buffer.
//
// The result is stored in the wstring field of the buffers parameter.
template<Numeric T>
void formatNumber(T value, MainWindow::ScratchBuffers &buffers, unsigned int decimalPlaces) {
	static constexpr size_t startBufferSize = std::numeric_limits<T>::digits10 + 3;

	// Write the value, in locale-agnostic raw format, to buffers.string.
	{
		buffers.string.resize(std::max(startBufferSize, buffers.string.capacity()));
		for(;;) {
			std::to_chars_result result;
			if constexpr(std::integral<T>) {
				result = std::to_chars(buffers.string.data(), buffers.string.data() + buffers.string.size(), value);
			} else {
				// GetNumberFormatEx only likes fixed-point, not scientific notation.
				result = std::to_chars(buffers.string.data(), buffers.string.data() + buffers.string.size(), value, std::chars_format::fixed);
			}
			if(result.ec == std::errc()) {
				buffers.string.resize(result.ptr - buffers.string.data());
				break;
			} else if(result.ec == std::errc::value_too_large) {
				// to_chars does not tell us how much space it wanted, so just try doubling.
				buffers.string.resize(buffers.string.size() * 2);
			} else {
				throw std::system_error(std::make_error_code(result.ec));
			}
		}
	}

	// Convert the multibyte string in buffers.string to a wide string in buffers.wstring2.
	for(;;) {
		// Optimistically try an initial conversion with a buffer.
		// Once wstring2 settles to a stable allocation, this will succeed nearly all the time, so is faster.
		// It's slower (three calls instead of two) when the allocation is too small, but that should be rare.
		buffers.wstring2.resize(std::max(startBufferSize, buffers.wstring2.capacity()));
		int written = MultiByteToWideChar(CP_ACP, 0, buffers.string.data(), buffers.string.size(), buffers.wstring2.data(), buffers.wstring2.size());
		if(written) {
			buffers.wstring2.resize(written);
			break;
		}
		if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			winrt::throw_last_error();
		}
		// Buffer is too small. Make another call without a buffer, which will tell us how big a buffer is needed.
		int needed = MultiByteToWideChar(CP_ACP, 0, buffers.string.data(), buffers.string.size(), nullptr, 0);
		if(!needed) {
			winrt::throw_last_error();
		}
		buffers.wstring2.resize(needed);
	}

	// Add proper number formatting.
	NUMBERFMTW fmt = numberFormat(decimalPlaces);
	for(;;) {
		// Optimistically try an initial conversion with a buffer.
		// Once wstring2 settles to a stable allocation, this will succeed nearly all the time, so is faster.
		// It's slower (three calls instead of two) when the allocation is too small, but that should be rare.
		buffers.wstring.resize(std::max(startBufferSize, buffers.wstring.capacity()));
		int written = GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, buffers.wstring2.c_str(), &fmt, buffers.wstring.data(), buffers.wstring.size());
		if(written) {
			buffers.wstring.resize(written - 1 /* NUL */);
			break;
		}
		if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
			winrt::throw_last_error();
		}
		// Buffer is too small. Make another call without a buffer, which will tell us how big a buffer is needed.
		int needed = GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, buffers.wstring2.c_str(), &fmt, nullptr, 0);
		if(!needed) {
			winrt::throw_last_error();
		}
		buffers.wstring.resize(needed);
	}
}

// Metadata about a column of the list.
class Column {
	public:
	// The ID of the string table entry that holds the title of this column.
	unsigned int stringID;

	// Updates a train object to hold a new value from a SOAP update message, returning whether or not the value changed.
	virtual bool update(MainWindow::TrainInfo &dest, const soap::TrainData &source) const = 0;

	// Formats the text for this column, given a scratch buffer which may (but need not) be used.
	virtual const std::wstring &text(const MainWindow::TrainInfo &train, MainWindow::ScratchBuffers &scratch) const = 0;

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
	const std::wstring &text(const MainWindow::TrainInfo &train, MainWindow::ScratchBuffers &) const override final {
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

// Metadata about a list column that holds a numeric value.
template<Numeric T, typename Source = T>
class NumberColumn final : public Column {
	public:
	explicit constexpr NumberColumn(unsigned int stringID, T MainWindow::TrainInfo:: *member, Source soap::TrainData:: *soapMember, unsigned int decimalPlaces) :
		Column(stringID),
		member(member),
		soapMember(soapMember),
		decimalPlaces(decimalPlaces) {
	}

	bool update(MainWindow::TrainInfo &dest, const soap::TrainData &source) const override {
		T newValue = static_cast<T>(source.*soapMember);
		bool ret = dest.*member != newValue;
		dest.*member = newValue;
		return ret;
	}

	const std::wstring &text(const MainWindow::TrainInfo &train, MainWindow::ScratchBuffers &scratch) const override {
		formatNumber(train.*member, scratch, decimalPlaces);
		return scratch.wstring;
	}

	int compare(const MainWindow::TrainInfo &x, const MainWindow::TrainInfo &y) const override {
		T xValue = x.*member;
		T yValue = y.*member;
		return xValue < yValue ? -1 : xValue > yValue ? 1 : 0;
	}

	private:
	// Which member of the TrainInfo holds the number.
	T MainWindow::TrainInfo:: *member;

	// Which member of the SOAP update message holds the number.
	Source soap::TrainData:: *soapMember;

	// How many decimal places to show in this column.
	unsigned int decimalPlaces;
};

// The train length column.
constexpr const NumberColumn<uint32_t> lengthColumn(IDS_MAIN_COLUMN_LENGTH, &MainWindow::TrainInfo::length, &soap::TrainData::length, 0);

// The train weight column.
constexpr const NumberColumn<uint32_t> weightColumn(IDS_MAIN_COLUMN_WEIGHT, &MainWindow::TrainInfo::weight, &soap::TrainData::weight, 0);

// The train HP/t column.
constexpr const NumberColumn<float> horsepowerPerTonColumn(IDS_MAIN_COLUMN_HPT, &MainWindow::TrainInfo::horsepowerPerTon, &soap::TrainData::horsepowerPerTon, 1);

// The train speed column.
constexpr const NumberColumn<int, float> speedColumn(IDS_MAIN_COLUMN_SPEED, &MainWindow::TrainInfo::speed, &soap::TrainData::speed, 0);

// The territory column.
class TerritoryColumn final : public Column {
	public:
	// The only instance of this object.
	static const TerritoryColumn instance;

	explicit constexpr TerritoryColumn() : Column(IDS_MAIN_COLUMN_TERRITORY) {
	}

	bool update(MainWindow::TrainInfo &dest, const soap::TrainData &source) const override {
		std::optional<unsigned int> newValue = territory::idByBlock(source.block);
		if(newValue != dest.territory) {
			dest.territory = newValue;
			return true;
		} else {
			return false;
		}
	}

	const std::wstring &text(const MainWindow::TrainInfo &train, MainWindow::ScratchBuffers &scratch) const override {
		if(train.territory) {
			const std::wstring *n = territory::nameByID(*train.territory);
			if(n) {
				// The territory has a known name.
				return *n;
			} else {
				// The territory does not have a known name. Render it as the integer instead.
				formatNumber(*train.territory, scratch, 0);
				return scratch.wstring;
			}
		} else {
			// The train is in an unsignalled location.
			scratch.wstring.clear();
			return scratch.wstring;
		}
	}

	int compare(const MainWindow::TrainInfo &x, const MainWindow::TrainInfo &y) const override {
		if(x.territory && y.territory) {
			// Both are in signalled locations.
			const std::wstring *xn = territory::nameByID(*x.territory), *yn = territory::nameByID(*y.territory);
			if(xn && yn) {
				// Both have strings, so order by name.
				return xn->compare(*yn);
			} else if(!xn && !yn) {
				// Neither has a string, so order by ID.
				return *x.territory < *y.territory ? -1 : *x.territory > *y.territory ? 1 : 0;
			} else if(xn) {
				// X has a string and Y does not, so order X first.
				return -1;
			} else {
				// Y has a string and X does not, so order Y first.
				return 1;
			}
		} else if(!x.territory && !y.territory) {
			// Both are in unsignalled locations. They are incomparable.
			return 0;
		} else if(x.territory) {
			// X is in an unsignalled location. It comes after Y.
			return 1;
		} else {
			// Y is in an unsignalled location. It comes after X.
			return -1;
		}
	}
};
constinit const TerritoryColumn TerritoryColumn::instance;

// The location column.
class LocationColumn final : public Column {
	public:
	// The only instance of this object.
	static const LocationColumn instance;

	explicit constexpr LocationColumn() :
		Column(IDS_MAIN_COLUMN_LOCATION) {
	}

	bool update(MainWindow::TrainInfo &dest, const soap::TrainData &source) const override {
		bool changed = source.block != dest.block;
		dest.block = source.block;
		if(dest.block != -1 && (location::nameByBlock(dest.block) || !location::nameByBlock(dest.lastNamedBlock))) {			dest.lastNamedBlock = dest.block;
		}
		return changed;
	}

	const std::wstring &text(const MainWindow::TrainInfo &train, MainWindow::ScratchBuffers &scratch) const override {
		if(train.lastNamedBlock == train.block) {
			if(train.block == -1) {
				// The train is, and always has been, in unsignalled territory.
				scratch.wstring.clear();
				return scratch.wstring;
			} else if(const std::wstring *loc = location::nameByBlock(train.block); loc) {
				// We have a name for the current location.
				return *loc;
			} else {
				// We don't have a name for any location, current or historical. Show the raw block ID.
				formatNumber(train.block, scratch, 0);
				return scratch.wstring;
			}
		} else {
			// We don't have a name for where the train is now, but we do have a name for where it used to be. Show that.
			return *location::nameByBlock(train.lastNamedBlock);
		}
	}

	int compare(const MainWindow::TrainInfo &x, const MainWindow::TrainInfo &y) const override {
		if(x.block < y.block) {
			return -1;
		} else if(x.block > y.block) {
			return 1;
		} else {
			return 0;
		}
	}
};
constinit const LocationColumn LocationColumn::instance;

// The crew column.
class CrewColumn final : public Column {
	public:
	// The only instance of this object.
	static const CrewColumn instance;

	explicit constexpr CrewColumn() :
		Column(IDS_MAIN_COLUMN_CREW) {
	}

	bool update(MainWindow::TrainInfo &dest, const soap::TrainData &source) const override {
		bool changed = dest.engineerType != source.engineerType || dest.engineerName != source.engineerName;
		dest.engineerType = source.engineerType;
		dest.engineerName = source.engineerName;
		return changed;
	}

	const std::wstring &text(const MainWindow::TrainInfo &train, MainWindow::ScratchBuffers &) const override {
		return train.engineerName;
	}

	int compare(const MainWindow::TrainInfo &x, const MainWindow::TrainInfo &y) const override {
		if(x.engineerType != y.engineerType) {
			auto mapToSortKey = [](soap::EngineerType t) -> unsigned int {
				switch(t) {
					case soap::EngineerType::NONE:
						return 2;
					case soap::EngineerType::PLAYER:
						return 0;
					case soap::EngineerType::AI:
						return 1;
				}
				return 3;
			};
			return mapToSortKey(x.engineerType) < mapToSortKey(y.engineerType);
		} else {
			return x.engineerName < y.engineerName;
		}
	}
};

constinit const CrewColumn CrewColumn::instance;

// The columns.
static constinit const std::array columnMetadata{
	static_cast<const Column *>(&LeadUnitColumn::instance),
	static_cast<const Column *>(&SymbolColumn::instance),
	static_cast<const Column *>(&lengthColumn),
	static_cast<const Column *>(&weightColumn),
	static_cast<const Column *>(&horsepowerPerTonColumn),
	static_cast<const Column *>(&speedColumn),
	static_cast<const Column *>(&TerritoryColumn::instance),
	static_cast<const Column *>(&LocationColumn::instance),
	static_cast<const Column *>(&CrewColumn::instance),
};

// The age threshold above which trains are removed.
constexpr unsigned int ageThreshold = 5;
}
}

constinit const wchar_t MainWindow::windowClass[] = L"main";

MainWindow::MainWindow(HWND handle, MessagePump &pump, Connection connection) :
	Window(handle, pump),
	trains(),
	driverImageList(nullptr),
	font(nullptr),
	timeFrame(util::createWindowEx(0, WC_BUTTONW, util::loadString(instance(), IDS_MAIN_TIME_FRAME).c_str(), BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, *this, nullptr, instance(), nullptr)),
	timeLabel(util::createWindowEx(0, WC_STATICW, L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, timeFrame, nullptr, instance(), nullptr)),
	trainsView(util::createWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"", LVS_REPORT | LVS_SHAREIMAGELISTS | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, *this, nullptr, instance(), nullptr)),
	getDispInfoBuffers(),
	sortColumn(0),
	sortOrder(1),
	maxTextSize(0),
	closing(false),
	connection(std::move(connection)),
	receiveMessagesAction(receiveMessages()),
	enabledTerritories([]() {decltype(enabledTerritories) b; b.set(); return b; }()),
	enabledUnknownTerritories(true) {
	// Load the driver image list.
	driverImageList.reset(ImageList_LoadImageW(instance(), MAKEINTRESOURCE(IDB_DRIVER_ICONS), 24, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_MONOCHROME));
	if(!driverImageList) {
		winrt::throw_last_error();
	}
	for(int i : {LVSIL_NORMAL, LVSIL_SMALL}) {
		ListView_SetImageList(trainsView, driverImageList.get(), i);
	}

	// Enable menus to report clicks via WM_MENUCOMMAND instead of WM_COMMAND.
	HMENU bar = winrt::check_pointer(GetMenu(*this));
	{
		MENUINFO info{.cbSize = sizeof(info), .fMask = MIM_STYLE};
		winrt::check_bool(GetMenuInfo(bar, &info));
		info.dwStyle |= MNS_NOTIFYBYPOS;
		winrt::check_bool(SetMenuInfo(bar, &info));
	}

	// Populate the View/Territories menu.
	{
		// Find the View/Territories menu. It doesn't have an ID because submenus can't have IDs, so we must search for it by knowing that it contains the Unknown item, which does have an ID.
		HMENU viewMenu = winrt::check_pointer(findSubMenuContainingID(bar, ID_MAIN_MENU_VIEW_TERRITORIES_UNKNOWN));
		HMENU territoriesMenu = winrt::check_pointer(findSubMenuContainingID(viewMenu, ID_MAIN_MENU_VIEW_TERRITORIES_UNKNOWN));

		// Build the list of strings that will be added to it, which is the territory names but in sorted order, and add them. Give each menu item a dwItemData that is the territory ID. Insert these items before the Unknown item.
		std::array<std::pair<const std::wstring *, unsigned int>, territory::count> sortedStrings;
		for(size_t i = 0; i != sortedStrings.size(); ++i) {
			sortedStrings[i].first = territory::nameByIndex(i);
			sortedStrings[i].second = territory::idByIndex(i);
		}
		std::sort(sortedStrings.begin(), sortedStrings.end(),
			[](const std::pair<const std::wstring *, unsigned int> &x, const std::pair<const std::wstring *, unsigned int> &y) -> bool {
				return *x.first < *y.first;
			});
		for(size_t i = 0; i != sortedStrings.size(); ++i) {
			MENUITEMINFOW info{
				.cbSize = sizeof(info),
				.fMask = MIIM_DATA | MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING,
				.fType = MFT_STRING,
				.fState = MFS_CHECKED,
				.wID = ID_MAIN_MENU_VIEW_TERRITORIES_SPECIFIC,
				.dwItemData = sortedStrings[i].second,
				.dwTypeData = const_cast<wchar_t *>(sortedStrings[i].first->c_str()),
			};
			winrt::check_bool(InsertMenuItemW(territoriesMenu, i, TRUE, &info));
		}
	}

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
	updateIcon();
	updateLayoutAndFont();
	updateColumnHeaderArrows();
}

LRESULT MainWindow::windowProc(unsigned int message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_CLOSE:
			handleClose();
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_DPICHANGED:
		{
			const RECT &rect = *reinterpret_cast<const RECT *>(lParam);
			SetWindowPos(*this, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
			updateIcon();
			updateLayoutAndFont();
		}
		return 0;

		case WM_MENUCOMMAND:
		{
			HMENU menu = reinterpret_cast<HMENU>(lParam);
			MENUITEMINFOW info{};
			info.cbSize = sizeof(info);
			info.fMask = MIIM_DATA | MIIM_ID | MIIM_STATE;
			winrt::check_bool(GetMenuItemInfoW(menu, wParam, TRUE, &info));
			switch(info.wID) {
				case ID_MAIN_MENU_FILE_EXIT:
					handleClose();
					break;

				case ID_MAIN_MENU_VIEW_ALWAYS_ON_TOP:
				{
					info.fMask = MIIM_STATE;
					info.fState ^= MFS_CHECKED;
					winrt::check_bool(SetMenuItemInfoW(menu, wParam, TRUE, &info));
					SetWindowPos(*this, (info.fState & MFS_CHECKED) ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				}
				break;

				case ID_MAIN_MENU_VIEW_TERRITORIES_SPECIFIC:
				{
					info.fMask = MIIM_STATE;
					info.fState ^= MFS_CHECKED;
					winrt::check_bool(SetMenuItemInfoW(menu, wParam, TRUE, &info));
					size_t territoryIndex = *territory::indexByID(info.dwItemData);
					enabledTerritories.set(territoryIndex, info.fState & MFS_CHECKED);
				}
				break;

				case ID_MAIN_MENU_VIEW_TERRITORIES_UNKNOWN:
				{
					info.fMask = MIIM_STATE;
					info.fState ^= MFS_CHECKED;
					winrt::check_bool(SetMenuItemInfoW(menu, wParam, TRUE, &info));
					enabledUnknownTerritories = info.fState & MFS_CHECKED;
				}
				break;
			}
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
							info.item.pszText = const_cast<wchar_t *>(columnMetadata[info.item.iSubItem]->text(train, getDispInfoBuffers).c_str());
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

HMENU MainWindow::findSubMenuContainingID(HMENU parent, unsigned int id) {
	int count = GetMenuItemCount(parent);
	if(count < 0) {
		winrt::throw_last_error();
	}
	for(int i = 0; i != count; ++i) {
		MENUITEMINFOW info{.cbSize = sizeof(info), .fMask = MIIM_SUBMENU};
		winrt::check_bool(GetMenuItemInfoW(parent, i, TRUE, &info));
		HMENU candidate = info.hSubMenu;
		if(candidate) {
			info.fMask = MIIM_STATE;
			if(GetMenuItemInfoW(candidate, id, FALSE, &info)) {
				return candidate;
			} else if(GetLastError() == ERROR_MENU_ITEM_NOT_FOUND) {
				// Go on to the next one; this is not a fatal error.
			} else {
				winrt::throw_last_error();
			}
		}
	}
	return nullptr;
};

void MainWindow::handleClose() {
	if(!closing) {
		closing = true;
		receiveMessagesAction.Cancel();
	}
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

			// Check whether the train is in an enabled territory.
			std::optional<unsigned int> territoryID = territory::idByBlock(data->block);
			std::optional<size_t> territoryIndex = territoryID ? territory::indexByID(*territoryID) : std::nullopt;
			bool inEnabledTerritory = territoryIndex ? enabledTerritories[*territoryIndex] : enabledUnknownTerritories;
			if(inEnabledTerritory) {
				// Add an element to the trains map.
				auto [element, added] = trains.emplace(data->id, TrainInfo{});

				// Zero the age of the train, because we just saw an update so it obviously still exists.
				element->second.age = 0;

				// Fill the data provided by Run 8, keeping track of which fields changed.
				std::bitset<columnMetadata.size()> columnsChanged;
				bool crewChanged = false;
				for(size_t i = 0; i != columnMetadata.size(); ++i) {
					columnsChanged[i] = columnMetadata[i]->update(element->second, *data);
					if(columnMetadata[i] == &CrewColumn::instance) {
						crewChanged = columnsChanged[i];
					}
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
						.mask = LVIF_IMAGE | LVIF_PARAM,
						.iItem = newIndex,
						.iImage = static_cast<int>(data->engineerType),
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
				if(crewChanged && newIndex == oldIndex) {
					LVITEMW item = {
						.mask = LVIF_IMAGE,
						.iItem = newIndex,
						.iImage = static_cast<int>(data->engineerType),
					};
					ListView_SetItem(trainsView, &item);
				}
			} else {
				// See if we already have a record of this train, from when it was in a different territory or when this territory was previously enabled.
				if(auto i = trains.find(data->id); i != trains.end()) {
					ListView_DeleteItem(trainsView, ListView_MapIDToIndex(trainsView, i->second.listViewID));
					trains.erase(i);
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