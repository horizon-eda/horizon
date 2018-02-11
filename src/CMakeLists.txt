
set(RESOURCE_LIST
	window.ui
	lib/util/window_state_schema.sql
	imp/layer_display_default.json
	imp/app_menu.ui
	imp/preferences.ui
	lib/property_panels/property_panel.ui
	lib/pool-update/schema.sql
	pool-update-parametric/schema.sql
	prj-mgr/window.ui
	prj-mgr/app_menu.ui
	prj-mgr/prefs.ui
	prj-mgr/pool_cache_window.ui
	pool-mgr/window.ui
	pool-mgr/app_menu.ui
	pool-mgr/unit_editor.ui
	pool-mgr/entity_editor.ui
	pool-mgr/part_editor.ui
	pool-mgr/part_wizard/part_wizard.ui
	pool-mgr/pool_merge.ui
	icons/scalable/actions/edit-schematic-symbolic.svg
	icons/scalable/actions/edit-board-symbolic.svg
	icons/scalable/actions/action-link-symbolic.svg
	icons/scalable/actions/action-unlink-symbolic.svg
	icons/scalable/apps/horizon-eda.svg
    icons/16x16/apps/horizon-eda.png
    icons/32x32/apps/horizon-eda.png
    icons/64x64/apps/horizon-eda.png
    icons/256x256/apps/horizon-eda.png
	imp/footprint_generator/footprint_generator.ui
	imp/footprint_generator/dual.svg
	imp/footprint_generator/single.svg
	imp/footprint_generator/quad.svg
	imp/footprint_generator/grid.svg
	imp/rules/rules_window.ui
	imp/rules/rule_editor_hole_size.ui
	imp/rules/rule_editor_track_width.ui
	imp/rules/rule_editor_clearance_copper.ui
	imp/fab_output.ui
	imp/3d_view.ui
	imp/step_export.ui
	prj-mgr/part_browser/part_browser.ui
	global.css
	lib/canvas/shaders/grid-vertex.glsl
	lib/canvas/shaders/grid-fragment.glsl
	lib/canvas/shaders/selectable-vertex.glsl
	lib/canvas/shaders/selectable-fragment.glsl
	lib/canvas/shaders/selectable-geometry.glsl
	lib/canvas/shaders/selection-vertex.glsl
	lib/canvas/shaders/selection-fragment.glsl
	lib/canvas/shaders/triangle-vertex.glsl
	lib/canvas/shaders/triangle-fragment.glsl
	lib/canvas/shaders/triangle-geometry.glsl
	lib/canvas/shaders/marker-vertex.glsl
	lib/canvas/shaders/marker-fragment.glsl
	lib/canvas/shaders/marker-geometry.glsl
	lib/canvas/shaders/selection-line-vertex.glsl
	lib/canvas/shaders/selection-line-fragment.glsl
	lib/canvas/shaders/selection-line-geometry.glsl
	lib/canvas3d/shaders/cover-vertex.glsl
	lib/canvas3d/shaders/cover-fragment.glsl
	lib/canvas3d/shaders/cover-geometry.glsl
	lib/canvas3d/shaders/wall-vertex.glsl
	lib/canvas3d/shaders/wall-fragment.glsl
	lib/canvas3d/shaders/wall-geometry.glsl
	lib/canvas3d/shaders/face-vertex.glsl
	lib/canvas3d/shaders/face-fragment.glsl
	lib/canvas3d/shaders/face-geometry.glsl
	lib/canvas3d/shaders/background-vertex.glsl
	lib/canvas3d/shaders/background-fragment.glsl
	)

SET(RESOURCE_CPP "${CMAKE_CURRENT_BINARY_DIR}/resources.c")

compile_gresources(RESOURCE_FILE
                   CML_OUT
                   TARGET ${RESOURCE_CPP}
                   TYPE EMBED_C
                   RESOURCES ${RESOURCE_LIST}
				   PREFIX "net/carrotIndustries/horizon")

ADD_CUSTOM_TARGET(resource ALL DEPENDS ${RESOURCE_FILE})

ADD_SUBDIRECTORY(auto-build)
ADD_SUBDIRECTORY(lib)

ADD_SUBDIRECTORY(imp)
ADD_SUBDIRECTORY(pgm-test)
ADD_SUBDIRECTORY(pool-mgr)
ADD_SUBDIRECTORY(pool-update-parametric)
ADD_SUBDIRECTORY(pool-util)
ADD_SUBDIRECTORY(prj-mgr)
ADD_SUBDIRECTORY(prj-util)
