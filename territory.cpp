#include "pch.h"
#include "territory.h"
#include "resource.h"
#include "util.h"
#include <algorithm>
#include <array>

namespace trainlist8::territory {
namespace {
// The known territory IDs and their associated string resource IDs.
constexpr std::array<std::pair<unsigned int, unsigned int>, count> resourceIDs{{
	{100, IDS_TERRITORY_MOJAVE},
	{110, IDS_TERRITORY_NEEDLES},
	{120, IDS_TERRITORY_CAJON},
	{130, IDS_TERRITORY_SELIGMAN},
	{150, IDS_TERRITORY_BARSTOW},
	{200, IDS_TERRITORY_SBD},
	{250, IDS_TERRITORY_BAKERSFIELD},
	}};
static_assert(std::is_sorted(resourceIDs.cbegin(), resourceIDs.cend()));

// The loaded string resources, in the same order as resourceIDs.
constinit std::array<std::unique_ptr<std::wstring>, count> strings;
}
}

// Loads the territory name strings.
void trainlist8::territory::init(HINSTANCE instance) {
	for(size_t i = 0; i != resourceIDs.size(); ++i) {
		strings[i].reset(new std::wstring(util::loadString(instance, resourceIDs[i].second)));
	}
}

// Finds the name of a territory, if known, or nullptr if not.
const std::wstring *trainlist8::territory::nameByID(unsigned int territory) {
	auto i = std::lower_bound(resourceIDs.cbegin(), resourceIDs.cend(), territory,
		[](const std::pair<unsigned int, unsigned int> &candidate, unsigned int target) -> bool {
			return candidate.first < target;
		});
	if(i != resourceIDs.cend() && i->first == territory) {
		return strings[i - resourceIDs.cbegin()].get();
	} else {
		return nullptr;
	}
}
