PKGCONFIG=pkg-config
BUILDDIR = build
PROGS    = $(addprefix $(BUILDDIR)/horizon-,imp pool prj eda)

all: $(PROGS)
pymodule: $(BUILDDIR)/horizon.so

.PHONY: all pymodule

SRC_COMMON = \
	src/util/uuid.cpp \
	src/util/uuid_path.cpp\
	src/util/sqlite.cpp\
	src/util/str_util.cpp\
	src/pool/pool_manager.cpp \
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
	src/block/bom.cpp\
	src/block/bom_export_settings.cpp\
	src/schematic/schematic.cpp\
	src/schematic/sheet.cpp\
	src/common/text.cpp\
	src/schematic/line_net.cpp\
	src/schematic/net_label.cpp\
	src/schematic/bus_label.cpp\
	src/schematic/bus_ripper.cpp\
	src/schematic/schematic_symbol.cpp\
	src/schematic/power_symbol.cpp\
	src/schematic/schematic_rules.cpp\
	src/schematic/rule_single_pin_net.cpp\
	src/common/pdf_export_settings.cpp\
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
	src/board/rule_preflight_checks.cpp\
	src/board/rule_clearance_copper_keepout.cpp\
	src/board/airwires.cpp\
	src/board/fab_output_settings.cpp\
	src/board/board_hole.cpp\
	src/board/connection_line.cpp\
	src/pool/pool.cpp \
	src/util/placement.cpp\
	src/util/util.cpp\
	src/util/csv.cpp\
	src/common/object_descr.cpp\
	src/block/net_class.cpp\
	src/project/project.cpp\
	src/util/version.cpp\
	src/rules/rules.cpp\
	src/rules/rule.cpp\
	src/rules/rule_descr.cpp\
	src/rules/rule_match.cpp\
	src/rules/rule_match_keepout.cpp\
	src/parameter/program.cpp\
	src/parameter/set.cpp\
	3rd_party/clipper/clipper.cpp\
	src/common/dimension.cpp\
	src/logger/logger.cpp\
	src/parameter/program_polygon.cpp\
	src/frame/frame.cpp\
	src/common/keepout.cpp\
	src/board/board_layers.cpp\
	src/pool/pool_parametric.cpp \

ifeq ($(OS),Windows_NT)
    SRC_COMMON += src/util/uuid_win32.cpp
endif
SRC_COMMON_GEN += $(GENDIR)/resources.cpp
SRC_COMMON_GEN += $(GENDIR)/version_gen.cpp


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
	src/canvas/selectables_renderer.cpp \
	src/canvas/hover_prelight.cpp\
	src/canvas/triangle.cpp\
	src/canvas/image.cpp\
	src/canvas/selection_filter.cpp\
	3rd_party/polypartition/polypartition.cpp\
	src/canvas/marker.cpp\
	src/canvas/annotation.cpp \
	src/canvas/fragment_cache.cpp \
	src/util/text_data.cpp \
	3rd_party/poly2tri/common/shapes.cpp\
	3rd_party/poly2tri/sweep/cdt.cpp\
	3rd_party/poly2tri/sweep/sweep.cpp\
	3rd_party/poly2tri/sweep/sweep_context.cpp\
	3rd_party/poly2tri/sweep/advancing_front.cpp\
	src/util/msd.cpp\
	src/util/msd_animator.cpp\
	src/canvas/appearance.cpp\

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
	src/imp/imp_frame.cpp\
	src/imp/tool_popover.cpp\
	src/imp/selection_filter_dialog.cpp\
	src/imp/action.cpp\
	src/imp/action_catalog.cpp\
	$(SRC_CANVAS) \
	src/core/core.cpp \
	src/core/core_properties.cpp\
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
	src/core/tool_generate_courtyard.cpp\
	src/core/tool_set_group.cpp\
	src/core/tool_copy_placement.cpp\
	src/core/tool_copy_tracks.cpp\
	src/core/tool_swap_nets.cpp\
	src/core/tool_line_loop_to_polygon.cpp\
	src/core/tool_change_unit.cpp\
	src/core/tool_helper_line_width_setting.cpp\
	src/core/tool_set_nc_all.cpp\
	src/core/tool_set_nc.cpp\
	src/core/tool_add_keepout.cpp\
	src/core/tool_helper_draw_net_setting.cpp\
	src/core/tool_helper_get_symbol.cpp\
	src/core/tool_change_symbol.cpp\
	src/core/tool_place_refdes_and_value.cpp\
	src/core/tool_helper_restrict.cpp\
	src/core/tool_draw_polygon_circle.cpp\
	src/core/tool_draw_connection_line.cpp\
	src/core/tool_backannotate_connection_lines.cpp\
	src/core/tool_import_kicad_package.cpp\
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
	src/dialogs/edit_frame.cpp\
	src/dialogs/edit_keepout.cpp\
	src/dialogs/select_group_tag.cpp\
	src/dialogs/ask_datum_angle.cpp\
	src/util/sort_controller.cpp\
	src/core/core_symbol.cpp\
	src/core/core_schematic.cpp\
	src/core/core_padstack.cpp\
	src/core/core_package.cpp\
	src/core/core_board.cpp\
	src/core/core_frame.cpp\
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
	src/widgets/cell_renderer_color_box.cpp\
	src/widgets/net_class_button.cpp\
	src/widgets/parameter_set_editor.cpp\
	src/widgets/pool_browser.cpp\
	src/widgets/component_selector.cpp\
	src/widgets/component_button.cpp\
	src/widgets/preview_canvas.cpp\
	src/widgets/about_dialog.cpp\
	src/export_pdf/canvas_pdf.cpp\
	src/export_pdf/export_pdf.cpp\
	src/export_pdf/export_pdf_board.cpp\
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
	src/imp/footprint_generator/footprint_generator_footag.cpp\
	src/imp/footprint_generator/footag/display.cpp\
	src/imp/imp_interface.cpp\
	src/imp/parameter_window.cpp\
	src/widgets/pool_browser_part.cpp\
	src/widgets/pool_browser_entity.cpp\
	src/widgets/pool_browser_padstack.cpp\
	src/widgets/pool_browser_package.cpp\
	src/widgets/pool_browser_padstack.cpp\
	src/widgets/pool_browser_unit.cpp\
	src/widgets/pool_browser_symbol.cpp\
	src/widgets/pool_browser_frame.cpp\
	src/widgets/plane_editor.cpp\
	src/widgets/title_block_values_editor.cpp\
	3rd_party/dxflib/dl_dxf.cpp\
	3rd_party/dxflib/dl_writer_ascii.cpp\
	src/import_dxf/dxf_importer.cpp\
	src/imp/rules/rules_window.cpp\
	src/imp/rules/rule_editor.cpp\
	src/imp/rules/rule_match_editor.cpp\
	src/imp/rules/rule_match_keepout_editor.cpp\
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
	src/imp/rules/rule_editor_clearance_copper_keepout.cpp\
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
	src/imp/bom_export_window.cpp\
	src/imp/pdf_export_window.cpp\
	src/imp/3d_view.cpp\
	src/imp/step_export_window.cpp\
	src/imp/tuning_window.cpp\
	src/canvas3d/canvas3d.cpp\
	src/canvas3d/cover.cpp\
	src/canvas3d/wall.cpp\
	src/canvas3d/face.cpp\
	src/canvas3d/background.cpp\
	src/imp/header_button.cpp\
	src/preferences/preferences.cpp\
	src/pool/pool_cached.cpp\
	src/canvas/canvas_pads.cpp\
	src/util/window_state_store.cpp\
	src/widgets/board_display_options.cpp\
	src/widgets/log_window.cpp\
	src/widgets/log_view.cpp\
	src/imp/hud_util.cpp\
	src/util/pool_completion.cpp\
	src/export_bom/export_bom.cpp\
	src/widgets/unplaced_box.cpp\
	src/widgets/tag_entry.cpp\
	src/widgets/layer_help_box.cpp\
	src/preferences/preferences_util.cpp\
	src/preferences/preferences_provider.cpp\
	src/widgets/spin_button_angle.cpp\
	src/util/exception_util.cpp\
	src/util/status_dispatcher.cpp\
	src/util/export_file_chooser.cpp\
	3rd_party/sexpr/sexpr_parser.cpp\
	3rd_party/sexpr/sexpr.cpp\
	src/util/kicad_package_parser.cpp\
	src/widgets/pool_browser_button.cpp\

SRC_IMPC = \
	3rd_party/footag/wiz.c\
	3rd_party/footag/wiz_setref.c\
	3rd_party/footag/hint.c\
	3rd_party/footag/chip.c\
	3rd_party/footag/chiparray.c\
	3rd_party/footag/molded.c\
	3rd_party/footag/capae.c\
	3rd_party/footag/soic.c\
	3rd_party/footag/sod.c\
	3rd_party/footag/soj.c\
	3rd_party/footag/qfp.c\
	3rd_party/footag/son.c\
	3rd_party/footag/qfn.c\
	3rd_party/footag/pson.c\
	3rd_party/footag/pqfn.c\
	3rd_party/footag/bga.c\
	3rd_party/footag/sot223.c\
	3rd_party/footag/sot23.c\
	3rd_party/footag/dip.c\
	3rd_party/footag/sip.c\
	3rd_party/footag/pga.c\
	3rd_party/footag/ipc7351b/ipc7351b.c\
	3rd_party/footag/ipc7351b/table.c\
	3rd_party/footag/ipc7251draft1/ipc7251draft1.c\

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
	3rd_party/router/common/geometry/shape_arc.cpp\
	3rd_party/router/common/geometry/shape.cpp \
	3rd_party/router/common/geometry/shape_collisions.cpp\
	3rd_party/router/common/geometry/seg.cpp\
	3rd_party/router/common/geometry/geometry_utils.cpp\
	3rd_party/router/common/math/math_util.cpp\
	3rd_party/router/wx_compat.cpp\
	src/router/pns_horizon_iface.cpp\
	src/core/tool_route_track_interactive.cpp\


SRC_POOL_UTIL = \
	src/pool-util/util_main.cpp\
	src/pool-update/pool-update.cpp\
	src/pool-update/graph.cpp\
	src/pool-update/pool-update_parametric.cpp\

SRC_PRJ_UTIL = \
	src/prj-util/util_main.cpp

SRC_POOL_PRJ_MGR = \
	src/pool-prj-mgr/pool-prj-mgr-main.cpp\
	src/pool-prj-mgr/pool-prj-mgr-app.cpp\
	src/pool-prj-mgr/pool-prj-mgr-app_win.cpp\
	src/pool-prj-mgr/pool-prj-mgr-app_win_download.cpp\
	src/pool-prj-mgr/pool-prj-mgr-process.cpp\
	src/pool-prj-mgr/pool-mgr/pool_notebook.cpp\
	src/pool-prj-mgr/pool-mgr/pool_notebook_units.cpp\
	src/pool-prj-mgr/pool-mgr/pool_notebook_symbols.cpp\
	src/pool-prj-mgr/pool-mgr/pool_notebook_entities.cpp\
	src/pool-prj-mgr/pool-mgr/pool_notebook_padstacks.cpp\
	src/pool-prj-mgr/pool-mgr/pool_notebook_packages.cpp\
	src/pool-prj-mgr/pool-mgr/pool_notebook_parts.cpp\
	src/pool-prj-mgr/pool-mgr/pool_notebook_frames.cpp\
	src/pool-prj-mgr/pool-mgr/editors/unit_editor.cpp\
	src/pool-prj-mgr/pool-mgr/editors/part_editor.cpp\
	src/pool-prj-mgr/pool-mgr/editors/parametric.cpp\
	src/pool-prj-mgr/pool-mgr/editors/entity_editor.cpp\
	src/pool-prj-mgr/pool-mgr/editors/editor_window.cpp\
	src/pool-prj-mgr/pool-mgr/create_part_dialog.cpp\
	src/pool-prj-mgr/pool-mgr/part_wizard/part_wizard.cpp\
	src/pool-prj-mgr/pool-mgr/part_wizard/pad_editor.cpp\
	src/pool-prj-mgr/pool-mgr/part_wizard/gate_editor.cpp\
	src/pool-prj-mgr/pool-mgr/part_wizard/location_entry.cpp\
	src/pool-prj-mgr/pool-mgr/duplicate/duplicate_unit.cpp\
	src/pool-prj-mgr/pool-mgr/duplicate/duplicate_entity.cpp\
	src/pool-prj-mgr/pool-mgr/duplicate/duplicate_part.cpp\
	src/pool-prj-mgr/pool-mgr/duplicate/duplicate_window.cpp\
	src/pool-prj-mgr/pool-mgr/pool_remote_box.cpp\
	src/pool-prj-mgr/pool-mgr/pool_settings_box.cpp\
	src/pool-prj-mgr/pool-mgr/pool_merge_dialog.cpp\
	src/pool-prj-mgr/pool-mgr/pool_update_error_dialog.cpp\
	src/pool-prj-mgr/pool-mgr/pool_git_box.cpp\
	src/widgets/pool_browser.cpp\
	src/widgets/pool_browser_unit.cpp\
	src/widgets/pool_browser_symbol.cpp\
	src/widgets/pool_browser_entity.cpp\
	src/widgets/pool_browser_padstack.cpp\
	src/widgets/pool_browser_part.cpp\
	src/widgets/pool_browser_package.cpp\
	src/widgets/pool_browser_frame.cpp\
	src/widgets/pool_browser_parametric.cpp\
	src/dialogs/pool_browser_dialog.cpp\
	src/widgets/cell_renderer_color_box.cpp\
	src/widgets/where_used_box.cpp\
	src/util/sort_controller.cpp\
	src/util/editor_process.cpp\
	$(SRC_CANVAS)\
	src/pool-update/pool-update.cpp\
	src/pool-update/pool-update_parametric.cpp\
	src/pool-update/graph.cpp\
	src/util/gtk_util.cpp\
	src/util/window_state_store.cpp\
	src/util/http_client.cpp\
	src/util/github_client.cpp\
	src/widgets/recent_item_box.cpp\
	src/util/recent_util.cpp\
	src/widgets/part_preview.cpp\
	src/widgets/entity_preview.cpp\
	src/widgets/preview_canvas.cpp\
	src/widgets/preview_base.cpp\
	src/widgets/unit_preview.cpp\
	src/widgets/symbol_preview.cpp\
	src/util/pool_completion.cpp\
	src/pool/pool_cached.cpp\
	src/pool-prj-mgr/prj-mgr/prj-mgr_views.cpp\
	src/pool-prj-mgr/prj-mgr/pool_cache_window.cpp\
	src/pool-prj-mgr/prj-mgr/part_browser/part_browser_window.cpp\
	src/pool-prj-mgr/prj-mgr/pool_cache_window.cpp\
	src/pool-prj-mgr/close_utils.cpp\
	src/pool-prj-mgr/prj-mgr/pool_cache_cleanup_dialog.cpp\
	src/pool-prj-mgr/pool-mgr/view_create_pool.cpp\
	src/widgets/pool_chooser.cpp\
	src/preferences/preferences.cpp\
	src/preferences/preferences_provider.cpp\
	src/preferences/preferences_util.cpp\
	src/pool-prj-mgr/preferences_window.cpp\
	src/pool-prj-mgr/preferences_window_keys.cpp\
	src/pool-prj-mgr/preferences_window_canvas.cpp\
	src/pool-prj-mgr/preferences_window_pool.cpp\
	src/imp/action.cpp\
	src/imp/action_catalog.cpp\
	src/widgets/unit_info_box.cpp\
	src/widgets/entity_info_box.cpp\
	src/widgets/padstack_preview.cpp\
	src/widgets/tag_entry.cpp\
	src/widgets/about_dialog.cpp\
	src/util/status_dispatcher.cpp\
	src/util/exception_util.cpp\
	src/widgets/package_info_box.cpp\
	src/widgets/pool_browser_button.cpp\

SRC_PGM_TEST = \
	src/pgm-test/pgm-test.cpp

SRC_GEN_PKG = \
	src/gen-pkg/gen-pkg.cpp

SRC_OCE = \
	src/util/step_importer.cpp\
	src/export_step/export_step.cpp\

SRC_PYTHON = \
	src/python_module/horizonmodule.cpp \
	src/python_module/util.cpp \
	src/python_module/schematic.cpp \
	src/python_module/project.cpp \
	src/python_module/board.cpp \

SRC_ALL = $(sort $(SRC_COMMON) $(SRC_IMP) $(SRC_POOL_UTIL) $(SRC_PRJ_UTIL) $(SRC_POOL_UPDATE_PARA) $(SRC_PGM_TEST) $(SRC_POOL_PRJ_MGR) $(SRC_GEN_PKG))

INC = -Isrc -I3rd_party

DEFINES = -D_USE_MATH_DEFINES -DGLM_ENABLE_EXPERIMENTAL

LIBS_COMMON = sqlite3 yaml-cpp libzip
ifneq ($(OS),Windows_NT)
	LIBS_COMMON += uuid
endif
LIBS_ALL = $(LIBS_COMMON) gtkmm-3.0 epoxy cairomm-pdf-1.0 librsvg-2.0 libzmq libgit2 libcurl glm

OPTIMIZE=-fdata-sections -ffunction-sections -O3
#OPTIMIZE=-fdata-sections -ffunction-sections
DEBUG   =-g3
CXXFLAGS  =$(DEBUG) $(DEFINES) $(OPTIMIZE) $(shell $(PKGCONFIG) --cflags $(LIBS_ALL)) -MP -MMD -pthread -Wall -Wshadow -std=c++14 
CFLAGS = $(filter-out -std=%,$(CXXFLAGS)) -std=c99
LDFLAGS = -lm -lpthread
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

ifeq ($(OS),Windows_NT)
    LDFLAGS += -lrpcrt4
    DEFINES += -DWIN32_UUID
    LDFLAGS_GUI = -Wl,-subsystem,windows
else
    UNAME := $(shell uname)
    ifeq ($(UNAME), Darwin)
    	# O3 generate incompatibel frames?
    	OPTIMIZE=-fdata-sections -ffunction-sections -O2
    	# support brew on non-standart location
    	BREWROOT ?= $(shell brew --prefix)
    	INC += -I$(BREWROOT)/include
    	INC_OCE = -I$(BREWROOT)/include/opencascade
    	LDFLAGS_OCE = -L$(BREWROOT)/lib/ -lTKSTEP  -lTKernel  -lTKXCAF -lTKXSBase -lTKBRep -lTKCDF -lTKXDESTEP -lTKLCAF -lTKMath -lTKMesh -lTKTopAlgo -lTKPrim -lTKBO -lTKG3d
    	CFLAGS += -mmacosx-version-min=10.12 
    	CXXFLAGS += -mmacosx-version-min=10.12 
    	LDFLAGS += -mmacosx-version-min=10.12 $(DEBUG)
    	# osrf/homebrew-simulation/cppzmq, podofo
    else
        LDFLAGS += -fuse-ld=gold
    endif
endif

SRC_SHARED = $(SRC_COMMON) \
	src/pool/pool_cached.cpp \
	src/export_pdf/canvas_pdf.cpp \
	src/export_pdf/export_pdf.cpp \
	src/export_pdf/export_pdf_board.cpp \
	src/canvas/canvas.cpp \
	src/canvas/appearance.cpp \
	src/canvas/render.cpp \
	src/canvas/draw.cpp \
	src/canvas/text.cpp \
	src/canvas/hershey_fonts.cpp \
	src/canvas/image.cpp\
	src/canvas/selectables.cpp\
	src/canvas/fragment_cache.cpp\
	src/util/text_data.cpp \
	3rd_party/polypartition/polypartition.cpp\
	3rd_party/poly2tri/common/shapes.cpp\
	3rd_party/poly2tri/sweep/cdt.cpp\
	3rd_party/poly2tri/sweep/sweep.cpp\
	3rd_party/poly2tri/sweep/sweep_context.cpp\
	3rd_party/poly2tri/sweep/advancing_front.cpp\
	src/export_bom/export_bom.cpp\
	src/export_gerber/gerber_writer.cpp\
	src/export_gerber/excellon_writer.cpp\
	src/export_gerber/gerber_export.cpp\
	src/export_gerber/canvas_gerber.cpp\
	src/export_gerber/hash.cpp\
	src/board/plane_update.cpp\
	src/canvas/canvas_pads.cpp\
	src/canvas/canvas_patch.cpp\

SRC_SHARED_GEN = $(SRC_COMMON_GEN)

OBJDIR           = $(BUILDDIR)/obj
PICOBJDIR        = $(BUILDDIR)/picobj
GENDIR           = $(BUILDDIR)/gen
MKDIR            = mkdir -p
ifneq ($(VERBOSE),1)
QUIET            = @
endif
ECHO             = @echo

# Object files
OBJ_ALL          = $(addprefix $(OBJDIR)/,$(SRC_ALL:.cpp=.o))
OBJ_ALL         += $(addprefix $(OBJDIR)/,$(SRC_IMPC:.c=.o))
OBJ_ROUTER       = $(addprefix $(OBJDIR)/,$(SRC_ROUTER:.cpp=.o))
OBJ_COMMON       = $(addprefix $(OBJDIR)/,$(SRC_COMMON:.cpp=.o))
OBJ_COMMON      += $(addprefix $(OBJDIR)/,$(SRC_COMMON_GEN:.cpp=.o))
OBJ_OCE          = $(addprefix $(OBJDIR)/,$(SRC_OCE:.cpp=.o))
OBJ_PYTHON       = $(addprefix $(PICOBJDIR)/,$(SRC_PYTHON:.cpp=.o))
OBJ_SHARED       = $(addprefix $(PICOBJDIR)/,$(SRC_SHARED:.cpp=.o))
OBJ_SHARED      += $(addprefix $(PICOBJDIR)/,$(SRC_SHARED_GEN:.cpp=.o))

OBJ_IMP          = $(addprefix $(OBJDIR)/,$(SRC_IMP:.cpp=.o))
OBJ_IMP         += $(addprefix $(OBJDIR)/,$(SRC_IMPC:.c=.o))
OBJ_POOL_UTIL    = $(addprefix $(OBJDIR)/,$(SRC_POOL_UTIL:.cpp=.o))
OBJ_PRJ_UTIL     = $(addprefix $(OBJDIR)/,$(SRC_PRJ_UTIL:.cpp=.o))
OBJ_POOL_PRJ_MGR = $(addprefix $(OBJDIR)/,$(SRC_POOL_PRJ_MGR:.cpp=.o)) $(OBJ_RES)
OBJ_PGM_TEST     = $(addprefix $(OBJDIR)/,$(SRC_PGM_TEST:.cpp=.o))
OBJ_GEN_PKG      = $(addprefix $(OBJDIR)/,$(SRC_GEN_PKG:.cpp=.o))



INC_ROUTER = -I3rd_party/router/include/ -I3rd_party/router -I3rd_party
INC_OCE ?= -I/opt/opencascade/inc/ -I/mingw64/include/oce/ -I/usr/include/oce -I/usr/include/opencascade -I${CASROOT}/include/opencascade -I/usr/local/include/OpenCASCADE
INC_PYTHON = $(shell $(PKGCONFIG) --cflags python3)
LDFLAGS_OCE ?= -L /opt/opencascade/lib/ -L${CASROOT}/lib -lTKSTEP  -lTKernel  -lTKXCAF -lTKXSBase -lTKBRep -lTKCDF -lTKXDESTEP -lTKLCAF -lTKMath -lTKMesh -lTKTopAlgo -lTKPrim -lTKBO -lTKG3d
ifeq ($(OS),Windows_NT)
	LDFLAGS_OCE += -lTKV3d
endif

OBJ_RES =
ifeq ($(OS),Windows_NT)
	SRC_RES = src/horizon-pool-prj-mgr.rc
	OBJ_RES = $(addprefix $(OBJDIR)/,$(SRC_RES:.rc=.res))
endif

src/preferences/color_presets.json: $(wildcard src/preferences/color_presets/*)
	python3 scripts/make_color_presets.py $^ > $@

$(BUILDDIR)/gen/resources.cpp: imp.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies imp.gresource.xml |  while read line; do echo "src/$$line"; done)
	$(QUIET)$(MKDIR) $(dir $@)
	$(ECHO) " $@"
	$(QUIET)$(GLIB_COMPILE_RESOURCES) imp.gresource.xml --target=$@ --sourcedir=src --generate-source

$(BUILDDIR)/gen/version_gen.cpp: $(wildcard .git/HEAD .git/index) version.py make_version.py
	$(QUIET)$(MKDIR) $(dir $@)
	$(ECHO) " $@"
	$(QUIET)python3 make_version.py $@

$(BUILDDIR)/horizon-imp: $(OBJ_COMMON) $(OBJ_ROUTER) $(OBJ_OCE) $(OBJ_IMP)
	$(ECHO) " $@"
	$(QUIET)$(CXX) $^ $(LDFLAGS) $(LDFLAGS_GUI) $(LDFLAGS_OCE) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0 epoxy cairomm-pdf-1.0 librsvg-2.0 libzmq) -lpodofo -o $@

$(BUILDDIR)/horizon-pool: $(OBJ_COMMON) $(OBJ_POOL_UTIL)
	$(ECHO) " $@"
	$(QUIET)$(CXX) $^ $(LDFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0) -o $@

$(BUILDDIR)/horizon-prj: $(OBJ_COMMON) $(OBJ_PRJ_UTIL)
	$(ECHO) " $@"
	$(QUIET)$(CXX) $^ $(LDFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) glibmm-2.4 giomm-2.4) -o $@

$(BUILDDIR)/horizon-eda: $(OBJ_COMMON) $(OBJ_POOL_PRJ_MGR) $(OBJ_RES)
	$(ECHO) " $@"
	$(QUIET)$(CXX) $^ $(LDFLAGS) $(LDFLAGS_GUI) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) gtkmm-3.0 epoxy libzmq libcurl libgit2) -o $@

$(BUILDDIR)/horizon-pgm-test: $(OBJ_COMMON) $(OBJ_PGM_TEST)
	$(ECHO) " $@"
	$(QUIET)$(CXX) $^ $(LDFLAGS) $(LDFLAGS_GUI) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) glibmm-2.4 giomm-2.4) -o $@

$(BUILDDIR)/horizon-gen-pkg: $(OBJ_COMMON) $(OBJ_GEN_PKG)
	$(ECHO) " $@"
	$(QUIET)$(CXX) $^ $(LDFLAGS) $(INC) $(CXXFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) glibmm-2.4 giomm-2.4) -o $@

$(BUILDDIR)/horizon.so: $(OBJ_PYTHON) $(OBJ_SHARED)
	$(ECHO) " $@"
	$(QUIET)$(CXX) $^ $(LDFLAGS) $(INC) $(CXXFLAGS) $(shell $(PKGCONFIG) --libs $(LIBS_COMMON) python3 glibmm-2.4 giomm-2.4) -lpodofo -shared -o $@

$(OBJDIR)/%.o: %.c
	$(QUIET)$(MKDIR) $(dir $@)
	$(ECHO) " $@"
	$(QUIET)$(CC) -c $(INC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: %.cpp
	$(QUIET)$(MKDIR) $(dir $@)
	$(ECHO) " $@"
	$(QUIET)$(CXX) -c $(INC) $(CXXFLAGS) $< -o $@

$(OBJ_ROUTER): INC += $(INC_ROUTER)

$(OBJ_OCE): INC += $(INC_OCE)

$(PICOBJDIR)/%.o: %.cpp
	$(QUIET)$(MKDIR) $(dir $@)
	$(ECHO) " $@"
	$(QUIET)$(CXX) -c $(INC) $(CXXFLAGS) -fPIC $< -o $@

$(OBJ_PYTHON): INC += $(INC_PYTHON)

$(OBJ_RES): $(OBJDIR)/%.res: %.rc
	$(QUIET)$(MKDIR) $(dir $@)
	windres $< -O coff -o $@

clean: clean_router clean_oce clean_res
	$(RM) $(OBJ_ALL) $(BUILDDIR)/horizon-imp $(BUILDDIR)/horizon-pool $(BUILDDIR)/horizon-prj $(BUILDDIR)/horizon-pool-mgr $(BUILDDIR)/horizon-prj-mgr $(BUILDDIR)/horizon-pgm-test $(BUILDDIR)/horizon-gen-pkg $(BUILDDIR)/horizon-eda $(OBJ_ALL:.o=.d) $(GENDIR)/resources.cpp $(GENDIR)/version_gen.cpp
	$(RM) $(BUILDDIR)/horizon.so
	$(RM) $(OBJ_SHARED) $(OBJ_PYTHON) $(OBJ_SHARED:.o=.d) $(OBJ_PYTHON:.o=.d)

clean_router:
	$(RM) $(OBJ_ROUTER) $(OBJ_ROUTER:.o=.d)

clean_oce:
	$(RM) $(OBJ_OCE) $(OBJ_OCE:.o=.d)

clean_res:
	$(RM) $(OBJ_RES)

-include  $(OBJ_ALL:.o=.d)
-include  $(OBJ_ROUTER:.o=.d)
-include  $(OBJ_OCE:.o=.d)

.PHONY: clean clean_router clean_oce clean_res
