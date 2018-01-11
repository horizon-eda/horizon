#include "object_descr.hpp"
#include "hole.hpp"
#include "dimension.hpp"
#include "pool/symbol.hpp"

namespace horizon {
	const std::map<ObjectType, ObjectDescription> object_descriptions = {
		{ObjectType::SYMBOL_PIN, {"Symbol Pin", "Symbol Pins", {
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Name", 0}},
			{ObjectProperty::ID::NAME_VISIBLE, {ObjectProperty::Type::BOOL, "Name visible", 1}},
			{ObjectProperty::ID::PAD_VISIBLE, {ObjectProperty::Type::BOOL, "Pad visible", 2}},
			{ObjectProperty::ID::LENGTH, {ObjectProperty::Type::LENGTH, "Length", 3}},
			{ObjectProperty::ID::DOT, {ObjectProperty::Type::BOOL, "Inverted", 4}},
			{ObjectProperty::ID::CLOCK, {ObjectProperty::Type::BOOL, "Clock", 5}},
			{ObjectProperty::ID::SCHMITT, {ObjectProperty::Type::BOOL, "Schmitt", 5}},
			{ObjectProperty::ID::DRIVER, {ObjectProperty::Type::ENUM, "Driver", 5,
				{
					{static_cast<int>(SymbolPin::Decoration::Driver::DEFAULT), "Default"},
					{static_cast<int>(SymbolPin::Decoration::Driver::OPEN_COLLECTOR), "Open Collector"},
					{static_cast<int>(SymbolPin::Decoration::Driver::OPEN_COLLECTOR_PULLUP), "O.C. w/  pullup"},
					{static_cast<int>(SymbolPin::Decoration::Driver::OPEN_EMITTER), "Open Emitter"},
					{static_cast<int>(SymbolPin::Decoration::Driver::OPEN_EMITTER_PULLDOWN), "O.E. w/  pulldown"},
					{static_cast<int>(SymbolPin::Decoration::Driver::TRISTATE), "Tristate"},
				}
			}},
		}}},
		{ObjectType::JUNCTION, {"Junction", "Junctions", {
		}}},
		{ObjectType::INVALID, {"Invalid", "Invalid", {
		}}},
		{ObjectType::SYMBOL, {"Symbol", "Symbols", {
		}}},
		{ObjectType::NET_CLASS, {"Net class", "Net classes", {
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
			{ObjectProperty::ID::DISPLAY_DIRECTIONS, {ObjectProperty::Type::BOOL, "Pin directions", 0}},
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
			{ObjectProperty::ID::FROM_RULES, {ObjectProperty::Type::BOOL, "From rules", 1}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Net", 0}},
			{ObjectProperty::ID::LOCKED, {ObjectProperty::Type::BOOL, "Locked", 2}},
		}}},
		{ObjectType::SHAPE, {"Shape", "Shapes", {
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer", 0}},
			{ObjectProperty::ID::PARAMETER_CLASS, {ObjectProperty::Type::STRING, "Parameter class", 1}},
			{ObjectProperty::ID::POSITION_X, {ObjectProperty::Type::DIM, "Position X", 2}},
			{ObjectProperty::ID::POSITION_Y, {ObjectProperty::Type::DIM, "Position Y", 3}},
			{ObjectProperty::ID::ANGLE, {ObjectProperty::Type::ANGLE, "Angle", 4}},
		}}},
		{ObjectType::LINE, {"Line", "Lines", {
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Width", 0}},
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer", 1}},
		}}},
		{ObjectType::ARC, {"Arc", "Arcs", {
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Width", 0}},
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer", 1}},
		}}},
		{ObjectType::TEXT, {"Text", "Texts", {
			{ObjectProperty::ID::SIZE, {ObjectProperty::Type::LENGTH, "Size", 1}},
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Width", 2}},
			{ObjectProperty::ID::TEXT, {ObjectProperty::Type::STRING, "Text", 0}},
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer", 3}},
			{ObjectProperty::ID::POSITION_X, {ObjectProperty::Type::DIM, "Position X", 4}},
			{ObjectProperty::ID::POSITION_Y, {ObjectProperty::Type::DIM, "Position Y", 5}},
			{ObjectProperty::ID::ANGLE, {ObjectProperty::Type::ANGLE, "Angle", 6}},

		}}},
		{ObjectType::COMPONENT, {"Component", "Components", {
			{ObjectProperty::ID::REFDES, {ObjectProperty::Type::STRING, "Ref. Desig.", 0}},
			{ObjectProperty::ID::VALUE, {ObjectProperty::Type::STRING, "Value", 2}},
			{ObjectProperty::ID::MPN, {ObjectProperty::Type::STRING_RO, "MPN", 1}},
		}}},
		{ObjectType::NET, {"Net", "Nets", {
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING, "Name", 0}},
			{ObjectProperty::ID::IS_POWER, {ObjectProperty::Type::BOOL, "Is power net", 2}},
			{ObjectProperty::ID::NET_CLASS, {ObjectProperty::Type::NET_CLASS, "Net class", 1}},
			{ObjectProperty::ID::DIFFPAIR, {ObjectProperty::Type::STRING_RO, "Diff. pair", 3}},
		}}},
		{ObjectType::NET_LABEL, {"Net label", "Net labels", {
			{ObjectProperty::ID::SIZE, {ObjectProperty::Type::LENGTH, "Size", 2}},
			{ObjectProperty::ID::OFFSHEET_REFS, {ObjectProperty::Type::BOOL, "Offsheet refs", 1}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Net name", 0}},
		}}},
		{ObjectType::POLYGON, {"Polygon", "Polygons", {
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER, "Layer", 0}},
			{ObjectProperty::ID::PARAMETER_CLASS, {ObjectProperty::Type::STRING, "Parameter class", 1}},
			{ObjectProperty::ID::USAGE, {ObjectProperty::Type::STRING_RO, "Usage", 2}},
		}}},
		{ObjectType::HOLE, {"Hole", "Holes", {
			{ObjectProperty::ID::DIAMETER, {ObjectProperty::Type::LENGTH, "Diameter", 0}},
			{ObjectProperty::ID::LENGTH, {ObjectProperty::Type::LENGTH, "Length", 3}},
			{ObjectProperty::ID::PLATED, {ObjectProperty::Type::BOOL, "Plated", 1}},
			{ObjectProperty::ID::SHAPE, {ObjectProperty::Type::ENUM, "Shape", 2,
				{
					{static_cast<int>(Hole::Shape::ROUND), "Round"},
					{static_cast<int>(Hole::Shape::SLOT), "Slot"},
				}
			}},
			{ObjectProperty::ID::PARAMETER_CLASS, {ObjectProperty::Type::STRING, "Parameter class", 4}},
		}}},
		{ObjectType::PAD, {"Pad", "Pads", {
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING, "Name", 0}},
			{ObjectProperty::ID::VALUE, {ObjectProperty::Type::STRING_RO, "Padstack", 1}},
			{ObjectProperty::ID::PAD_TYPE, {ObjectProperty::Type::STRING_RO, "Pad type", 2}},
			{ObjectProperty::ID::ANGLE, {ObjectProperty::Type::ANGLE, "Angle", 5}},
			{ObjectProperty::ID::POSITION_X, {ObjectProperty::Type::DIM, "Position X", 3}},
			{ObjectProperty::ID::POSITION_Y, {ObjectProperty::Type::DIM, "Position Y", 4}},
		}}},
		{ObjectType::BOARD_HOLE, {"Hole", "Holes", {
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Padstack", 0}},
			{ObjectProperty::ID::VALUE, {ObjectProperty::Type::STRING_RO, "Net", 1}},
			{ObjectProperty::ID::PAD_TYPE, {ObjectProperty::Type::STRING_RO, "Type", 2}},
			{ObjectProperty::ID::ANGLE, {ObjectProperty::Type::ANGLE, "Angle", 5}},
			{ObjectProperty::ID::POSITION_X, {ObjectProperty::Type::DIM, "Position X", 3}},
			{ObjectProperty::ID::POSITION_Y, {ObjectProperty::Type::DIM, "Position Y", 4}},
		}}},
		{ObjectType::BOARD_PACKAGE, {"Package", "Packages", {
			{ObjectProperty::ID::FLIPPED, {ObjectProperty::Type::BOOL, "Flipped", 4}},
			{ObjectProperty::ID::REFDES, {ObjectProperty::Type::STRING_RO, "Ref. Desig.", 0}},
			{ObjectProperty::ID::ALTERNATE_PACKAGE, {ObjectProperty::Type::NET_CLASS, "Package", 1}},
			{ObjectProperty::ID::VALUE, {ObjectProperty::Type::STRING_RO, "Value", 3}},
			{ObjectProperty::ID::MPN, {ObjectProperty::Type::STRING_RO, "MPN", 2}},
			{ObjectProperty::ID::ANGLE, {ObjectProperty::Type::ANGLE, "Angle", 7}},
			{ObjectProperty::ID::POSITION_X, {ObjectProperty::Type::DIM, "Position X", 5}},
			{ObjectProperty::ID::POSITION_Y, {ObjectProperty::Type::DIM, "Position Y", 6}},
		}}},
		{ObjectType::TRACK, {"Track", "Tracks", {
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Width", 3}},
			{ObjectProperty::ID::LAYER, {ObjectProperty::Type::LAYER_COPPER, "Layer", 1}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Net", 0}},
			{ObjectProperty::ID::NET_CLASS, {ObjectProperty::Type::STRING_RO, "Net class", 4}},
			{ObjectProperty::ID::WIDTH_FROM_RULES, {ObjectProperty::Type::BOOL, "Width from rules", 2}},
			{ObjectProperty::ID::LOCKED, {ObjectProperty::Type::BOOL, "Locked", 5}},
		}}},
		{ObjectType::PLANE, {"Plane", "Planes", {
			{ObjectProperty::ID::WIDTH, {ObjectProperty::Type::LENGTH, "Min. Width", 2}},
			{ObjectProperty::ID::NAME, {ObjectProperty::Type::STRING_RO, "Net", 0}},
			{ObjectProperty::ID::FROM_RULES, {ObjectProperty::Type::BOOL, "From rules", 1}},
		}}},
		{ObjectType::DIMENSION, {"Dimension", "Dimensions", {
			{ObjectProperty::ID::SIZE, {ObjectProperty::Type::LENGTH, "Size", 0}},
			{ObjectProperty::ID::MODE, {ObjectProperty::Type::ENUM, "Mode", 1,
				{
					{static_cast<int>(Dimension::Mode::DISTANCE), "Distance"},
					{static_cast<int>(Dimension::Mode::HORIZONTAL), "Horizontal"},
					{static_cast<int>(Dimension::Mode::VERTICAL), "Vertical"},
				}
			}},
		}}}
	};

}
