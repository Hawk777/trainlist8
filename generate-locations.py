#!/usr/bin/env python3

"""
Generate the string table resources and lookup table code file for location names.

The input is read from all CSV files in the "location-csvs" subdirectory. The output is written to
"location.cpp" and "location.rc".

Each input CSV file is expected to have three columns and a header row. The columns should be
"Route", "Block ID", and "Location". The "Route" should be the name of a route as in the
_TERRITORIES dictionary. The "Block ID" should be one or more numeric block IDs, excluding the
leading three digits which indicate territory, separated by commas and/or whitespace. The
"Location" should be the string to display for those blocks.
"""

import argparse
import csv
import math
import pathlib
import re


_TERRITORIES = {
	"Mojave Sub": 100,
}

_COMMA_OR_SPACE = re.compile("[, ]")

_FIRST_STRING_ID = 10000


def main():
	"""The application entry point."""
	csv_dir = pathlib.Path("location-csvs")
	cpp_file = pathlib.Path("location.cpp")
	rc_file = pathlib.Path("location.rc")

	# Load a mapping from full block ID to location string.
	mapping = {}
	for csv_file in csv_dir.iterdir():
		with csv_file.open("r", newline="") as fp:
			for row in csv.DictReader(fp):
				route = row["Route"]
				block_ids = row["Block ID"]
				location = row["Location"]
				route_id = _TERRITORIES[route]
				for block_id in (int(part) for part in _COMMA_OR_SPACE.split(block_ids)):
					block_id_digits = (math.floor(math.log10(block_id)) + 1) if block_id != 0 else 1
					full_id = route_id * 10 ** block_id_digits + block_id
					if full_id in mapping:
						raise ValueError(f"Full block ID {full_id} appears more than once")
					mapping[full_id] = location

	# Generate the ID numbers of the distinct strings.
	string_ids = {}
	strings_by_id = {}
	next_id = _FIRST_STRING_ID
	for location in sorted(mapping.values()):
		if location not in string_ids:
			string_ids[location] = next_id
			strings_by_id[next_id] = location
			next_id += 1

	# Generate the block-ID-to-string-ID mapping table source code.
	cpp_text = f"""#include "pch.h"
#include "location.h"
#include "util.h"
#include <algorithm>
#include <array>
#include <memory>
#include <utility>

namespace trainlist8::location {{
namespace {{
constexpr size_t count = {len(mapping)};

constexpr std::array<std::pair<int32_t, unsigned int>, count> blockIDToLocationStringID{{{{
"""
	for block_id, string in sorted(mapping.items()):
		cpp_text += f"\t{{{block_id}, {string_ids[string]}}},\n"
	cpp_text += f"""
}}}};

static_assert(std::is_sorted(blockIDToLocationStringID.cbegin(), blockIDToLocationStringID.cend()));

// The loaded string resources, in the same order as blockIDToLocationStringID.
constinit std::array<std::unique_ptr<std::wstring>, count> strings;
}}
}}

// Loads the location name strings.
void trainlist8::location::init(HINSTANCE instance) {{
	for(size_t i = 0; i != blockIDToLocationStringID.size(); ++i) {{
		strings[i].reset(new std::wstring(util::loadString(instance, blockIDToLocationStringID[i].second)));
	}}
}}

// Finds the name of a location, if known, or nullptr if not.
const std::wstring *trainlist8::location::nameByBlock(int32_t block) {{
	auto i = std::lower_bound(blockIDToLocationStringID.cbegin(), blockIDToLocationStringID.cend(), block,
		[](const std::pair<int32_t, unsigned int> &candidate, int32_t target) -> bool {{
			return candidate.first < target;
		}});
	if(i != blockIDToLocationStringID.cend() && i->first == block) {{
		return strings[i - blockIDToLocationStringID.cbegin()].get();
	}} else {{
		return nullptr;
	}}
}}
"""

	# Generate the resource script.
	rc_text = """#include "winres.h"
LANGUAGE LANG_ENGLISH, SUBLANG_NEUTRAL
STRINGTABLE
BEGIN
"""
	for string_id, string in sorted(strings_by_id.items()):
		string_escaped = string.replace("\"", "\"\"")
		rc_text += f"\t{string_id} \"{string_escaped}\"\n"
	rc_text += "END\n"

	# Write the output files.
	cpp_file.write_text(cpp_text)
	rc_file.write_text(rc_text)


if __name__ == "__main__":
	main()