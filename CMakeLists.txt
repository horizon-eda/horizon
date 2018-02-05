CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT(horizon)

# https://github.com/Makman2/GCR_CMake
list(APPEND CMAKE_MODULE_PATH
	${PROJECT_SOURCE_DIR}/GCR_CMake/macros)
include(GlibCompileResourcesSupport)

set(RESOURCE_LIST
	src/window.ui
	src/lib/util/window_state_schema.sql
	src/lib/imp-util/layer_display_default.json
	src/lib/imp-util/app_menu.ui
	src/lib/imp-util/preferences.ui
	src/lib/property_panels/property_panel.ui
	src/lib/pool-update/schema.sql
	src/pool-update-parametric/schema.sql
	src/prj-mgr/window.ui
	src/prj-mgr/app_menu.ui
	src/prj-mgr/prefs.ui
	src/prj-mgr/pool_cache_window.ui
	src/pool-mgr/window.ui
	src/pool-mgr/app_menu.ui
	src/pool-mgr/unit_editor.ui
	src/pool-mgr/entity_editor.ui
	src/pool-mgr/part_editor.ui
	src/pool-mgr/part_wizard/part_wizard.ui
	src/pool-mgr/pool_merge.ui
	src/icons/scalable/actions/edit-schematic-symbolic.svg
	src/icons/scalable/actions/edit-board-symbolic.svg
	src/icons/scalable/actions/action-link-symbolic.svg
	src/icons/scalable/actions/action-unlink-symbolic.svg
	src/icon.svg
	src/lib/imp-util/footprint_generator/footprint_generator.ui
	src/lib/imp-util/footprint_generator/dual.svg
	src/lib/imp-util/footprint_generator/single.svg
	src/lib/imp-util/footprint_generator/quad.svg
	src/lib/imp-util/footprint_generator/grid.svg
	src/lib/imp-util/rules/rules_window.ui
	src/lib/imp-util/rules/rule_editor_hole_size.ui
	src/lib/imp-util/rules/rule_editor_track_width.ui
	src/lib/imp-util/rules/rule_editor_clearance_copper.ui
	src/lib/imp-util/fab_output.ui
	src/lib/imp-util/3d_view.ui
	src/lib/imp-util/step_export.ui
	src/prj-mgr/part_browser/part_browser.ui
	src/canvas/grid-vertex.glsl
	src/canvas/grid-fragment.glsl
	src/canvas/selectable-vertex.glsl
	src/canvas/selectable-fragment.glsl
	src/canvas/selectable-geometry.glsl
	src/canvas/selection-vertex.glsl
	src/canvas/selection-fragment.glsl
	src/canvas/triangle-vertex.glsl
	src/canvas/triangle-fragment.glsl
	src/canvas/triangle-geometry.glsl
	src/canvas/marker-vertex.glsl
	src/canvas/marker-fragment.glsl
	src/canvas/marker-geometry.glsl
	src/canvas/selection-line-vertex.glsl
	src/canvas/selection-line-fragment.glsl
	src/canvas/selection-line-geometry.glsl
	src/canvas/cover-vertex.glsl
	src/canvas/cover-fragment.glsl
	src/canvas/cover-geometry.glsl
	src/canvas/wall-vertex.glsl
	src/canvas/wall-fragment.glsl
	src/canvas/wall-geometry.glsl
	src/canvas/face-vertex.glsl
	src/canvas/face-fragment.glsl
	src/canvas/face-geometry.glsl
	src/canvas/background-vertex.glsl
	src/canvas/background-fragment.glsl
	)

SET(RESOURCE_CPP "${CMAKE_CURRENT_BINARY_DIR}/resources.c")

compile_gresources(RESOURCE_FILE
                   CML_OUT
                   TARGET ${RESOURCE_CPP}
                   TYPE EMBED_C
                   RESOURCES ${RESOURCE_LIST}
                   PREFIX "/net/carrotIndustries/horizon")

ADD_CUSTOM_TARGET(resource ALL DEPENDS ${RESOURCE_FILE})

SET(CMAKE_CXX_STANDARD 14)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

INCLUDE_DIRECTORIES(src/lib)
INCLUDE_DIRECTORIES(3rd_party)
INCLUDE_DIRECTORIES(3rd_party/router)
INCLUDE_DIRECTORIES(3rd_party/router/include)

FIND_PACKAGE(Boost)
IF (Boost_FOUND)
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
	LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF()
FIND_PACKAGE(Threads)
FIND_PACKAGE(PkgConfig  REQUIRED)
IF(UNIX)
	pkg_check_modules(UUID    REQUIRED uuid)
ENDIF(UNIX)
pkg_check_modules(DEPS    REQUIRED sigc++-2.0)
pkg_check_modules(GTKMM   REQUIRED gtkmm-3.0)
pkg_check_modules(GLIBMM  REQUIRED glibmm-2.4)
pkg_check_modules(EPOXY   REQUIRED epoxy)
pkg_check_modules(RSVG    REQUIRED librsvg-2.0)
pkg_check_modules(YAML    REQUIRED yaml-cpp)
pkg_check_modules(ZMQ     REQUIRED libzmq)
pkg_check_modules(SQLITE  REQUIRED sqlite3)
pkg_check_modules(CAIROMM REQUIRED cairomm-pdf-1.0)
pkg_check_modules(GIT     REQUIRED libgit2)
pkg_check_modules(CURL    REQUIRED libcurl)
pkg_check_modules(GLM     REQUIRED glm)
pkg_check_modules(GLIB    REQUIRED glib-2.0)
pkg_check_modules(GLM     REQUIRED glm)

INCLUDE_DIRECTORIES(${UUID_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${DEPS_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GTKMM_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GLIBMM_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${EPOXY_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${RSVG_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${YAML_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${ZMQ_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${SQLITE_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CAIROMM_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GIT_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GLM_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIRS})

IF(UNIX)
	INCLUDE_DIRECTORIES(/opt/opencascade/inc/)
	LINK_DIRECTORIES(/opt/opencascade/lib/)
ENDIF(UNIX)
IF(WIN32)
	INCLUDE_DIRECTORIES(C:/msys64/mingw64/include/oce/)
ENDIF(WIN32)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
SET(CMAKE_CXX_FLAGS "-g3")
#SET(CMAKE_CXX_FLAGS "-o3")
IF(WIN32)
	ADD_DEFINITIONS(-DWIN32_UUID)
	SET(WIN_LIBS rpcrt4)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-subsystem,windows")
ENDIF(WIN32)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(3rd_party)
