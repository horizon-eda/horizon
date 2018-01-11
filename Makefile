PKGCONFIG=pkg-config

all: horizon-imp horizon-pool horizon-prj horizon-pool-update-parametric horizon-prj-mgr horizon-pool-mgr

SRC_COMMON = \
	src/util/uuid.cpp \
	src/util/uuid_path.cpp\
	src/util/sqlite.cpp\
	src/util/str_util.cpp\
	src/pool/unit.cpp \
	src/pool/symbol.cpp \
	src/pool/part.cpp\
	src/common/common.cpp \
	src/common/junction.cpp \
	src/common/line.cpp \
	src/common/arc.cpp \
	src/common/layer_provider.cpp \
	src/pool/gate.cpp\
	src/block/net.cpp\
	src/block/bus.cpp\
	src/block/block.cpp\
	src/pool/entity.cpp\
	src/block/component.cpp\
	src/schematic/schematic.cpp\
	src/schematic/sheet.cpp\
	src/common/text.cpp\
	src/schematic/line_net.cpp\
	src/schematic/net_label.cpp\
	src/schematic/bus_label.cpp\
	src/schematic/bus_ripper.cpp\
	src/schematic/schematic_symbol.cpp\
	src/schematic/power_symbol.cpp\
	src/schematic/frame.cpp\
	src/schematic/schematic_rules.cpp\
	src/schematic/rule_single_pin_net.cpp\
	src/pool/padstack.cpp\
	src/common/polygon.cpp\
	src/common/hole.cpp\
	src/common/shape.cpp\
	src/common/patch_type_names.cpp\
	src/pool/package.cpp\
	src/package/pad.cpp\
	src/package/package_rules.cpp\
	src/package/rule_package_checks.cpp\
	src/board/board.cpp\
	src/board/board_package.cpp\
	src/board/track.cpp\
	src/board/via_padstack_provider.cpp\
	src/board/via.cpp\
	src/board/plane.cpp\
	src/board/board_rules.cpp\
	src/board/rule_hole_size.cpp\
	src/board/rule_clearance_silk_exp_copper.cpp\
	src/board/rule_track_width.cpp\
	src/board/rule_clearance_copper.cpp\
	src/board/rule_parameters.cpp\
	src/board/rule_via.cpp\
	src/board/rule_clearance_copper_other.cpp\
	src/board/rule_plane.cpp\
	src/board/rule_diffpair.cpp\
	src/board/airwires.cpp\
	src/board/fab_output_settings.cpp\
	src/board/board_hole.cpp\
	src/pool/pool.cpp \
	src/util/placement.cpp\
	src/util/util.cpp\
	src/common/object_descr.cpp\
	src/block/net_class.cpp\
	src/project/project.cpp\
	src/resources.cpp\
	src/gitversion.cpp\
	src/rules/rules.cpp\
	src/rules/rule.cpp\
	src/rules/rule_descr.cpp\
	src/rules/rule_match.cpp\
	src/parameter/program.cpp\
	src/parameter/set.cpp\
	3rd_party/clipper/clipper.cpp\
	src/common/dimension.cpp\
	src/logger/logger.cpp\
	
ifeq ($(OS),Windows_NT)
    SRC_COMMON += src/util/uuid_win32.cpp
endif


SRC_CANVAS = \
	src/canvas/canvas.cpp \
	src/canvas/canvas_gl.cpp \
	src/canvas/canvas_cairo.cpp \
	src/canvas/grid.cpp \
	src/canvas/gl_util.cpp \
	src/canvas/pan.cpp \
	src/canvas/render.cpp \
	src/canvas/draw.cpp \
	src/canvas/text.cpp \
	src/canvas/hershey_fonts.cpp \
	src/canvas/drag_selection.cpp \
	src/canvas/selectables.cpp \
	src/canvas/hover_prelight.cpp\
	src/canvas/triangle.cpp\
	src/canvas/image.cpp\
	src/canvas/selection_filter.cpp\
	3rd_party/polypartition/polypartition.cpp\
	src/canvas/marker.cpp\
	src/canvas/annotation.cpp \
	3rd_party/poly2tri/common/shapes.cpp\
	3rd_party/poly2tri/sweep/cdt.cpp\
	3rd_party/poly2tri/sweep/sweep.cpp\
	3rd_party/poly2tri/sweep/sweep_context.cpp\
	3rd_party/poly2tri/sweep/advancing_front.cpp\

SRC_IMP = \
	src/imp/imp_main.cpp \
	src/imp/main_window.cpp \
	src/imp/imp.cpp \
	src/imp/imp_layer.cpp\
	src/imp/imp_symbol.cpp\
	src/imp/imp_schematic.cpp\
	src/imp/imp_padstack.cpp\
	src/imp/imp_package.cpp\
	src/imp/imp_board.cpp\
	src/imp/tool_popover.cpp\
	src/imp/selection_filter_dialog.cpp\
	$(SRC_CANVAS) \
	src/core/core.cpp \
	src/core/core_properties.cpp\
	src/core/tool_catalog.cpp\
	src/core/tool_move.cpp\
	src/core/tool_place_junction.cpp\
	src/core/tool_draw_line.cpp\
	src/core/tool_delete.cpp\
	src/core/tool_draw_arc.cpp\
	src/core/tool_map_pin.cpp\
	src/core/tool_map_symbol.cpp\
	src/core/tool_draw_line_net.cpp\
	src/core/tool_place_text.cpp\
	src/core/tool_place_net_label.cpp\
	src/core/tool_disconnect.cpp\
	src/core/tool_bend_line_net.cpp\
	src/core/tool_move_net_segment.cpp\
	src/core/tool_place_power_symbol.cpp\
	src/core/tool_edit_symbol_pin_names.cpp\
	src/core/tool_place_bus_label.cpp\
	src/core/tool_place_bus_ripper.cpp\
	src/core/tool_manage_buses.cpp\
	src/core/tool_draw_polygon.cpp\
	src/core/tool_enter_datum.cpp\
	src/core/tool_place_hole.cpp\
	src/core/tool_place_pad.cpp\
	src/core/tool_paste.cpp\
	src/core/tool_assign_part.cpp\
	src/core/tool_map_package.cpp\
	src/core/tool_draw_track.cpp\
	src/core/tool_place_via.cpp\
	src/core/tool_route_track.cpp\
	src/core/tool_drag_keep_slope.cpp\
	src/core/tool_add_part.cpp\
	src/core/tool_smash.cpp\
	src/core/tool_place_shape.cpp\
	src/core/tool_edit_shape.cpp\
	src/core/tool_import_dxf.cpp\
	src/core/tool_edit_parameter_program.cpp\
	src/core/tool_edit_pad_parameter_set.cpp\
	src/core/tool_draw_polygon_rectangle.cpp\
	src/core/tool_draw_line_rectangle.cpp\
	src/core/tool_edit_line_rectangle.cpp\
	src/core/tool_helper_map_symbol.cpp\
	src/core/tool_helper_move.cpp\
	src/core/tool_helper_merge.cpp\
	src/core/tool_edit_via.cpp\
	src/core/tool_rotate_arbitrary.cpp\
	src/core/tool_edit_plane.cpp\
	src/core/tool_update_all_planes.cpp\
	src/core/tool_draw_dimension.cpp\
	src/core/tool_set_diffpair.cpp\
	src/core/tool_select_more.cpp\
	src/core/tool_set_via_net.cpp\
	src/core/tool_lock.cpp\
	src/core/tool_add_vertex.cpp\
	src/core/tool_place_board_hole.cpp\
	src/core/tool_edit_board_hole.cpp\
	src/core/cores.cpp\
	src/core/clipboard.cpp\
	src/core/buffer.cpp\
	src/dialogs/map_pin.cpp\
	src/dialogs/map_symbol.cpp\
	src/dialogs/map_package.cpp\
	src/dialogs/ask_net_merge.cpp\
	src/dialogs/ask_delete_component.cpp\
	src/dialogs/select_net.cpp\
	src/dialogs/symbol_pin_names.cpp\
	src/dialogs/manage_buses.cpp\
	src/dialogs/ask_datum.cpp\
	src/dialogs/ask_datum_string.cpp\
	src/dialogs/select_via_padstack.cpp\
	src/dialogs/dialogs.cpp\
	src/dialogs/annotate.cpp\
	src/dialogs/edit_shape.cpp\
	src/dialogs/manage_net_classes.cpp\
	src/dialogs/manage_power_nets.cpp\
	src/dialogs/edit_parameter_program.cpp\
	src/dialogs/edit_parameter_set.cpp\
	src/dialogs/edit_pad_parameter_set.cpp\
	src/dialogs/edit_via.cpp\
	src/dialogs/pool_browser_dialog.cpp\
	src/dialogs/schematic_properties.cpp\
	src/dialogs/edit_plane.cpp\
	src/dialogs/edit_stackup.cpp\
	src/dialogs/edit_board_hole.cpp\
	src/util/sort_controller.cpp\
	src/core/core_symbol.cpp\
	src/core/core_schematic.cpp\
	src/core/core_padstack.cpp\
	src/core/core_package.cpp\
	src/core/core_board.cpp\
	src/property_panels/property_panels.cpp\
	src/property_panels/property_panel.cpp\
	src/property_panels/property_editor.cpp\
	src/widgets/warnings_box.cpp\
	src/widgets/net_selector.cpp\
	src/widgets/sheet_box.cpp \
	src/widgets/net_button.cpp\
	src/widgets/layer_box.cpp\
	src/widgets/spin_button_dim.cpp\
	src/widgets/chooser_buttons.cpp\
	src/widgets/cell_renderer_layer_display.cpp\
	src/widgets/net_class_button.cpp\
	src/widgets/parameter_set_editor.cpp\
	src/widgets/pool_browser.cpp\
	src/export_pdf/export_pdf.cpp\
	src/imp/key_sequence.cpp\
	src/imp/keyseq_dialog.cpp\
	src/canvas/canvas_patch.cpp\
	src/export_gerber/gerber_writer.cpp\
	src/export_gerber/excellon_writer.cpp\
	src/export_gerber/gerber_export.cpp\
	src/export_gerber/canvas_gerber.cpp\
	src/export_gerber/hash.cpp\
	src/imp/footprint_generator/footprint_generator_window.cpp\
	src/imp/footprint_generator/footprint_generator_base.cpp\
	src/imp/footprint_generator/footprint_generator_dual.cpp\
	src/imp/footprint_generator/footprint_generator_single.cpp\
	src/imp/footprint_generator/footprint_generator_quad.cpp\
	src/imp/footprint_generator/footprint_generator_grid.cpp\
	src/imp/footprint_generator/svg_overlay.cpp\
	src/imp/imp_interface.cpp\
	src/imp/parameter_window.cpp\
	src/widgets/pool_browser_part.cpp\
	src/widgets/pool_browser_entity.cpp\
	src/widgets/pool_browser_padstack.cpp\
	src/widgets/pool_browser_package.cpp\
	src/widgets/pool_browser_padstack.cpp\
	src/widgets/pool_browser_unit.cpp\
	src/widgets/pool_browser_symbol.cpp\
	src/widgets/plane_editor.cpp\
	3rd_party/dxflib/dl_dxf.cpp\
	3rd_party/dxflib/dl_writer_ascii.cpp\
	src/import_dxf/dxf_importer.cpp\
	src/imp/rules/rules_window.cpp\
	src/imp/rules/rule_editor.cpp\
	src/imp/rules/rule_match_editor.cpp\
	src/imp/rules/rule_editor_hole_size.cpp\
	src/imp/rules/rule_editor_clearance_silk_exp_copper.cpp\
	src/imp/rules/rule_editor_track_width.cpp\
	src/imp/rules/rule_editor_clearance_copper.cpp\
	src/imp/rules/rule_editor_single_pin_net.cpp\
	src/imp/rules/rule_editor_via.cpp\
	src/imp/rules/rule_editor_clearance_copper_other.cpp\
	src/imp/rules/rule_editor_plane.cpp\
	src/imp/rules/rule_editor_diffpair.cpp\
	src/imp/rules/rule_editor_package_checks.cpp\
	src/rules/rules_with_core.cpp\
	src/rules/cache.cpp\
	src/board/board_rules_check.cpp\
	src/schematic/schematic_rules_check.cpp\
	src/package/package_rules_check.cpp\
	src/board/plane_update.cpp\
	src/imp/symbol_preview/symbol_preview_window.cpp\
	src/imp/symbol_preview/preview_box.cpp\
	src/util/gtk_util.cpp\
	src/imp/fab_output_window.cpp\
	src/imp/3d_view.cpp\
	src/canvas3d/canvas3d.cpp\
	src/canvas3d/cover.cpp\
	src/canvas3d/wall.cpp\
	src/canvas3d/face.cpp\
	src/canvas3d/background.cpp\
	src/imp/header_button.cpp\
	src/imp/preferences.cpp\
	src/imp/preferences_window.cpp\
	src/pool/pool_cached.cpp\
	src/canvas/canvas_pads.cpp\
	src/util/window_state_store.cpp\
	src/widgets/board_display_options.cpp\
	src/widgets/log_window.cpp\
	src/widgets/log_view.cpp\

SRC_ROUTER = \
	3rd_party/router/router/pns_router.cpp \
	3rd_party/router/router/pns_item.cpp \
	3rd_party/router/router/pns_node.cpp \
	3rd_party/router/router/pns_solid.cpp \
	3rd_party/router/router/pns_optimizer.cpp \
	3rd_party/router/router/pns_topology.cpp \
	3rd_party/router/router/pns_walkaround.cpp \
	3rd_party/router/router/pns_utils.cpp \
	3rd_party/router/router/pns_algo_base.cpp \
	3rd_party/router/router/pns_diff_pair_placer.cpp \
	3rd_party/router/router/pns_diff_pair.cpp \
	3rd_party/router/router/pns_dp_meander_placer.cpp\
	3rd_party/router/router/pns_dragger.cpp\
	3rd_party/router/router/pns_itemset.cpp \
	3rd_party/router/router/pns_line_placer.cpp \
	3rd_party/router/router/pns_line.cpp \
	3rd_party/router/router/pns_via.cpp \
	3rd_party/router/router/pns_logger.cpp \
	3rd_party/router/router/pns_meander_placer_base.cpp\
	3rd_party/router/router/pns_meander_placer.cpp\
	3rd_party/router/router/pns_meander_skew_placer.cpp\
	3rd_party/router/router/pns_meander.cpp\
	3rd_party/router/router/pns_shove.cpp \
	3rd_party/router/router/time_limit.cpp \
	3rd_party/router/router/pns_routing_settings.cpp \
	3rd_party/router/router/pns_sizes_settings.cpp \
	3rd_party/router/common/geometry/shape_line_chain.cpp\
	3rd_party/router/common/geometry/shape.cpp \
	3rd_party/router/common/geometry/shape_collisions.cpp\
	3rd_party/router/common/geometry/seg.cpp\
	3rd_party/router/common/math/math_util.cpp\
	3rd_party/router/wx_compat.cpp\
	src/router/pns_horizon_iface.cpp\
	src/core/tool_route_track_interactive.cpp\


SRC_POOL_UTIL = \
	src/pool-util/util_main.cpp\
	src/pool-update/pool-update.cpp

SRC_POOL_UPDATE_PARA = \
	src/pool-update-parametric/pool-update-parametric.cpp\
	
SRC_PRJ_UTIL = \
	src/prj-util/util_main.cpp

SRC_PRJ_MGR = \
	src/prj-mgr/prj-mgr-main.cpp\
	src/prj-mgr/prj-mgr-app.cpp\
	src/prj-mgr/prj-mgr-app_win.cpp\
	src/prj-mgr/prj-mgr-prefs.cpp\
	src/prj-mgr/pool_cache_window.cpp\
	src/util/editor_process.cpp\
	src/prj-mgr/part_browser/part_browser_window.cpp\
	src/widgets/pool_browser_part.cpp\
	src/util/sort_controller.cpp\
	src/widgets/pool_browser.cpp\
	src/pool-update/pool-update.cpp\
	src/widgets/cell_renderer_layer_display.cpp\
	src/pool/pool_cached.cpp\
	src/util/gtk_util.cpp\
	src/util/window_state_store.cpp\
	src/widgets/recent_item_box.cpp\
	src/util/recent_util.cpp\
	src/widgets/part_preview.cpp\
	src/widgets/entity_preview.cpp\
	src/widgets/preview_canvas.cpp\
	src/widgets/preview_base.cpp\
	$(SRC_CANVAS)\

SRC_POOL_MGR = \
	src/pool-mgr/pool-mgr-main.cpp\
	src/pool-mgr/pool-mgr-app.cpp\
	src/pool-mgr/pool-mgr-app_win.cpp\
	src/pool-mgr/pool_notebook.cpp\
	src/pool-mgr/pool_notebook_units.cpp\
	src/pool-mgr/pool_notebook_symbols.cpp\
	src/pool-mgr/pool_notebook_entities.cpp\
	src/pool-mgr/pool_notebook_padstacks.cpp\
	src/pool-mgr/pool_notebook_packages.cpp\
	src/pool-mgr/pool_notebook_parts.cpp\
	src/pool-mgr/unit_editor.cpp\
	src/pool-mgr/part_editor.cpp\
	src/pool-mgr/entity_editor.cpp\
	src/pool-mgr/editor_window.cpp\
	src/pool-mgr/create_part_dialog.cpp\
	src/pool-mgr/part_wizard/part_wizard.cpp\
	src/pool-mgr/part_wizard/pad_editor.cpp\
	src/pool-mgr/part_wizard/gate_editor.cpp\
	src/pool-mgr/part_wizard/location_entry.cpp\
	src/pool-mgr/duplicate/duplicate_unit.cpp\
	src/pool-mgr/duplicate/duplicate_entity.cpp\
	src/pool-mgr/duplicate/duplicate_part.cpp\
	src/pool-mgr/duplicate/duplicate_window.cpp\
	src/pool-mgr/pool_remote_box.cpp\
	src/pool-mgr/pool_merge_dialog.cpp\
	src/widgets/pool_browser.cpp\
	src/widgets/pool_browser_unit.cpp\
	src/widgets/pool_browser_symbol.cpp\
	src/widgets/pool_browser_entity.cpp\
	src/widgets/pool_browser_padstack.cpp\
	src/widgets/pool_browser_part.cpp\
	src/widgets/pool_browser_package.cpp\
	src/dialogs/pool_browser_dialog.cpp\
	src/widgets/cell_renderer_layer_display.cpp\
	src/util/sort_controller.cpp\
	src/util/editor_process.cpp\
	$(SRC_CANVAS)\
	src/pool-update/pool-update.cpp\
	src/util/gtk_util.cpp\
	src/util/window_state_store.cpp\
	src/util/rest_client.cpp\
	src/util/github_client.cpp\
	src/widgets/recent_item_box.cpp\
	src/util/recent_util.cpp\
	src/widgets/part_preview.cpp\
	src/widgets/entity_preview.cpp\
	src/widgets/preview_canvas.cpp\
	src/widgets/preview_base.cpp\
	src/widgets/unit_preview.cpp\

SRC_PGM_TEST = \
	src/pgm-test/pgm-test.cpp
	
SRC_OCE = \
	src/util/step_importer.cpp


SRC_ALL = $(sort $(SRC_COMMON) $(SRC_IMP) $(SRC_POOL_UTIL) $(SRC_PRJ_UTIL) $(SRC_POOL_UPDATE_PARA) $(SRC_PRJ_MGR) $(SRC_PGM_TEST) $(SRC_POOL_MGR))

INC = -Isrc -I3rd_party

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
OBJ_OCE = $(SRC_OCE:.cpp=.o)

INC_ROUTER = -I3rd_party/router/include/ -I3rd_party/router -I3rd_party
INC_OCE = -I/opt/opencascade/inc/ -I/mingw64/include/oce/ -I/usr/include/oce
LDFLAGS_OCE = -L /opt/opencascade/lib/ -lTKSTEP  -lTKernel  -lTKXCAF -lTKXSBase -lTKBRep -lTKCDF -lTKXDESTEP -lTKLCAF -lTKMath -lTKMesh
ifeq ($(OS),Windows_NT)
	LDFLAGS_OCE += -lTKV3d
endif

src/resources.cpp: imp.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=src --generate-dependencies imp.gresource.xml)
	$(GLIB_COMPILE_RESOURCES) imp.gresource.xml --target=$@ --sourcedir=src --generate-source

src/gitversion.cpp: .git/HEAD .git/index
	echo "const char *gitversion = \"$(shell git log -1 --pretty="format:%h %ci %s")\";" > $@

horizon-imp: $(OBJ_COMMON) $(OBJ_ROUTER) $(OBJ_OCE) $(SRC_IMP:.cpp=.o)
	$(CXX) $^ $(LDFLAGS) $(LDFLAGS_GUI) $(LDFLAGS_OCE) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0 epoxy cairomm-pdf-1.0 librsvg-2.0 libzmq) -o $@

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

$(OBJ_OCE): %.o: %.cpp
	$(CXX) -c $(INC) $(INC_OCE) $(CXXFLAGS) $< -o $@

clean: clean_router
	rm -f $(OBJ_ALL) horizon-imp horizon-pool horizon-prj horizon-pool-mgr horizon-pool-update-parametric horizon-prj-mgr horizon-pgm-test $(OBJ_ALL:.o=.d) resources.cpp gitversion.cpp

clean_router:
	rm -f $(OBJ_ROUTER) $(OBJ_ROUTER:.o=.d)

-include  $(OBJ_ALL:.o=.d)
-include  $(OBJ_ROUTER:.o=.d)

.PHONY: clean clean_router
