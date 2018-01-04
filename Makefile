PKGCONFIG=pkg-config

all: horizon-imp horizon-pool horizon-prj horizon-pool-update-parametric horizon-prj-mgr horizon-pool-mgr

SRC_COMMON = \
	util/uuid.cpp \
	util/uuid_path.cpp\
	util/sqlite.cpp\
	util/str_util.cpp\
	pool/unit.cpp \
	pool/symbol.cpp \
	pool/part.cpp\
	common/common.cpp \
	common/junction.cpp \
	common/line.cpp \
	common/arc.cpp \
	common/layer_provider.cpp \
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
	schematic/schematic_rules.cpp\
	schematic/rule_single_pin_net.cpp\
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
	board/plane.cpp\
	board/board_rules.cpp\
	board/rule_hole_size.cpp\
	board/rule_clearance_silk_exp_copper.cpp\
	board/rule_track_width.cpp\
	board/rule_clearance_copper.cpp\
	board/rule_parameters.cpp\
	board/rule_via.cpp\
	board/rule_clearance_copper_other.cpp\
	board/rule_plane.cpp\
	board/rule_diffpair.cpp\
	board/airwires.cpp\
	board/fab_output_settings.cpp\
	board/board_hole.cpp\
	pool/pool.cpp \
	util/placement.cpp\
	util/util.cpp\
	object_descr.cpp\
	block/net_class.cpp\
	project/project.cpp\
	resources.cpp\
	gitversion.cpp\
	rules/rules.cpp\
	rules/rule.cpp\
	rules/rule_descr.cpp\
	rules/rule_match.cpp\
	parameter/program.cpp\
	parameter/set.cpp\
	clipper/clipper.cpp\
	common/dimension.cpp\
	logger/logger.cpp\
	
ifeq ($(OS),Windows_NT)
    SRC_COMMON += util/uuid_win32.cpp
endif


SRC_CANVAS = \
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
	canvas/drag_selection.cpp \
	canvas/selectables.cpp \
	canvas/hover_prelight.cpp\
	canvas/triangle.cpp\
	canvas/image.cpp\
	canvas/selection_filter.cpp\
	canvas/polypartition/polypartition.cpp\
	canvas/marker.cpp\
	canvas/annotation.cpp \
	canvas/poly2tri/common/shapes.cpp\
	canvas/poly2tri/sweep/cdt.cpp\
	canvas/poly2tri/sweep/sweep.cpp\
	canvas/poly2tri/sweep/sweep_context.cpp\
	canvas/poly2tri/sweep/advancing_front.cpp\

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
	$(SRC_CANVAS) \
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
	core/tool_place_text.cpp\
	core/tool_place_net_label.cpp\
	core/tool_disconnect.cpp\
	core/tool_bend_line_net.cpp\
	core/tool_move_net_segment.cpp\
	core/tool_place_power_symbol.cpp\
	core/tool_edit_symbol_pin_names.cpp\
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
	core/tool_edit_parameter_program.cpp\
	core/tool_edit_pad_parameter_set.cpp\
	core/tool_draw_polygon_rectangle.cpp\
	core/tool_draw_line_rectangle.cpp\
	core/tool_edit_line_rectangle.cpp\
	core/tool_helper_map_symbol.cpp\
	core/tool_helper_move.cpp\
	core/tool_helper_merge.cpp\
	core/tool_edit_via.cpp\
	core/tool_rotate_arbitrary.cpp\
	core/tool_edit_plane.cpp\
	core/tool_update_all_planes.cpp\
	core/tool_draw_dimension.cpp\
	core/tool_set_diffpair.cpp\
	core/tool_select_more.cpp\
	core/tool_set_via_net.cpp\
	core/tool_lock.cpp\
	core/tool_add_vertex.cpp\
	core/tool_place_board_hole.cpp\
	core/tool_edit_board_hole.cpp\
	core/cores.cpp\
	core/clipboard.cpp\
	core/buffer.cpp\
	dialogs/map_pin.cpp\
	dialogs/map_symbol.cpp\
	dialogs/map_package.cpp\
	dialogs/ask_net_merge.cpp\
	dialogs/ask_delete_component.cpp\
	dialogs/select_net.cpp\
	dialogs/symbol_pin_names.cpp\
	dialogs/manage_buses.cpp\
	dialogs/ask_datum.cpp\
	dialogs/ask_datum_string.cpp\
	dialogs/select_via_padstack.cpp\
	dialogs/dialogs.cpp\
	dialogs/annotate.cpp\
	dialogs/edit_shape.cpp\
	dialogs/manage_net_classes.cpp\
	dialogs/manage_power_nets.cpp\
	dialogs/edit_parameter_program.cpp\
	dialogs/edit_parameter_set.cpp\
	dialogs/edit_pad_parameter_set.cpp\
	dialogs/edit_via.cpp\
	dialogs/pool_browser_dialog.cpp\
	dialogs/schematic_properties.cpp\
	dialogs/edit_plane.cpp\
	dialogs/edit_stackup.cpp\
	dialogs/edit_board_hole.cpp\
	util/sort_controller.cpp\
	core/core_symbol.cpp\
	core/core_schematic.cpp\
	core/core_padstack.cpp\
	core/core_package.cpp\
	core/core_board.cpp\
	property_panels/property_panels.cpp\
	property_panels/property_panel.cpp\
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
	widgets/parameter_set_editor.cpp\
	widgets/pool_browser.cpp\
	export_pdf.cpp\
	imp/key_sequence.cpp\
	imp/keyseq_dialog.cpp\
	canvas/canvas_patch.cpp\
	export_gerber/gerber_writer.cpp\
	export_gerber/excellon_writer.cpp\
	export_gerber/gerber_export.cpp\
	export_gerber/canvas_gerber.cpp\
	export_gerber/hash.cpp\
	imp/footprint_generator/footprint_generator_window.cpp\
	imp/footprint_generator/footprint_generator_base.cpp\
	imp/footprint_generator/footprint_generator_dual.cpp\
	imp/footprint_generator/footprint_generator_single.cpp\
	imp/footprint_generator/footprint_generator_quad.cpp\
	imp/footprint_generator/footprint_generator_grid.cpp\
	imp/footprint_generator/svg_overlay.cpp\
	imp/imp_interface.cpp\
	imp/parameter_window.cpp\
	widgets/pool_browser_part.cpp\
	widgets/pool_browser_entity.cpp\
	widgets/pool_browser_padstack.cpp\
	widgets/pool_browser_package.cpp\
	widgets/pool_browser_padstack.cpp\
	widgets/pool_browser_unit.cpp\
	widgets/pool_browser_symbol.cpp\
	widgets/plane_editor.cpp\
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
	imp/rules/rule_editor_single_pin_net.cpp\
	imp/rules/rule_editor_via.cpp\
	imp/rules/rule_editor_clearance_copper_other.cpp\
	imp/rules/rule_editor_plane.cpp\
	imp/rules/rule_editor_diffpair.cpp\
	rules/rules_with_core.cpp\
	rules/cache.cpp\
	board/board_rules_check.cpp\
	schematic/schematic_rules_check.cpp\
	board/plane_update.cpp\
	imp/symbol_preview/symbol_preview_window.cpp\
	imp/symbol_preview/preview_box.cpp\
	util/gtk_util.cpp\
	imp/fab_output_window.cpp\
	imp/3d_view.cpp\
	canvas3d/canvas3d.cpp\
	canvas3d/cover.cpp\
	canvas3d/wall.cpp\
	imp/header_button.cpp\
	imp/preferences.cpp\
	imp/preferences_window.cpp\
	pool/pool_cached.cpp\
	canvas/canvas_pads.cpp\
	util/window_state_store.cpp\
	widgets/board_display_options.cpp\
	widgets/log_window.cpp\
	widgets/log_view.cpp\

SRC_ROUTER = \
	router/router/pns_router.cpp \
	router/router/pns_item.cpp \
	router/router/pns_node.cpp \
	router/router/pns_solid.cpp \
	router/router/pns_optimizer.cpp \
	router/router/pns_topology.cpp \
	router/router/pns_walkaround.cpp \
	router/router/pns_utils.cpp \
	router/router/pns_algo_base.cpp \
	router/router/pns_diff_pair_placer.cpp \
	router/router/pns_diff_pair.cpp \
	router/router/pns_dp_meander_placer.cpp\
	router/router/pns_dragger.cpp\
	router/router/pns_itemset.cpp \
	router/router/pns_line_placer.cpp \
	router/router/pns_line.cpp \
	router/router/pns_via.cpp \
	router/router/pns_logger.cpp \
	router/router/pns_meander_placer_base.cpp\
	router/router/pns_meander_placer.cpp\
	router/router/pns_meander_skew_placer.cpp\
	router/router/pns_meander.cpp\
	router/router/pns_shove.cpp \
	router/router/time_limit.cpp \
	router/router/pns_routing_settings.cpp \
	router/router/pns_sizes_settings.cpp \
	router/common/geometry/shape_line_chain.cpp\
	router/common/geometry/shape.cpp \
	router/common/geometry/shape_collisions.cpp\
	router/common/geometry/seg.cpp\
	router/common/math/math_util.cpp\
	router/wx_compat.cpp\
	router/pns_horizon_iface.cpp\
	core/tool_route_track_interactive.cpp\


SRC_POOL_UTIL = \
	pool-util/util_main.cpp\
	pool-update/pool-update.cpp

SRC_POOL_UPDATE_PARA = \
	pool-update-parametric/pool-update-parametric.cpp\
	
SRC_PRJ_UTIL = \
	prj-util/util_main.cpp

SRC_PRJ_MGR = \
	prj-mgr/prj-mgr-main.cpp\
	prj-mgr/prj-mgr-app.cpp\
	prj-mgr/prj-mgr-app_win.cpp\
	prj-mgr/prj-mgr-prefs.cpp\
	prj-mgr/pool_cache_window.cpp\
	util/editor_process.cpp\
	prj-mgr/part_browser/part_browser_window.cpp\
	widgets/pool_browser_part.cpp\
	util/sort_controller.cpp\
	widgets/pool_browser.cpp\
	pool-update/pool-update.cpp\
	widgets/cell_renderer_layer_display.cpp\
	pool/pool_cached.cpp\
	util/gtk_util.cpp\
	util/window_state_store.cpp\
	widgets/recent_item_box.cpp\
	util/recent_util.cpp\
	widgets/part_preview.cpp\
	widgets/entity_preview.cpp\
	widgets/preview_canvas.cpp\
	widgets/preview_base.cpp\
	$(SRC_CANVAS)\

SRC_POOL_MGR = \
	pool-mgr/pool-mgr-main.cpp\
	pool-mgr/pool-mgr-app.cpp\
	pool-mgr/pool-mgr-app_win.cpp\
	pool-mgr/pool_notebook.cpp\
	pool-mgr/pool_notebook_units.cpp\
	pool-mgr/pool_notebook_symbols.cpp\
	pool-mgr/pool_notebook_entities.cpp\
	pool-mgr/pool_notebook_padstacks.cpp\
	pool-mgr/pool_notebook_packages.cpp\
	pool-mgr/pool_notebook_parts.cpp\
	pool-mgr/unit_editor.cpp\
	pool-mgr/part_editor.cpp\
	pool-mgr/entity_editor.cpp\
	pool-mgr/editor_window.cpp\
	pool-mgr/create_part_dialog.cpp\
	pool-mgr/part_wizard/part_wizard.cpp\
	pool-mgr/part_wizard/pad_editor.cpp\
	pool-mgr/part_wizard/gate_editor.cpp\
	pool-mgr/part_wizard/location_entry.cpp\
	pool-mgr/duplicate/duplicate_unit.cpp\
	pool-mgr/duplicate/duplicate_entity.cpp\
	pool-mgr/duplicate/duplicate_part.cpp\
	pool-mgr/duplicate/duplicate_window.cpp\
	pool-mgr/pool_remote_box.cpp\
	pool-mgr/pool_merge_dialog.cpp\
	widgets/pool_browser.cpp\
	widgets/pool_browser_unit.cpp\
	widgets/pool_browser_symbol.cpp\
	widgets/pool_browser_entity.cpp\
	widgets/pool_browser_padstack.cpp\
	widgets/pool_browser_part.cpp\
	widgets/pool_browser_package.cpp\
	dialogs/pool_browser_dialog.cpp\
	widgets/cell_renderer_layer_display.cpp\
	util/sort_controller.cpp\
	util/editor_process.cpp\
	$(SRC_CANVAS)\
	pool-update/pool-update.cpp\
	util/gtk_util.cpp\
	util/window_state_store.cpp\
	util/rest_client.cpp\
	util/github_client.cpp\
	widgets/recent_item_box.cpp\
	util/recent_util.cpp\
	widgets/part_preview.cpp\
	widgets/entity_preview.cpp\
	widgets/preview_canvas.cpp\
	widgets/preview_base.cpp\
	widgets/unit_preview.cpp\

SRC_PGM_TEST = \
	pgm-test.cpp


SRC_ALL = $(sort $(SRC_COMMON) $(SRC_IMP) $(SRC_POOL_UTIL) $(SRC_PRJ_UTIL) $(SRC_POOL_UPDATE_PARA) $(SRC_PRJ_MGR) $(SRC_PGM_TEST) $(SRC_POOL_MGR))

INC = -I. -Iblock -Iboard -Icommon -Iimp -Ipackage -Ipool -Ischematic -Iutil -Iconstraints

DEFINES = -D_USE_MATH_DEFINES

LIBS_COMMON = sqlite3 yaml-cpp
ifneq ($(OS),Windows_NT)
	LIBS_COMMON += uuid
endif
LIBS_ALL = $(LIBS_COMMON) gtkmm-3.0 epoxy cairomm-pdf-1.0 librsvg-2.0 libzmq libgit2 libcurl

OPTIMIZE=-fdata-sections -ffunction-sections
DEBUG   =-g3
CXXFLAGS  =$(DEBUG) $(DEFINES) $(OPTIMIZE) $(shell pkg-config --cflags $(LIBS_ALL)) -MP -MMD -pthread -Wall -Wshadow -std=c++14 -O3
LDFLAGS = -lm -lpthread
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

ifeq ($(OS),Windows_NT)
    LDFLAGS += -lrpcrt4
    DEFINES += -DWIN32_UUID
    LDFLAGS_GUI = -Wl,-subsystem,windows
else
    LDFLAGS += -fuse-ld=gold
endif

# Object files
OBJ_ALL = $(SRC_ALL:.cpp=.o)
OBJ_ROUTER = $(SRC_ROUTER:.cpp=.o)
OBJ_COMMON = $(SRC_COMMON:.cpp=.o)

INC_ROUTER = -Irouter/include/ -Irouter

resources.cpp: imp.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies imp.gresource.xml)
	$(GLIB_COMPILE_RESOURCES) imp.gresource.xml --target=$@ --sourcedir=. --generate-source

gitversion.cpp: .git/HEAD .git/index
	echo "const char *gitversion = \"$(shell git log -1 --pretty="format:%h %ci %s")\";" > $@

horizon-imp: $(OBJ_COMMON) $(OBJ_ROUTER) $(SRC_IMP:.cpp=.o)
	$(CXX) $^ $(LDFLAGS) $(LDFLAGS_GUI) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0 epoxy cairomm-pdf-1.0 librsvg-2.0 libzmq) -o $@

horizon-pool: $(OBJ_COMMON) $(SRC_POOL_UTIL:.cpp=.o)
	$(CXX) $^ $(LDFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0) -o $@

horizon-pool-update-parametric: $(OBJ_COMMON) $(SRC_POOL_UPDATE_PARA:.cpp=.o)
	$(CXX) $^ $(LDFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) glibmm-2.4 giomm-2.4) -o $@

horizon-prj: $(OBJ_COMMON) $(SRC_PRJ_UTIL:.cpp=.o)
	$(CXX) $^ $(LDFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) glibmm-2.4 giomm-2.4) -o $@

horizon-prj-mgr: $(OBJ_COMMON) $(SRC_PRJ_MGR:.cpp=.o)
	$(CXX) $^ $(LDFLAGS) $(LDFLAGS_GUI) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0 libzmq epoxy) -o $@

horizon-pool-mgr: $(OBJ_COMMON) $(SRC_POOL_MGR:.cpp=.o)
	$(CXX) $^ $(LDFLAGS) $(LDFLAGS_GUI) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0 epoxy libzmq libcurl libgit2) -o $@

horizon-pgm-test: $(OBJ_COMMON) $(SRC_PGM_TEST:.cpp=.o)
	$(CXX) $^ $(LDFLAGS) $(LDFLAGS_GUI) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) glibmm-2.4 giomm-2.4) -o $@

$(OBJ_ALL): %.o: %.cpp
	$(CXX) -c $(INC) $(CXXFLAGS) $< -o $@

$(OBJ_ROUTER): %.o: %.cpp
	$(CXX) -c $(INC) $(INC_ROUTER) $(CXXFLAGS) $< -o $@

clean: clean_router
	rm -f $(OBJ_ALL) horizon-imp horizon-pool horizon-prj horizon-pool-mgr horizon-pool-update-parametric horizon-prj-mgr horizon-pgm-test $(OBJ_ALL:.o=.d) resources.cpp gitversion.cpp

clean_router:
	rm -f $(OBJ_ROUTER) $(OBJ_ROUTER:.o=.d)

-include  $(OBJ_ALL:.o=.d)
-include  $(OBJ_ROUTER:.o=.d)

.PHONY: clean clean_router
