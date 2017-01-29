#include "object_descr.hpp"

namespace horizon {
	const std::map<ObjectType, ObjectDescription> object_descriptions = {
		{ObjectType::SYMBOL_PIN, {"Symbol Pin", "Symbol Pins", {
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Name"}},
			{ObjectProperty::ID::NAME_VISIBLE, {ObjectProperty::Type::BOOL, "Name visible"}},
			{ObjectProperty::ID::PAD_VISIBLE, {ObjectProperty::Type::BOOL, "Pad visible"}},
			{ObjectProperty::ID::LENGTH, {ObjectProperty::Type::LENGTH, "Length"}},
		}}},
		{ObjectType::JUNCTION, {"Junction", "Junctions", {
		}}},
		{ObjectType::LINE_NET, {"Net line", "Net lines", {
		}}},
		{ObjectType::BUS_LABEL, {"Bus label", "Bus labels", {
		}}},
		{ObjectType::BUS_RIPPER, {"Bus ripper", "Bus rippers", {
		}}},
		{ObjectType::SCHEMATIC_SYMBOL, {"Symbol", "Symbols", {
		}}},
		{ObjectType::POWER_SYMBOL, {"Power symbol", "Power symbols", {
		}}},
		{ObjectType::POLYGON_EDGE, {"Polygon edge", "Polygon edges", {
		}}},
		{ObjectType::POLYGON_VERTEX, {"Polygon vertex", "Polygon vertices", {
		}}},
		{ObjectType::POLYGON_ARC_CENTER, {"Polygon arc center", "Polygon arc centers", {
		}}},
		{ObjectType::VIA, {"Via", "Vias", {
		}}},
		{ObjectType::LINE, {"Line", "Lines", {
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Width"}},
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer"}},
		}}},
		{ObjectType::ARC, {"Arc", "Arcs", {
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Width"}},
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer"}},
		}}},
		{ObjectType::TEXT, {"Text", "Texts", {
			{ObjectProperty::ID::SIZE, {ObjectProperty::Type::LENGTH, "Size"}},
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Width"}},
			{ObjectProperty::ID::TEXT, {ObjectProperty::Type::STRING, "Text"}},
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer"}},

		}}},
		{ObjectType::COMPONENT, {"Component", "Components", {
			{ObjectProperty::ID::REFDES, {ObjectProperty::Type::STRING, "Ref. Desig."}},
			{ObjectProperty::ID::VALUE, {ObjectProperty::Type::STRING, "Value"}},
			{ObjectProperty::ID::MPN, {ObjectProperty::Type::STRING_RO, "MPN"}},
		}}},
		{ObjectType::NET, {"Net", "Nets", {
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING, "Name"}},
			{ObjectProperty::ID::IS_POWER, {ObjectProperty::Type::BOOL, "Is power net"}},
			{ObjectProperty::ID::NET_CLASS, {ObjectProperty::Type::NET_CLASS, "Net class"}},
		}}},
		{ObjectType::NET_LABEL, {"Net label", "Net labels", {
			{ObjectProperty::ID::SIZE, {ObjectProperty::Type::LENGTH, "Size"}},
			{ObjectProperty::ID::OFFSHEET_REFS, {ObjectProperty::Type::BOOL, "Offsheet refs"}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Net name"}},
		}}},
		{ObjectType::POLYGON, {"Polygon", "Polygons", {
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer"}},
		}}},
		{ObjectType::HOLE, {"Hole", "Holes", {
			{ObjectProperty::ID::DIAMETER, {ObjectProperty::Type::LENGTH, "Diameter"}},
			{ObjectProperty::ID::PLATED, {ObjectProperty::Type::BOOL, "Plated"}},
		}}},
		{ObjectType::PAD, {"Pad", "Pads", {
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING, "Name"}},
		}}},
		{ObjectType::BOARD_PACKAGE, {"Package", "Packages", {
			{ObjectProperty::ID::FLIPPED, {ObjectProperty::Type::BOOL, "Flipped"}},
			{ObjectProperty::ID::REFDES, {ObjectProperty::Type::STRING_RO, "Ref. Desig."}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Package"}},
		}}},
		{ObjectType::TRACK, {"Track", "Tracks", {
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Width"}},
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER_COPPER, "Layer"}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Net"}},
			{ObjectProperty::ID::NET_CLASS, {ObjectProperty::Type::STRING_RO, "Net class"}},
			{ObjectProperty::ID::WIDTH_FROM_NET_CLASS, {ObjectProperty::Type::BOOL, "Width from net class"}},
		}}},
	};

}
