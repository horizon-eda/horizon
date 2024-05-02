project('horizon-eda', ['cpp', 'c'],
  version: '2.5.0',
  meson_version: '>=0.53',
  default_options: ['cpp_std=c++17', 'warning_level=1'],
)

cxx = meson.get_compiler('cpp')
is_windows = target_machine.system() == 'windows'

libm = cxx.find_library('m')
pthread = cxx.find_library('pthread')
gtk3 = dependency('gtk+-3.0', version: '>=3.22')
gtkmm = dependency('gtkmm-3.0', version: '>=3.0.0')
glibmm = dependency('glibmm-2.4')
giomm = dependency('giomm-2.4')
png = dependency('libpng')
epoxy = dependency('epoxy')
sqlite3 = dependency('sqlite3')
libgit2 = dependency('libgit2')
zeromq = dependency('libzmq')
cppzmq = dependency('cppzmq', required:false)
curl = dependency('libcurl')
rsvg = dependency('librsvg-2.0')
libarchive = dependency('libarchive')
#podofo=dependency('libpodofo', version: '<0.10.0', required: false)
opencascade = dependency('OpenCASCADE', method : 'cmake', required:false)
if not opencascade.found()
    opencascade = dependency('OCE', method : 'cmake')
endif

if not cxx.links('int main(void) {return 0;}', dependencies: opencascade)
    message('opencascade link args are broken, replacing')
    # on ubuntu 20.04, the link args are broken and are missing the -l for some reason??
    opencascade = declare_dependency(dependencies: opencascade.partial_dependency(compile_args: true, includes:true), link_args:['-lTKSTEP', '-lTKernel', '-lTKXCAF', '-lTKXSBase', '-lTKBRep', '-lTKCDF', '-lTKXDESTEP', '-lTKLCAF', '-lTKMath', '-lTKMesh', '-lTKTopAlgo', '-lTKPrim', '-lTKBO', '-lTKShHealing', '-lTKBRep', '-lTKG3d', '-lTKGeomBase', '-lTKHLR'])
endif


spnav = dependency('spnav', required: false)
if not spnav.found()
    spnav = cxx.find_library('spnav', required:false)
endif

if not cppzmq.found()
    if not cxx.has_header('zmq.hpp')
        error('cppzmq not found')
    endif
endif

libpython = dependency('python3', required: false, disabler:true)
py3cairo = dependency('py3cairo', required: false, disabler:true)
osmesa = dependency('osmesa', required: false, disabler:true)
glm = dependency('glm')

cpp_args_router = ['-D_USE_MATH_DEFINES']
if is_windows
    cpp_args_router += '-DWIN32_UUID'
endif
cpp_args = cpp_args_router + ['-DGLM_ENABLE_EXPERIMENTAL', '-DJSON_USE_IMPLICIT_CONVERSIONS=0']
if spnav.found()
    cpp_args += '-DHAVE_SPNAV'
endif

if get_option('debug')
    cpp_args += '-DUUID_PTR_CHECK'
    cpp_args += '-DCONNECTION_CHECK'
endif

# pkg-config is useless for old podofo :(
podofo_legacy = true
podofo_lib =cxx.find_library('podofo', dirs: '/usr/lib/podofo-0.9/', required: false, has_headers:['/usr/include/podofo-0.9/podofo/podofo.h'] )
if podofo_lib.found()
  podofo = declare_dependency (
    dependencies: podofo_lib,
    include_directories: include_directories('/usr/include/podofo-0.9')
  )
else
  podofo = dependency('libpodofo09', required:false)
  if podofo.found()
    cpp_args += '-DINC_PODOFO_WITHOUT_DIRECTORY'
  else
    podofo = dependency('libpodofo')
    if podofo.version() == 'unknown' or podofo.version() < '0.10.0'
      podofo_legacy = true
    else
      podofo_legacy = false
    endif
  endif
endif

if podofo_legacy
  message('using legacy podofo')
endif

stdlibs = []
is_libstdcpp = cxx.get_define('__GLIBCXX__', prefix: '#include <vector>') != ''
if is_libstdcpp
    message('using libstdc++')
else
    message('not using libstdc++')
endif
if is_libstdcpp
    stdlibs += cxx.find_library('stdc++')
    stdlibs += cxx.find_library('stdc++fs', required:false)
endif


build_dependencies_libhorizon = [libm, pthread, gtkmm, sqlite3, png] + stdlibs
build_dependencies_horizonmodule = [libm, gtkmm, sqlite3, png, py3cairo, podofo, libpython, osmesa, opencascade, libarchive]
if not is_windows
	uuid = dependency('uuid')
	build_dependencies_libhorizon += uuid
	build_dependencies_horizonmodule += uuid
else
	build_dependencies_libhorizon += [cxx.find_library('rpcrt4')]
endif

build_dependencies_libhorizonui = build_dependencies_libhorizon + [epoxy, zeromq, cppzmq, curl, glm]
build_dependencies_gen_pkg =    build_dependencies_libhorizon
build_dependencies_pr_review = build_dependencies_libhorizon + [podofo, osmesa, opencascade, libgit2]

build_dependencies_pool_prj_mgr = build_dependencies_libhorizonui + [libgit2]
build_dependencies_imp = build_dependencies_libhorizonui +[rsvg, podofo, libarchive, opencascade, spnav]


src_common = files(
    'src/block/block.cpp',
    'src/block/block_instance.cpp',
    'src/block/bom.cpp',
    'src/block/bom_export_settings.cpp',
    'src/block/bus.cpp',
    'src/block/component.cpp',
    'src/block/net.cpp',
    'src/block/net_class.cpp',
    'src/block/net_tie.cpp',
    'src/block_symbol/block_symbol.cpp',
    'src/blocks/blocks.cpp',
    'src/blocks/blocks_schematic.cpp',
    'src/blocks/dependency_graph.cpp',
    'src/board/airwire.cpp',
    'src/board/airwires.cpp',
    'src/board/board.cpp',
    'src/board/board_decal.cpp',
    'src/board/board_hole.cpp',
    'src/board/board_junction.cpp',
    'src/board/board_layers.cpp',
    'src/board/board_net_tie.cpp',
    'src/board/board_package.cpp',
    'src/board/board_panel.cpp',
    'src/board/board_rules.cpp',
    'src/board/connection_line.cpp',
    'src/board/gerber_output_settings.cpp',
    'src/board/included_board.cpp',
    'src/board/odb_output_settings.cpp',
    'src/board/plane.cpp',
    'src/board/pnp.cpp',
    'src/board/pnp_export_settings.cpp',
    'src/board/rule_board_connectivity.cpp',
    'src/board/rule_clearance_copper.cpp',
    'src/board/rule_clearance_copper_keepout.cpp',
    'src/board/rule_clearance_copper_other.cpp',
    'src/board/rule_clearance_same_net.cpp',
    'src/board/rule_clearance_silk_exp_copper.cpp',
    'src/board/rule_diffpair.cpp',
    'src/board/rule_hole_size.cpp',
    'src/board/rule_layer_pair.cpp',
    'src/board/rule_net_ties.cpp',
    'src/board/rule_parameters.cpp',
    'src/board/rule_plane.cpp',
    'src/board/rule_preflight_checks.cpp',
    'src/board/rule_shorted_pads.cpp',
    'src/board/rule_thermals.cpp',
    'src/board/rule_track_width.cpp',
    'src/board/rule_via.cpp',
    'src/board/rule_via_definitions.cpp',
    'src/board/step_export_settings.cpp',
    'src/board/track.cpp',
    'src/board/via.cpp',
    'src/board/via_definition.cpp',
    'src/canvas/hershey_fonts.cpp',
    'src/common/arc.cpp',
    'src/common/common.cpp',
    'src/common/dimension.cpp',
    'src/common/grid_settings.cpp',
    'src/common/hole.cpp',
    'src/common/junction.cpp',
    'src/common/junction_util.cpp',
    'src/common/keepout.cpp',
    'src/common/layer_provider.cpp',
    'src/common/line.cpp',
    'src/common/lut.cpp',
    'src/common/object_descr.cpp',
    'src/common/patch_type_names.cpp',
    'src/common/pdf_export_settings.cpp',
    'src/common/picture.cpp',
    'src/common/pin_name_orientation.cpp',
    'src/common/polygon.cpp',
    'src/common/shape.cpp',
    'src/common/text.cpp',
    'src/frame/frame.cpp',
    'src/logger/logger.cpp',
    'src/package/package_rules.cpp',
    'src/package/package_rules_check.cpp',
    'src/package/pad.cpp',
    'src/package/rule_clearance_package.cpp',
    'src/package/rule_package_checks.cpp',
    'src/parameter/program.cpp',
    'src/parameter/program_polygon.cpp',
    'src/parameter/set.cpp',
    'src/pool/decal.cpp',
    'src/pool/entity.cpp',
    'src/pool/gate.cpp',
    'src/pool/ipool.cpp',
    'src/pool/package.cpp',
    'src/pool/padstack.cpp',
    'src/pool/part.cpp',
    'src/pool/pool.cpp',
    'src/pool/pool_info.cpp',
    'src/pool/pool_manager.cpp',
    'src/pool/pool_parametric.cpp',
    'src/pool/project_pool.cpp',
    'src/pool/symbol.cpp',
    'src/pool/unit.cpp',
    'src/project/project.cpp',
    'src/rules/rule.cpp',
    'src/rules/rule_descr.cpp',
    'src/rules/rule_match.cpp',
    'src/rules/rule_match_component.cpp',
    'src/rules/rule_match_component.cpp',
    'src/rules/rule_match_keepout.cpp',
    'src/rules/rules.cpp',
    'src/rules/rules_import_export.cpp',
    'src/schematic/bus_label.cpp',
    'src/schematic/bus_ripper.cpp',
    'src/schematic/line_net.cpp',
    'src/schematic/net_label.cpp',
    'src/schematic/power_symbol.cpp',
    'src/schematic/rule_connectivity.cpp',
    'src/schematic/schematic.cpp',
    'src/schematic/schematic_block_symbol.cpp',
    'src/schematic/schematic_junction.cpp',
    'src/schematic/schematic_net_tie.cpp',
    'src/schematic/schematic_rules.cpp',
    'src/schematic/schematic_symbol.cpp',
    'src/schematic/sheet.cpp',
    'src/symbol/rule_symbol_checks.cpp',
    'src/symbol/symbol_rules.cpp',
    'src/symbol/symbol_rules_check.cpp',
    'src/util/csv.cpp',
    'src/util/dependency_graph.cpp',
    'src/util/file_version.cpp',
    'src/util/fs_util.cpp',
    'src/util/geom_util.cpp',
    'src/util/installation_uuid.cpp',
    'src/util/layer_range.cpp',
    'src/util/picture_data.cpp',
    'src/util/picture_load.cpp',
    'src/util/pin_direction_accumulator.cpp',
    'src/util/placement.cpp',
    'src/util/polygon_arc_removal_proxy.cpp',
    'src/util/sqlite.cpp',
    'src/util/str_util.cpp',
    'src/util/text_data.cpp',
    'src/util/util.cpp',
    'src/util/uuid.cpp',
    'src/util/uuid_path.cpp',
    'src/util/uuid_vec.cpp',
    'src/util/version.cpp',
    'src/util/clipper_util.cpp',
)

if is_windows
	src_common += 'src/util/uuid_win32.cpp'
endif

src_common_ui = files(
    'src/widgets/pool_browser.cpp',
    'src/widgets/pool_browser_button.cpp',
    'src/widgets/pool_browser_decal.cpp',
    'src/widgets/pool_browser_entity.cpp',
    'src/widgets/pool_browser_frame.cpp',
    'src/widgets/pool_browser_package.cpp',
    'src/widgets/pool_browser_padstack.cpp',
    'src/widgets/pool_browser_parametric.cpp',
    'src/widgets/pool_browser_part.cpp',
    'src/widgets/pool_browser_stockinfo.cpp',
    'src/widgets/pool_browser_symbol.cpp',
    'src/widgets/pool_browser_unit.cpp',
    'src/preferences/preferences.cpp',
    'src/preferences/preferences_provider.cpp',
    'src/preferences/preferences_util.cpp',
    'src/util/automatic_prefs.cpp',
    'src/util/sort_controller.cpp',
    'src/util/status_dispatcher.cpp',
    'src/util/stock_info_provider.cpp',
    'src/util/stock_info_provider_digikey.cpp',
    'src/util/stock_info_provider_partinfo.cpp',
    'src/util/treeview_state_store.cpp',
    'src/util/window_state_store.cpp',
    'src/util/zmq_helper.cpp',
    'src/widgets/about_dialog.cpp',
    'src/widgets/help_button.cpp',
    'src/widgets/location_entry.cpp',
    'src/widgets/log_view.cpp',
    'src/widgets/log_window.cpp',
    'src/widgets/tag_entry.cpp',
    'src/widgets/pool_selector.cpp',
    'src/widgets/preview_canvas.cpp',
    'src/widgets/project_meta_editor.cpp',
    'src/imp/action.cpp',
    'src/imp/action_catalog.cpp',
    'src/imp/in_tool_action_catalog.cpp',
    'src/dialogs/pool_browser_dialog.cpp',
    'src/util/exception_util.cpp',
    'src/util/gtk_util.cpp',
    'src/util/history_manager.cpp',
    'src/util/http_client.cpp',
    'src/util/pool_completion.cpp',
    'src/widgets/cell_renderer_color_box.cpp',
    'src/widgets/color_box.cpp',
    'src/util/kicad_package_parser.cpp',
    'src/logger/log_dispatcher.cpp',
)

src_canvas = files(
    'src/canvas/canvas.cpp',
    'src/canvas/canvas_layer_provider.cpp',
    'src/canvas/appearance.cpp',
    'src/canvas/render.cpp',
    'src/canvas/draw.cpp',
    'src/canvas/text.cpp',
    'src/canvas/image.cpp',
    'src/canvas/selectables.cpp',
    'src/canvas/fragment_cache.cpp',
    'src/util/text_renderer.cpp',
    'src/canvas/text_renderer.cpp',
    'src/canvas/layer_display.cpp',
)

src_pool_prj_mgr = files(
    'src/canvas/canvas_patch.cpp',
    'src/checks/check_entity.cpp',
    'src/checks/check_item.cpp',
    'src/checks/check_part.cpp',
    'src/checks/check_unit.cpp',
    'src/checks/check_util.cpp',
    'src/pool-prj-mgr/autosave_recovery_dialog.cpp',
    'src/pool-prj-mgr/close_utils.cpp',
    'src/pool-prj-mgr/output_window.cpp',
    'src/pool-prj-mgr/pool-mgr/check_column.cpp',
    'src/pool-prj-mgr/pool-mgr/create_part_dialog.cpp',
    'src/pool-prj-mgr/pool-mgr/duplicate/duplicate_entity.cpp',
    'src/pool-prj-mgr/pool-mgr/duplicate/duplicate_part.cpp',
    'src/pool-prj-mgr/pool-mgr/duplicate/duplicate_unit.cpp',
    'src/pool-prj-mgr/pool-mgr/duplicate/duplicate_window.cpp',
    'src/pool-prj-mgr/pool-mgr/editors/editor_base.cpp',
    'src/pool-prj-mgr/pool-mgr/editors/editor_window.cpp',
    'src/pool-prj-mgr/pool-mgr/editors/entity_editor.cpp',
    'src/pool-prj-mgr/pool-mgr/editors/parametric.cpp',
    'src/pool-prj-mgr/pool-mgr/editors/part_editor.cpp',
    'src/pool-prj-mgr/pool-mgr/editors/unit_editor.cpp',
    'src/pool-prj-mgr/pool-mgr/github_login_window.cpp',
    'src/pool-prj-mgr/pool-mgr/import_kicad_package_window.cpp',
    'src/pool-prj-mgr/pool-mgr/kicad_symbol_import_wizard/gate_editor.cpp',
    'src/pool-prj-mgr/pool-mgr/kicad_symbol_import_wizard/kicad_symbol_import_wizard.cpp',
    'src/pool-prj-mgr/pool-mgr/move_window.cpp',
    'src/pool-prj-mgr/pool-mgr/part_wizard/gate_editor.cpp',
    'src/pool-prj-mgr/pool-mgr/part_wizard/pad_editor.cpp',
    'src/pool-prj-mgr/pool-mgr/part_wizard/part_wizard.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_cache_box.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_git_box.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_notebook.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_notebook_decals.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_notebook_entities.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_notebook_frames.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_notebook_packages.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_notebook_padstacks.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_notebook_parametric.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_notebook_parts.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_notebook_symbols.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_notebook_units.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_remote_box.cpp',
    'src/pool-prj-mgr/pool-mgr/pool_settings_box.cpp',
    'src/pool-prj-mgr/pool-mgr/view_create_pool.cpp',
    'src/pool-prj-mgr/pool-prj-mgr-app.cpp',
    'src/pool-prj-mgr/pool-prj-mgr-app_win.cpp',
    'src/pool-prj-mgr/pool-prj-mgr-main.cpp',
    'src/pool-prj-mgr/pool-prj-mgr-process.cpp',
    'src/pool-prj-mgr/pool_update_error_dialog.cpp',
    'src/pool-prj-mgr/pools_window/pool_download_window.cpp',
    'src/pool-prj-mgr/pools_window/pool_index.cpp',
    'src/pool-prj-mgr/pools_window/pool_merge_box.cpp',
    'src/pool-prj-mgr/pools_window/pool_status_provider.cpp',
    'src/pool-prj-mgr/pools_window/pools_window.cpp',
    'src/pool-prj-mgr/pools_window/pools_window_available.cpp',
    'src/pool-prj-mgr/preferences/action_editor.cpp',
    'src/pool-prj-mgr/preferences/digikey_auth_window.cpp',
    'src/pool-prj-mgr/preferences/preferences_row.cpp',
    'src/pool-prj-mgr/preferences/preferences_window.cpp',
    'src/pool-prj-mgr/preferences/preferences_window_canvas.cpp',
    'src/pool-prj-mgr/preferences/preferences_window_in_tool_keys.cpp',
    'src/pool-prj-mgr/preferences/preferences_window_input_devices.cpp',
    'src/pool-prj-mgr/preferences/preferences_window_keys.cpp',
    'src/pool-prj-mgr/preferences/preferences_window_misc.cpp',
    'src/pool-prj-mgr/preferences/preferences_window_spacenav.cpp',
    'src/pool-prj-mgr/preferences/preferences_window_stock_info.cpp',
    'src/pool-prj-mgr/preferences/preferences_window_stock_info_digikey.cpp',
    'src/pool-prj-mgr/preferences/preferences_window_stock_info_partinfo.cpp',
    'src/pool-prj-mgr/prj-mgr/part_browser/part_browser_window.cpp',
    'src/pool-prj-mgr/prj-mgr/pool_cache_cleanup_dialog.cpp',
    'src/pool-prj-mgr/prj-mgr/prj-mgr_views.cpp',
    'src/pool-prj-mgr/welcome_window.cpp',
    'src/pool/pool_cache_status.cpp',
    'src/util/editor_process.cpp',
    'src/util/github_client.cpp',
    'src/util/kicad_lib_parser.cpp',
    'src/util/paned_state_store.cpp',
    'src/util/pool_check_schema_update.cpp',
    'src/util/sort_helper.cpp',
    'src/widgets/capture_dialog.cpp',
    'src/widgets/entity_info_box.cpp',
    'src/widgets/entity_preview.cpp',
    'src/widgets/forced_pool_update_dialog.cpp',
    'src/widgets/package_info_box.cpp',
    'src/widgets/padstack_preview.cpp',
    'src/widgets/part_preview.cpp',
    'src/widgets/pin_names_editor.cpp',
    'src/widgets/preview_base.cpp',
    'src/widgets/recent_item_box.cpp',
    'src/widgets/sqlite_shell.cpp',
    'src/widgets/symbol_preview.cpp',
    'src/widgets/unit_info_box.cpp',
    'src/widgets/unit_preview.cpp',
    'src/widgets/where_used_box.cpp',
)

src_pool_update = files(
    'src/pool-update/pool-update.cpp',
    'src/pool-update/pool-update_parametric.cpp',
    'src/pool-update/pool-update_pool.cpp',
    'src/pool-update/pool_updater.cpp',
    'src/pool-update/pool_updater_frame.cpp',
    'src/pool-update/pool_updater_decal.cpp',
    'src/pool-update/pool_updater_unit.cpp',
    'src/pool-update/pool_updater_entity.cpp',
    'src/pool-update/pool_updater_padstack.cpp',
    'src/pool-update/pool_updater_package.cpp',
    'src/pool-update/pool_updater_part.cpp',
    'src/pool-update/pool_updater_symbol.cpp',
    'src/pool-update/graph.cpp',
)

src_canvas_gl = files(
    'src/canvas/canvas_gl.cpp',
    'src/canvas/grid.cpp',
    'src/canvas/gl_util.cpp',
    'src/canvas/pan.cpp',
    'src/canvas/drag_selection.cpp',
    'src/canvas/selectables_renderer.cpp',
    'src/canvas/hover_prelight.cpp',
    'src/canvas/triangle_renderer.cpp',
    'src/canvas/selection_filter.cpp',
    'src/canvas/marker.cpp',
    'src/canvas/annotation.cpp',
    'src/util/msd.cpp',
    'src/util/msd_animator.cpp',
    'src/canvas/bitmap_font_util.cpp',
    'src/canvas/picture_renderer.cpp',
    'src/util/warp_cursor.cpp',
)

subdir('src/core/tools')

src_imp = files(
	'src/export_step/export_step.cpp',
	'src/imp/3d/imp_package_3d_occt.cpp',
    '3rd_party/sexpr/sexpr.cpp',
    '3rd_party/sexpr/sexpr_parser.cpp',
    'src/board/board_rules_import.cpp',
    'src/board/plane_update.cpp',
    'src/canvas/canvas_pads.cpp',
    'src/canvas/canvas_patch.cpp',
    'src/canvas3d/canvas3d.cpp',
    'src/core/clipboard/clipboard.cpp',
    'src/core/clipboard/clipboard_board.cpp',
    'src/core/clipboard/clipboard_package.cpp',
    'src/core/clipboard/clipboard_padstack.cpp',
    'src/core/clipboard/clipboard_schematic.cpp',
    'src/core/core.cpp',
    'src/core/core_board.cpp',
    'src/core/core_decal.cpp',
    'src/core/core_frame.cpp',
    'src/core/core_package.cpp',
    'src/core/core_padstack.cpp',
    'src/core/core_properties.cpp',
    'src/core/core_schematic.cpp',
    'src/core/core_symbol.cpp',
    'src/core/create_tool.cpp',
    'src/core/tool.cpp',
    'src/dialogs/align_and_distribute_window.cpp',
    'src/dialogs/annotate.cpp',
    'src/dialogs/ask_datum.cpp',
    'src/dialogs/ask_datum_angle.cpp',
    'src/dialogs/ask_datum_string.cpp',
    'src/dialogs/ask_net_merge.cpp',
    'src/dialogs/dialogs.cpp',
    'src/dialogs/edit_board_hole.cpp',
    'src/dialogs/edit_custom_value.cpp',
    'src/dialogs/edit_frame.cpp',
    'src/dialogs/edit_keepout.cpp',
    'src/dialogs/edit_plane_window.cpp',
    'src/dialogs/edit_shape.cpp',
    'src/dialogs/edit_stackup.cpp',
    'src/dialogs/edit_text_window.cpp',
    'src/dialogs/edit_via.cpp',
    'src/dialogs/enter_datum_angle_window.cpp',
    'src/dialogs/enter_datum_scale_window.cpp',
    'src/dialogs/enter_datum_window.cpp',
    'src/dialogs/generate_silkscreen_window.cpp',
    'src/dialogs/manage_buses.cpp',
    'src/dialogs/manage_included_boards.cpp',
    'src/dialogs/manage_net_classes.cpp',
    'src/dialogs/manage_ports.cpp',
    'src/dialogs/manage_power_nets.cpp',
    'src/dialogs/map_net_tie.cpp',
    'src/dialogs/map_package.cpp',
    'src/dialogs/map_uuid_path.cpp',
    'src/dialogs/pad_parameter_set_window.cpp',
    'src/dialogs/plane_update.cpp',
    'src/dialogs/project_properties.cpp',
    'src/dialogs/renumber_pads_window.cpp',
    'src/dialogs/router_settings_window.cpp',
    'src/dialogs/schematic_properties.cpp',
    'src/dialogs/select_block.cpp',
    'src/dialogs/select_group_tag.cpp',
    'src/dialogs/select_included_board.cpp',
    'src/dialogs/select_net.cpp',
    'src/dialogs/select_via_definition.cpp',
    'src/dialogs/symbol_pin_names_window.cpp',
    'src/dialogs/tool_window.cpp',
    'src/document/document.cpp',
    'src/document/document_board.cpp',
    'src/document/documents.cpp',
    'src/imp/3d/3d_view.cpp',
    'src/imp/3d/axes_lollipop.cpp',
    'src/imp/3d/imp_package_3d.cpp',
    'src/imp/3d/model_editor.cpp',
    'src/imp/3d/place_model_box.cpp',
    'src/imp/action_icon.cpp',
    'src/imp/airwire_filter_window.cpp',
    'src/imp/bom_export_window.cpp',
    'src/imp/clipboard_handler.cpp',
    'src/imp/fab_output_window.cpp',
    'src/imp/footprint_generator/footag/display.cpp',
    'src/imp/footprint_generator/footprint_generator_base.cpp',
    'src/imp/footprint_generator/footprint_generator_dual.cpp',
    'src/imp/footprint_generator/footprint_generator_footag.cpp',
    'src/imp/footprint_generator/footprint_generator_grid.cpp',
    'src/imp/footprint_generator/footprint_generator_quad.cpp',
    'src/imp/footprint_generator/footprint_generator_single.cpp',
    'src/imp/footprint_generator/footprint_generator_window.cpp',
    'src/imp/footprint_generator/svg_overlay.cpp',
    'src/imp/grid_controller.cpp',
    'src/imp/grids_window.cpp',
    'src/imp/header_button.cpp',
    'src/imp/imp.cpp',
    'src/imp/imp_action.cpp',
    'src/imp/imp_board.cpp',
    'src/imp/imp_decal.cpp',
    'src/imp/imp_frame.cpp',
    'src/imp/imp_hud.cpp',
    'src/imp/imp_interface.cpp',
    'src/imp/imp_key.cpp',
    'src/imp/imp_layer.cpp',
    'src/imp/imp_main.cpp',
    'src/imp/imp_package.cpp',
    'src/imp/imp_padstack.cpp',
    'src/imp/imp_schematic.cpp',
    'src/imp/imp_search.cpp',
    'src/imp/imp_symbol.cpp',
    'src/imp/keyseq_dialog.cpp',
    'src/imp/main_window.cpp',
    'src/imp/parameter_window.cpp',
    'src/imp/parts_window.cpp',
    'src/imp/pdf_export_window.cpp',
    'src/imp/pnp_export_window.cpp',
    'src/imp/rules/export.cpp',
    'src/imp/rules/import.cpp',
    'src/imp/rules/rule_editor.cpp',
    'src/imp/rules/rule_editor_clearance_copper.cpp',
    'src/imp/rules/rule_editor_clearance_copper_keepout.cpp',
    'src/imp/rules/rule_editor_clearance_copper_other.cpp',
    'src/imp/rules/rule_editor_clearance_same_net.cpp',
    'src/imp/rules/rule_editor_clearance_silk_exp_copper.cpp',
    'src/imp/rules/rule_editor_connectivity.cpp',
    'src/imp/rules/rule_editor_diffpair.cpp',
    'src/imp/rules/rule_editor_hole_size.cpp',
    'src/imp/rules/rule_editor_layer_pair.cpp',
    'src/imp/rules/rule_editor_package_checks.cpp',
    'src/imp/rules/rule_editor_plane.cpp',
    'src/imp/rules/rule_editor_shorted_pads.cpp',
    'src/imp/rules/rule_editor_thermals.cpp',
    'src/imp/rules/rule_editor_track_width.cpp',
    'src/imp/rules/rule_editor_via.cpp',
    'src/imp/rules/rule_editor_via_definitions.cpp',
    'src/imp/rules/rule_match_component_editor.cpp',
    'src/imp/rules/rule_match_editor.cpp',
    'src/imp/rules/rule_match_keepout_editor.cpp',
    'src/imp/rules/rules_window.cpp',
    'src/imp/search/searcher.cpp',
    'src/imp/search/searcher_board.cpp',
    'src/imp/search/searcher_package.cpp',
    'src/imp/search/searcher_schematic.cpp',
    'src/imp/search/searcher_symbol.cpp',
    'src/imp/selection_filter_dialog.cpp',
    'src/imp/step_export_window.cpp',
    'src/imp/symbol_preview/preview_box.cpp',
    'src/imp/symbol_preview/symbol_preview_expand_window.cpp',
    'src/imp/symbol_preview/symbol_preview_window.cpp',
    'src/imp/tool_popover.cpp',
    'src/imp/tuning_window.cpp',
    'src/imp/view_angle_window.cpp',
    'src/import_dxf/dxf_importer.cpp',
    'src/import_step/step_importer.cpp',
    'src/property_panels/property_editor.cpp',
    'src/property_panels/property_panel.cpp',
    'src/property_panels/property_panels.cpp',
    'src/rules/rules_with_core.cpp',
    'src/schematic/schematic_rules_check.cpp',
    'src/util/action_label.cpp',
    'src/util/csv_util.cpp',
    'src/util/done_revealer_controller.cpp',
    'src/util/export_file_chooser.cpp',
    'src/util/keep_slope_util.cpp',
    'src/util/pasted_package.cpp',
    'src/util/picture_util.cpp',
    'src/util/selection_util.cpp',
    'src/widgets/action_button.cpp',
    'src/widgets/board_display_options.cpp',
    'src/widgets/column_chooser.cpp',
    'src/widgets/component_button.cpp',
    'src/widgets/component_selector.cpp',
    'src/widgets/layer_box.cpp',
    'src/widgets/layer_combo_box.cpp',
    'src/widgets/layer_help_box.cpp',
    'src/widgets/layer_range_editor.cpp',
    'src/widgets/msd_tuning_window.cpp',
    'src/widgets/multi_component_button.cpp',
    'src/widgets/multi_component_selector.cpp',
    'src/widgets/multi_item_button.cpp',
    'src/widgets/multi_item_selector.cpp',
    'src/widgets/multi_net_button.cpp',
    'src/widgets/multi_net_selector.cpp',
    'src/widgets/multi_pad_button.cpp',
    'src/widgets/multi_pad_selector.cpp',
    'src/widgets/net_button.cpp',
    'src/widgets/net_class_button.cpp',
    'src/widgets/net_selector.cpp',
    'src/widgets/parameter_set_editor.cpp',
    'src/widgets/plane_editor.cpp',
    'src/widgets/reflow_box.cpp',
    'src/widgets/sheet_box.cpp',
    'src/widgets/spin_button_angle.cpp',
    'src/widgets/spin_button_dim.cpp',
    'src/widgets/text_editor.cpp',
    'src/widgets/title_block_values_editor.cpp',
    'src/widgets/unplaced_box.cpp',
    'src/widgets/warnings_box.cpp',
)

src_export = files(
    'src/export_gerber/gerber_writer.cpp',
    'src/export_gerber/excellon_writer.cpp',
    'src/export_gerber/gerber_export.cpp',
    'src/export_gerber/canvas_gerber.cpp',
    'src/export_util/padstack_hash.cpp',
    'src/export_util/tree_writer.cpp',
    'src/export_util/tree_writer_fs.cpp',
    'src/export_util/tree_writer_archive.cpp',
    'src/export_pnp/export_pnp.cpp',
    'src/export_bom/export_bom.cpp',
    'src/export_odb/odb_export.cpp',
    'src/export_odb/canvas_odb.cpp',
    'src/export_odb/db.cpp',
    'src/export_odb/eda_data.cpp',
    'src/export_odb/attribute_util.cpp',
    'src/export_odb/surface_data.cpp',
    'src/export_odb/features.cpp',
    'src/export_odb/components.cpp',
    'src/export_odb/symbol.cpp',
    'src/export_odb/odb_util.cpp',
    'src/export_odb/track_graph.cpp',
    'src/export_odb/structured_text_writer.cpp'
)

if podofo_legacy
  src_export += 'src/export_pdf/legacy/canvas_pdf.cpp'
  src_export += 'src/export_pdf/legacy/export_pdf.cpp'
  src_export += 'src/export_pdf/legacy/export_pdf_board.cpp'
  src_export += 'src/export_pdf/legacy/export_pdf_util.cpp'
else
  src_export += 'src/export_pdf/canvas_pdf.cpp'
  src_export += 'src/export_pdf/export_pdf.cpp'
  src_export += 'src/export_pdf/export_pdf_board.cpp'
  src_export += 'src/export_pdf/export_pdf_util.cpp'
endif

src_board_rules_check = files(
    'src/rules/cache.cpp',
    'src/board/board_rules_check.cpp',
    'src/board/board_rules_check_clearance_copper.cpp',
    'src/board/board_rules_check_clearance_copper_non_copper.cpp',
    'src/board/board_rules_check_net_ties.cpp',
    'src/board/board_rules_check_plane_priorities.cpp',
    'src/board/board_rules_check_board_connectivity.cpp',
    'src/board/board_rules_check_util.cpp',
)

src_python = files(
    'src/export_step/export_step.cpp',
    'src/import_step/step_importer.cpp',
    'src/python_module/horizonmodule.cpp',
    'src/python_module/util.cpp',
    'src/python_module/schematic.cpp',
    'src/python_module/project.cpp',
    'src/python_module/board.cpp',
    'src/python_module/pool_manager.cpp',
    'src/python_module/pool.cpp',
    'src/python_module/3d_image_exporter.cpp',
    'src/python_module/version.cpp',
)

src_canvas_3d = files(
    'src/canvas3d/canvas3d_base.cpp',
    'src/canvas3d/canvas_mesh.cpp',
    'src/canvas3d/background_renderer.cpp',
    'src/canvas3d/wall_renderer.cpp',
    'src/canvas3d/cover_renderer.cpp',
    'src/canvas3d/face_renderer.cpp',
    'src/canvas3d/point_renderer.cpp',
)

src_3d_export = files(
    'src/export_3d_image/export_3d_image.cpp',
    'src/canvas/gl_util.cpp',
)

src_shared = files(
    'src/board/plane_update.cpp',
    'src/canvas/canvas_pads.cpp',
    'src/canvas/canvas_patch.cpp',
    'src/util/csv_util.cpp',
    'src/document/document.cpp',
    'src/document/document_board.cpp',
)

src_pr_review = files(
    'src/pr-review/pr-review.cpp',
    'src/pr-review/canvas_cairo2.cpp',
    'src/canvas/canvas_patch.cpp',
    'src/import_step/step_importer.cpp',
    'src/checks/check_util.cpp',
    'src/checks/check_entity.cpp',
    'src/checks/check_unit.cpp',
    'src/checks/check_part.cpp',
    'src/checks/check_item.cpp',
)

src_gen_pkg = files(
    'src/gen-pkg/gen-pkg.cpp',
)

src_tests = files(
    '3rd_party/catch2/catch_amalgamated.cpp',
    'src/tests/tool_lut.cpp',
    'src/tests/layer_display_default.cpp',
    'src/imp/action_catalog.cpp',
    'src/imp/action.cpp',
)

prog_python = find_program('python3')

help_texts = custom_target(
    'help_texts',
    output : ['help_texts.hpp', 'help_texts.cpp'],
    input : ['scripts/make_help.py', 'src/help_texts.txt'],
    command : [prog_python, '@INPUT@', '@OUTPUT@'],
)

fs = import('fs')
has_git_dir = fs.is_dir('.git')
version_deps = ['version.py']
if has_git_dir
    message('including git commit')
    version_deps += ['.git/HEAD', '.git/index']
endif

version_gen_cpp = custom_target(
    'version_gen.cpp',
    output : 'version_gen.cpp',
    input : ['make_version.py'],
    depend_files: version_deps,
    command : [prog_python, '@INPUT@', '@OUTPUT@', '@SOURCE_ROOT@', meson.project_version()],
)
src_common += version_gen_cpp

color_presets = [
    'Ancient_technology.board',
    'Ancient_technology.schematic',
    'Blueprint.board',
    'Blueprint.schematic',
    'Boring_day.board',
    'Boring_day.schematic',
    'Bright.board',
    'Bright.schematic',
    'Nostalgia.board',
    'Nostalgia.schematic',
    'Peltier_cooler.board',
    'Peltier_cooler.schematic',
    'Rust.board',
    'Rust.schematic',
    'Solarized.schematic',
    'Venetian_Funk.board',
    'Venetian_Funk.schematic',
]

color_preset_files = files()
foreach preset: color_presets
    color_preset_files += 'src/preferences/color_presets/' / preset
endforeach

color_presets_json = custom_target(
    'color_presets.json',
    output : 'color_presets.json',
    input : color_preset_files,
    command : [prog_python, files('scripts/make_color_presets.py'), '@OUTPUT@', '@INPUT@'],
)

if is_windows
	windows = import('windows')
	
	horizon_eda_rc = custom_target(
        'horizon-eda.rc',
        output : 'horizon-eda.rc',
        input : ['make_rc.py'],
        depend_files: files('version.py'),
        command : [prog_python,  '@INPUT@', '@OUTPUT@'],
    )

	rc_compiled =  windows.compile_resources(horizon_eda_rc, include_directories:include_directories('.'))
else
	rc_compiled = []
endif


subdir('3rd_party/clipper')
subdir('3rd_party/delaunator')
subdir('3rd_party/polypartition')
subdir('3rd_party/poly2tri')
subdir('3rd_party/sexpr')
subdir('3rd_party/dxflib')
subdir('3rd_party/footag')


include_directories = [
  include_directories('src', '3rd_party', '3rd_party/range-v3'),
]



router = static_library('router',
    files(
        '3rd_party/router/router/pns_router.cpp',
        '3rd_party/router/router/pns_item.cpp',
        '3rd_party/router/router/pns_node.cpp',
        '3rd_party/router/router/pns_solid.cpp',
        '3rd_party/router/router/pns_optimizer.cpp',
        '3rd_party/router/router/pns_topology.cpp',
        '3rd_party/router/router/pns_walkaround.cpp',
        '3rd_party/router/router/pns_utils.cpp',
        '3rd_party/router/router/pns_algo_base.cpp',
        '3rd_party/router/router/pns_diff_pair_placer.cpp',
        '3rd_party/router/router/pns_diff_pair.cpp',
        '3rd_party/router/router/pns_dp_meander_placer.cpp',
        '3rd_party/router/router/pns_dragger.cpp',
        '3rd_party/router/router/pns_itemset.cpp',
        '3rd_party/router/router/pns_line_placer.cpp',
        '3rd_party/router/router/pns_line.cpp',
        '3rd_party/router/router/pns_via.cpp',
        '3rd_party/router/router/pns_logger.cpp',
        '3rd_party/router/router/pns_meander_placer_base.cpp',
        '3rd_party/router/router/pns_meander_placer.cpp',
        '3rd_party/router/router/pns_meander_skew_placer.cpp',
        '3rd_party/router/router/pns_meander.cpp',
        '3rd_party/router/router/pns_shove.cpp',
        '3rd_party/router/router/pns_index.cpp',
        '3rd_party/router/router/time_limit.cpp',
        '3rd_party/router/router/pns_routing_settings.cpp',
        '3rd_party/router/router/pns_sizes_settings.cpp',
        '3rd_party/router/router/pns_arc.cpp',
        '3rd_party/router/router/pns_component_dragger.cpp',
        '3rd_party/router/router/pns_mouse_trail_tracer.cpp',
        '3rd_party/router/wx_compat.cpp',
        'src/router/pns_horizon_iface.cpp',
        'src/core/tools/tool_route_track_interactive.cpp',
        '3rd_party/router/kimath/src/geometry/circle.cpp',
        '3rd_party/router/kimath/src/geometry/direction_45.cpp',
        '3rd_party/router/kimath/src/geometry/geometry_utils.cpp',
        '3rd_party/router/kimath/src/geometry/seg.cpp',
        '3rd_party/router/kimath/src/geometry/shape_arc.cpp',
        '3rd_party/router/kimath/src/geometry/shape_collisions.cpp',
        '3rd_party/router/kimath/src/geometry/shape_compound.cpp',
        '3rd_party/router/kimath/src/geometry/shape.cpp',
        '3rd_party/router/kimath/src/geometry/shape_file_io.cpp',
        '3rd_party/router/kimath/src/geometry/shape_line_chain.cpp',
        '3rd_party/router/kimath/src/geometry/shape_poly_set.cpp',
        '3rd_party/router/kimath/src/geometry/shape_rect.cpp',
        '3rd_party/router/kimath/src/geometry/shape_segment.cpp',
        '3rd_party/router/kimath/src/math/util.cpp',
        '3rd_party/router/kimath/src/md5_hash.cpp',
        '3rd_party/router/color4d.cpp',
        '3rd_party/router/kimath/src/trigo.cpp',
        '3rd_party/router/base_units.cpp',
        '3rd_party/router/clipper_kicad/clipper.cpp',
        '3rd_party/router/kimath/src/math/vector2.cpp',
    ),
    include_directories: [include_directories('3rd_party/router/include', '3rd_party/router/kimath/include', '3rd_party/clipper', '3rd_party', '3rd_party/rtree', '3rd_party/router', 'src')],
    dependencies: [gtkmm],
	cpp_args: cpp_args_router,
	pic: false
)

gnome = import('gnome')
resources = gnome.compile_resources(
  'resources',
  'imp.gresource.xml',
  dependencies: color_presets_json,
  c_name: 'horizon_resources',
  source_dir: ['src', '3rd_party']
)

libhorizon = static_library('libhorizon',
    [src_common],
    dependencies: build_dependencies_libhorizon,
    include_directories: include_directories,
    cpp_args: cpp_args,
    pic:false
)

libhorizonui = static_library('libhorizonui',
    [src_common_ui, src_canvas, src_canvas_gl, help_texts],
    dependencies: build_dependencies_libhorizonui,
    include_directories: include_directories,
    cpp_args: cpp_args,
    pic:false
)


horizon_eda = executable('horizon-eda',
    [src_pool_prj_mgr, src_pool_update,resources, rc_compiled],
    dependencies: build_dependencies_pool_prj_mgr,
    link_with: [clipper_nopic, delaunator_nopic, polypartition_nopic, poly2tri_nopic, sexpr, libhorizon, libhorizonui],
    include_directories: include_directories,
    cpp_args: cpp_args,
    gui_app: true, 
    install: true
)

horizon_imp = executable('horizon-imp',
    [src_imp, src_tools, src_canvas_3d, src_export, src_board_rules_check,resources],
    dependencies: build_dependencies_imp,
    link_with: [clipper_nopic, delaunator_nopic, polypartition_nopic, poly2tri_nopic, sexpr, dxflib, footag, router, libhorizon, libhorizonui],
    include_directories: include_directories,
    cpp_args: cpp_args,
    gui_app: true, 
    install: true
)

install_man('man/horizon-eda.1', 'man/horizon-imp.1')

datadir = get_option('datadir')
install_data('org.horizon_eda.HorizonEDA.desktop', install_dir:datadir / 'applications')
install_data('org.horizon_eda.HorizonEDA.metainfo.xml', install_dir:datadir / 'metainfo')
icondir = datadir / 'icons/hicolor'

icons = [
    '16x16',
    '32x32',
    '64x64',
    '256x256'
]

foreach icon: icons
    install_data('src/icons/' / icon / 'apps/horizon-eda.png', install_dir: icondir / icon / 'apps', rename: 'org.horizon_eda.HorizonEDA.png')
endforeach
install_data('src/icons/scalable/apps/horizon-eda.svg', install_dir: icondir /  'scalable/apps', rename: 'org.horizon_eda.HorizonEDA.svg')


horizon_py = shared_module('horizon',
    [src_python, src_3d_export, src_canvas_3d, src_shared, src_pool_update, src_common,src_canvas,version_gen_cpp, src_export, src_board_rules_check, resources],
    dependencies: build_dependencies_horizonmodule,
    link_with: [clipper_pic, delaunator_pic, polypartition_pic, poly2tri_pic],
    include_directories: include_directories,
    cpp_args: cpp_args + ['-DOFFSCREEN=1'],
    build_by_default: false,
    name_prefix: ''
)

horizon_pr_review = executable('horizon-pr-review',
    [src_pr_review, src_3d_export, src_canvas_3d, src_pool_update, src_canvas, resources],
    dependencies: build_dependencies_pr_review,
    link_with: [clipper_nopic, delaunator_nopic, polypartition_nopic, poly2tri_nopic, libhorizon],
    include_directories: include_directories,
    cpp_args: cpp_args + ['-DOFFSCREEN=1'],
    build_by_default: false,
)

horizon_gen_pkg = executable('horizon-gen-pkg',
    [src_gen_pkg],
    dependencies: build_dependencies_gen_pkg,
    link_with: [clipper_nopic, libhorizon],
    include_directories: include_directories,
    build_by_default: false,
)

horizon_tests = executable('horizon-tests',
    [src_tests, resources],
    dependencies: build_dependencies_libhorizon,
    link_with: [clipper_nopic, libhorizon],
    include_directories: include_directories,
    build_by_default: false,
)

test('tests', horizon_tests)
