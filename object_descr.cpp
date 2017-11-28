#include "object_descr.hpp"
#include "hole.hpp"
#include "dimension.hpp"

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
		{ObjectType::INVALID, {"Invalid", "Invalid", {
		}}},
		{ObjectType::SYMBOL, {"Symbol", "Symbols", {
		}}},
		{ObjectType::UNIT, {"Unit", "Units", {
		}}},
		{ObjectType::ENTITY, {"Entity", "Entities", {
		}}},
		{ObjectType::PACKAGE, {"Package", "Packages", {
		}}},
		{ObjectType::PADSTACK, {"Padstack", "Padstacks", {
		}}},
		{ObjectType::PART, {"Part", "Parts", {
		}}},
		{ObjectType::LINE_NET, {"Net line", "Net lines", {
		}}},
		{ObjectType::BUS_LABEL, {"Bus label", "Bus labels", {
		}}},
		{ObjectType::BUS_RIPPER, {"Bus ripper", "Bus rippers", {
		}}},
		{ObjectType::SCHEMATIC_SYMBOL, {"Symbol", "Symbols", {
			{ObjectProperty::ID::DISPLAY_DIRECTIONS, {ObjectProperty::Type::BOOL, "Pin directions"}},
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
			{ObjectProperty::ID::FROM_RULES, {ObjectProperty::Type::BOOL, "From rules"}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Net"}},
			{ObjectProperty::ID::LOCKED, {ObjectProperty::Type::BOOL, "Locked"}},
		}}},
		{ObjectType::SHAPE, {"Shape", "Shapes", {
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer"}},
			{ObjectProperty::ID::PARAMETER_CLASS, {ObjectProperty::Type::STRING, "Parameter class"}},
			{ObjectProperty::ID::POSITION_X, {ObjectProperty::Type::DIM, "Position X"}},
			{ObjectProperty::ID::POSITION_Y, {ObjectProperty::Type::DIM, "Position Y"}},
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
			{ObjectProperty::ID::DIFFPAIR, {ObjectProperty::Type::STRING_RO, "Diff. pair"}},
		}}},
		{ObjectType::NET_LABEL, {"Net label", "Net labels", {
			{ObjectProperty::ID::SIZE, {ObjectProperty::Type::LENGTH, "Size"}},
			{ObjectProperty::ID::OFFSHEET_REFS, {ObjectProperty::Type::BOOL, "Offsheet refs"}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Net name"}},
		}}},
		{ObjectType::POLYGON, {"Polygon", "Polygons", {
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer"}},
			{ObjectProperty::ID::PARAMETER_CLASS, {ObjectProperty::Type::STRING, "Parameter class"}},
			{ObjectProperty::ID::USAGE, {ObjectProperty::Type::STRING_RO, "Usage"}},
		}}},
		{ObjectType::HOLE, {"Hole", "Holes", {
			{ObjectProperty::ID::DIAMETER, {ObjectProperty::Type::LENGTH, "Diameter"}},
			{ObjectProperty::ID::LENGTH, {ObjectProperty::Type::LENGTH, "Length"}},
			{ObjectProperty::ID::PLATED, {ObjectProperty::Type::BOOL, "Plated"}},
			{ObjectProperty::ID::SHAPE, {ObjectProperty::Type::ENUM, "Shape",
				{
					{static_cast<int>(Hole::Shape::ROUND), "Round"},
					{static_cast<int>(Hole::Shape::SLOT), "Slot"},
				}
			}},
			{ObjectProperty::ID::PARAMETER_CLASS, {ObjectProperty::Type::STRING, "Parameter class"}},
		}}},
		{ObjectType::PAD, {"Pad", "Pads", {
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING, "Name"}},
			{ObjectProperty::ID::VALUE, {ObjectProperty::Type::STRING_RO, "Padstack"}},
			{ObjectProperty::ID::PAD_TYPE, {ObjectProperty::Type::STRING_RO, "Pad type"}},
			{ObjectProperty::ID::ANGLE, {ObjectProperty::Type::ANGLE, "Angle"}},
			{ObjectProperty::ID::POSITION_X, {ObjectProperty::Type::DIM, "Position X"}},
			{ObjectProperty::ID::POSITION_Y, {ObjectProperty::Type::DIM, "Position Y"}},
		}}},
		{ObjectType::BOARD_PACKAGE, {"Package", "Packages", {
			{ObjectProperty::ID::FLIPPED, {ObjectProperty::Type::BOOL, "Flipped"}},
			{ObjectProperty::ID::REFDES, {ObjectProperty::Type::STRING_RO, "Ref. Desig."}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Package"}},
			{ObjectProperty::ID::VALUE, {ObjectProperty::Type::STRING_RO, "Value"}},
			{ObjectProperty::ID::MPN, {ObjectProperty::Type::STRING_RO, "MPN"}},
			{ObjectProperty::ID::ANGLE, {ObjectProperty::Type::ANGLE, "Angle"}},
			{ObjectProperty::ID::POSITION_X, {ObjectProperty::Type::DIM, "Position X"}},
			{ObjectProperty::ID::POSITION_Y, {ObjectProperty::Type::DIM, "Position Y"}},
		}}},
		{ObjectType::TRACK, {"Track", "Tracks", {
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Width"}},
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER_COPPER, "Layer"}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Net"}},
			{ObjectProperty::ID::NET_CLASS, {ObjectProperty::Type::STRING_RO, "Net class"}},
			{ObjectProperty::ID::WIDTH_FROM_RULES, {ObjectProperty::Type::BOOL, "Width from rules"}},
			{ObjectProperty::ID::LOCKED, {ObjectProperty::Type::BOOL, "Locked"}},
		}}},
		{ObjectType::PLANE, {"Plane", "Planes", {
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Min. Width"}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Net"}},
			{ObjectProperty::ID::FROM_RULES, {ObjectProperty::Type::BOOL, "From rules"}},
		}}},
		{ObjectType::DIMENSION, {"Dimension", "Dimensions", {
			{ObjectProperty::ID::SIZE, {ObjectProperty::Type::LENGTH, "Size"}},
			{ObjectProperty::ID::MODE, {ObjectProperty::Type::ENUM, "Mode",
				{
					{static_cast<int>(Dimension::Mode::DISTANCE), "Distance"},
					{static_cast<int>(Dimension::Mode::HORIZONTAL), "Horizontal"},
					{static_cast<int>(Dimension::Mode::VERTICAL), "Vertical"},
				}
			}},
		}}}
	};

}
