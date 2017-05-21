CC=g++
PKGCONFIG=pkg-config

all: horizon-imp horizon-pool horizon-pool-update horizon-prj horizon-pool-update-parametric horizon-prj-mgr

SRC_COMMON = \
	util/uuid.cpp \
	util/uuid_path.cpp\
	util/sqlite.cpp\
	pool/unit.cpp \
	pool/symbol.cpp \
	pool/part.cpp\
	common/junction.cpp \
	common/line.cpp \
	common/arc.cpp \
	pool/gate.cpp\
	block/net.cpp\
	block/bus.cpp\
	block/block.cpp\
	pool/entity.cpp\
	block/component.cpp\
	schematic/schematic.cpp\
	schematic/sheet.cpp\
	common/text.cpp\
	schematic/line_net.cpp\
	schematic/net_label.cpp\
	schematic/bus_label.cpp\
	schematic/bus_ripper.cpp\
	schematic/schematic_symbol.cpp\
	schematic/power_symbol.cpp\
	schematic/frame.cpp\
	pool/padstack.cpp\
	common/polygon.cpp\
	common/hole.cpp\
	common/shape.cpp\
	common/patch_type_names.cpp\
	pool/package.cpp\
	package/pad.cpp\
	board/board.cpp\
	board/board_package.cpp\
	board/track.cpp\
	board/via_padstack_provider.cpp\
	board/via.cpp\
	board/board_rules.cpp\
	board/rule_hole_size.cpp\
	board/rule_clearance_silk_exp_copper.cpp\
	board/rule_track_width.cpp\
	board/rule_clearance_copper.cpp\
	pool/pool.cpp \
	util/placement.cpp\
	util/util.cpp\
	object_descr.cpp\
	constraints/net_class.cpp\
	constraints/net_classes.cpp\
	constraints/clearance.cpp \
	project/project.cpp\
	resources.cpp\
	gitversion.cpp\
	rules/rules.cpp\
	rules/rule.cpp\
	rules/rule_descr.cpp\
	rules/rule_match.cpp\
	
ifeq ($(OS),Windows_NT)
    SRC_COMMON += util/uuid_win32.cpp
endif
	
SRC_IMP = \
	imp/imp_main.cpp \
	imp/main_window.cpp \
	imp/imp.cpp \
	imp/imp_layer.cpp\
	imp/imp_symbol.cpp\
	imp/imp_schematic.cpp\
	imp/imp_padstack.cpp\
	imp/imp_package.cpp\
	imp/imp_board.cpp\
	imp/tool_popover.cpp\
	imp/selection_filter_dialog.cpp\
	imp/cam_job_dialog.cpp\
	canvas/canvas.cpp \
	canvas/canvas_gl.cpp \
	canvas/canvas_cairo.cpp \
	canvas/grid.cpp \
	canvas/gl_util.cpp \
	canvas/pan.cpp \
	canvas/render.cpp \
	canvas/draw.cpp \
	canvas/text.cpp \
	canvas/hershey_fonts.cpp \
	canvas/box_selection.cpp \
	canvas/selectables.cpp \
	canvas/hover_prelight.cpp\
	canvas/triangle.cpp\
	canvas/image.cpp\
	canvas/selection_filter.cpp\
	canvas/polypartition/polypartition.cpp\
	canvas/marker.cpp\
	canvas/error_polygons.cpp\
	core/core.cpp \
	core/core_properties.cpp\
	core/tool_catalog.cpp\
	core/tool_move.cpp\
	core/tool_place_junction.cpp\
	core/tool_draw_line.cpp\
	core/tool_delete.cpp\
	core/tool_draw_arc.cpp\
	core/tool_map_pin.cpp\
	core/tool_map_symbol.cpp\
	core/tool_draw_line_net.cpp\
	core/tool_add_component.cpp\
	core/tool_place_text.cpp\
	core/tool_place_net_label.cpp\
	core/tool_disconnect.cpp\
	core/tool_bend_line_net.cpp\
	core/tool_move_net_segment.cpp\
	core/tool_place_power_symbol.cpp\
	core/tool_edit_component_pin_names.cpp\
	core/tool_place_bus_label.cpp\
	core/tool_place_bus_ripper.cpp\
	core/tool_manage_buses.cpp\
	core/tool_draw_polygon.cpp\
	core/tool_enter_datum.cpp\
	core/tool_place_hole.cpp\
	core/tool_place_pad.cpp\
	core/tool_paste.cpp\
	core/tool_assign_part.cpp\
	core/tool_map_package.cpp\
	core/tool_draw_track.cpp\
	core/tool_place_via.cpp\
	core/tool_route_track.cpp\
	core/tool_drag_keep_slope.cpp\
	core/tool_add_part.cpp\
	core/tool_smash.cpp\
	core/tool_place_shape.cpp\
	core/tool_edit_shape.cpp\
	core/tool_import_dxf.cpp\
	core/cores.cpp\
	core/clipboard.cpp\
	core/buffer.cpp\
	dialogs/map_pin.cpp\
	dialogs/map_symbol.cpp\
	dialogs/map_package.cpp\
	dialogs/pool_browser_symbol.cpp\
	dialogs/pool_browser_entity.cpp\
	dialogs/pool_browser_padstack.cpp\
	dialogs/pool_browser_part.cpp\
	dialogs/ask_net_merge.cpp\
	dialogs/ask_delete_component.cpp\
	dialogs/select_net.cpp\
	dialogs/component_pin_names.cpp\
	dialogs/manage_buses.cpp\
	dialogs/ask_datum.cpp\
	dialogs/select_via_padstack.cpp\
	dialogs/dialogs.cpp\
	dialogs/pool_browser_box.cpp\
	dialogs/annotate.cpp\
	dialogs/edit_shape.cpp\
	util/sort_controller.cpp\
	core/core_symbol.cpp\
	core/core_schematic.cpp\
	core/core_padstack.cpp\
	core/core_package.cpp\
	core/core_board.cpp\
	property_panels/property_panels.cpp\
	property_panels/property_panel.cpp\
	property_panels/property_editors.cpp\
	property_panels/property_editor.cpp\
	widgets/warnings_box.cpp\
	widgets/net_selector.cpp\
	widgets/sheet_box.cpp \
	widgets/net_button.cpp\
	widgets/layer_box.cpp\
	widgets/spin_button_dim.cpp\
	widgets/chooser_buttons.cpp\
	widgets/cell_renderer_layer_display.cpp\
	widgets/net_class_button.cpp\
	export_pdf.cpp\
	imp/key_sequence.cpp\
	imp/keyseq_dialog.cpp\
	clipper/clipper.cpp\
	obstacle/canvas_obstacle.cpp\
	canvas/canvas_patch.cpp\
	export_gerber/gerber_writer.cpp\
	export_gerber/excellon_writer.cpp\
	export_gerber/gerber_export.cpp\
	export_gerber/canvas_gerber.cpp\
	export_gerber/cam_job.cpp\
	imp/footprint_generator/footprint_generator_window.cpp\
	imp/footprint_generator/footprint_generator_base.cpp\
	imp/footprint_generator/footprint_generator_dual.cpp\
	imp/footprint_generator/footprint_generator_single.cpp\
	imp/footprint_generator/footprint_generator_quad.cpp\
	imp/footprint_generator/svg_overlay.cpp\
	checks/check_runner.cpp\
	checks/check.cpp\
	checks/cache.cpp\
	checks/single_pin_net.cpp\
	imp/checks/checks_window.cpp\
	imp/checks/check_settings_dialog.cpp\
	imp/imp_interface.cpp\
	widgets/pool_browser_part.cpp\
	dxflib/dl_dxf.cpp\
	dxflib/dl_writer_ascii.cpp\
	import_dxf/dxf_importer.cpp\
	imp/rules/rules_window.cpp\
	imp/rules/rule_editor.cpp\
	imp/rules/rule_match_editor.cpp\
	imp/rules/rule_editor_hole_size.cpp\
	imp/rules/rule_editor_clearance_silk_exp_copper.cpp\
	imp/rules/rule_editor_track_width.cpp\
	imp/rules/rule_editor_clearance_copper.cpp\
	rules/rules_with_core.cpp\
	rules/cache.cpp\
	board/board_rules_check.cpp\


SRC_POOL_UTIL = \
	pool-util/util_main.cpp\
	pool-util/part-editor.cpp\
	dialogs/pool_browser_entity.cpp\
	dialogs/pool_browser_package.cpp\
	dialogs/pool_browser_part.cpp\
	dialogs/pool_browser_box.cpp\
	util/sort_controller.cpp\
	widgets/pool_browser_part.cpp\
	
SRC_POOL_UPDATE = \
	pool-update/pool-update.cpp\
	
SRC_POOL_UPDATE_PARA = \
	pool-update-parametric/pool-update-parametric.cpp\
	
SRC_PRJ_UTIL = \
	prj-util/util_main.cpp

SRC_PRJ_MGR = \
	prj-mgr/prj-mgr-main.cpp\
	prj-mgr/prj-mgr-app.cpp\
	prj-mgr/prj-mgr-app_win.cpp\
	prj-mgr/prj-mgr-prefs.cpp\
	prj-mgr/editor_process.cpp\
	prj-mgr/part_browser/part_browser_window.cpp\
	widgets/pool_browser_part.cpp\
	util/sort_controller.cpp\

SRC_ALL = $(sort $(SRC_COMMON) $(SRC_IMP) $(SRC_POOL_UTIL) $(SRC_POOL_UPDATE) $(SRC_PRJ_UTIL) $(SRC_POOL_UPDATE_PARA) $(SRC_PRJ_MGR))

INC = -I. -Iblock -Iboard -Icommon -Iimp -Ipackage -Ipool -Ischematic -Iutil -Iconstraints

DEFINES = -D_USE_MATH_DEFINES

LIBS_COMMON = sqlite3 yaml-cpp
ifneq ($(OS),Windows_NT)
	LIBS_COMMON += uuid
endif
LIBS_ALL = $(LIBS_COMMON) gtkmm-3.0 epoxy cairomm-pdf-1.0 librsvg-2.0 libzmq

OPTIMIZE=-fdata-sections -ffunction-sections
DEBUG   =-g3
CFLAGS  =$(DEBUG) $(DEFINES) $(OPTIMIZE) $(shell pkg-config --cflags $(LIBS_ALL)) -MP -MMD -pthread -Wall -Wshadow -std=c++14
LDFLAGS = -lm -lpthread
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

LDFLAGS_GUI=
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lrpcrt4
    DEFINES += -DWIN32_UUID
    LDFLAGS_GUI = -Wl,-subsystem,windows
endif

# Object files
OBJ_ALL = $(SRC_ALL:.cpp=.o)
OBJ_COMMON = $(SRC_COMMON:.cpp=.o)

resources.cpp: imp.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies imp.gresource.xml)
	$(GLIB_COMPILE_RESOURCES) imp.gresource.xml --target=$@ --sourcedir=. --generate-source

gitversion.cpp: .git/HEAD .git/index
	echo "const char *gitversion = \"$(shell git log -1 --pretty="format:%h %ci %s")\";" > $@

horizon-imp: $(OBJ_COMMON) $(SRC_IMP:.cpp=.o)
	$(CC) $^ $(LDFLAGS) $(LDFLAGS_GUI) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0 epoxy cairomm-pdf-1.0 librsvg-2.0 libzmq) -o $@

horizon-pool: $(OBJ_COMMON) $(SRC_POOL_UTIL:.cpp=.o)
	$(CC) $^ $(LDFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0) -o $@

horizon-pool-update: $(OBJ_COMMON) $(SRC_POOL_UPDATE:.cpp=.o)
	$(CC) $^ $(LDFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) glibmm-2.4 giomm-2.4) -o $@

horizon-pool-update-parametric: $(OBJ_COMMON) $(SRC_POOL_UPDATE_PARA:.cpp=.o)
	$(CC) $^ $(LDFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) glibmm-2.4 giomm-2.4) -o $@

horizon-prj: $(OBJ_COMMON) $(SRC_PRJ_UTIL:.cpp=.o)
	$(CC) $^ $(LDFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) glibmm-2.4 giomm-2.4) -o $@

horizon-prj-mgr: $(OBJ_COMMON) $(SRC_PRJ_MGR:.cpp=.o)
	$(CC) $^ $(LDFLAGS) $(LDFLAGS_GUI) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0 libzmq) -o $@

$(OBJ_ALL): %.o: %.cpp
	$(CC) -c $(INC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJ_ALL) horizon-imp horizon-pool horizon-prj horizon-pool-update horizon-pool-update-parametric horizon-prj-mgr $(OBJ_ALL:.o=.d)

-include  $(OBJ_ALL:.o=.d)

.PHONY: clean
