#include "canvas3d.hpp"
#include "canvas/gl_util.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <cmath>
#include "common/polygon.hpp"
#include "common/hole.hpp"
#include "poly2tri/poly2tri.h"
#include "board/board_layers.hpp"
#include "board/board.hpp"
#include "util/step_importer.hpp"
#include "logger/logger.hpp"
#include <thread>

namespace horizon {

	Canvas3D::Canvas3D(): Gtk::GLArea(), CanvasPatch::CanvasPatch(), cover_renderer(this), wall_renderer(this), face_renderer(this), background_renderer(this) {
		add_events(
		           Gdk::BUTTON_PRESS_MASK|
		           Gdk::BUTTON_RELEASE_MASK|
		           Gdk::BUTTON_MOTION_MASK|
				   Gdk::SCROLL_MASK|
				   Gdk::SMOOTH_SCROLL_MASK
		);

		models_loading_dispatcher.connect([this] {
			package_height_max = 0;
			for(const auto &it: face_vertex_buffer) {
				package_height_max = std::max(it.z, package_height_max);
			}
			request_push();
			s_signal_models_loading.emit(false);
		});
	}
	void Canvas3D::on_size_allocate(Gtk::Allocation &alloc) {
		width = alloc.get_width();
		height = alloc.get_height();


		//chain up
		Gtk::GLArea::on_size_allocate(alloc);

	}


	bool Canvas3D::on_button_press_event(GdkEventButton* button_event) {
		if(button_event->button == 2 || (button_event->button==1 && (button_event->state & Gdk::SHIFT_MASK))) {
			pan_mode = PanMode::MOVE;
			pointer_pos_orig = {button_event->x, button_event->y};
			center_orig = center;
		}
		else if(button_event->button==1) {
			pan_mode = PanMode::ROTATE;
			pointer_pos_orig = {button_event->x, button_event->y};
			cam_elevation_orig = cam_elevation;
			cam_azimuth_orig = cam_azimuth;
		}
		return Gtk::GLArea::on_button_press_event(button_event);
	}

	bool Canvas3D::on_motion_notify_event(GdkEventMotion *motion_event) {
		auto delta = glm::mat2(1, 0, 0, -1)*(glm::vec2(motion_event->x, motion_event->y) - pointer_pos_orig);
		if(pan_mode == PanMode::ROTATE) {
			cam_azimuth = cam_azimuth_orig-(delta.x/width)*360;
			cam_elevation = CLAMP(cam_elevation_orig-(delta.y/height)*90, -89.9, 89.9);
			queue_draw();
		}
		else if(pan_mode == PanMode::MOVE) {
			center = center_orig+glm::rotate(glm::mat2(1, 0, 0, sin(glm::radians(cam_elevation)))*delta*0.1f, glm::radians(cam_azimuth-90));
			queue_draw();
		}
		return Gtk::GLArea::on_motion_notify_event(motion_event);
	}

	bool Canvas3D::on_button_release_event(GdkEventButton *button_event) {
		pan_mode = PanMode::NONE;
		return Gtk::GLArea::on_button_release_event(button_event);
	}

	bool Canvas3D::on_scroll_event(GdkEventScroll *scroll_event) {
		float dist_new = cam_distance;
		if(scroll_event->direction == GDK_SCROLL_UP) {
			dist_new = cam_distance/1.5;
		}
		else if(scroll_event->direction == GDK_SCROLL_DOWN) {
			dist_new = cam_distance*1.5;
		}
		else if(scroll_event->direction == GDK_SCROLL_SMOOTH) {
			gdouble sx, sy;
			gdk_event_get_scroll_deltas((GdkEvent*)scroll_event, &sx, &sy);
			dist_new = cam_distance * powf(1.5, sy);
		}
		cam_distance = std::max(0.f, dist_new);
		queue_draw();
		return Gtk::GLArea::on_scroll_event(scroll_event);
	}

	void Canvas3D::request_push() {
		needs_push = true;
		queue_draw();
	}

	void Canvas3D::push() {
		cover_renderer.push();
		wall_renderer.push();
		face_renderer.push();
	}

	void Canvas3D::on_realize() {
		Gtk::GLArea::on_realize();
		make_current();
		set_has_depth_buffer(true);
		cover_renderer.realize();
		wall_renderer.realize();
		face_renderer.realize();
		background_renderer.realize();
		glEnable(GL_DEPTH_TEST);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		GL_CHECK_ERROR
	}

	void Canvas3D::add_path(int layer, const ClipperLib::Path &path) {
		for(size_t i = 0; i<path.size(); i++) {
			layers[layer].walls.emplace_back(path[i].X, path[i].Y);
			layers[layer].walls.emplace_back(path[(i+1)%path.size()].X, path[(i+1)%path.size()].Y);
		}
	}

	void Canvas3D::polynode_to_tris(const ClipperLib::PolyNode *node, int layer) {
		assert(node->IsHole()==false);

		std::vector<p2t::Point> point_store;
		size_t pts_total = node->Contour.size();
		for(const auto child: node->Childs)
			pts_total += child->Contour.size();
		point_store.reserve(pts_total); //important so that iterators won't get invalidated

		try {
			std::vector<p2t::Point*> contour;
			contour.reserve(node->Contour.size());
			for(const auto &p: node->Contour) {
				point_store.emplace_back(p.X, p.Y);
				contour.push_back(&point_store.back());
			}
			p2t::CDT cdt(contour);
			for(const auto child: node->Childs) {
				std::vector<p2t::Point*> hole;
				hole.reserve(child->Contour.size());
				for(const auto &p: child->Contour) {
					point_store.emplace_back(p.X, p.Y);
					hole.push_back(&point_store.back());
				}
				cdt.AddHole(hole);
			}
			cdt.Triangulate();
			auto tris = cdt.GetTriangles();

			for(const auto &tri: tris) {
				for(int i = 0; i<3; i++) {
					auto p = tri->GetPoint(i);
					layers[layer].tris.emplace_back(p->x, p->y);
				}
			}
		}
		catch (const std::runtime_error &e) {
			Logger::log_critical("error triangulating", Logger::Domain::BOARD, e.what());
		}
		catch (...) {
			Logger::log_critical("error triangulating", Logger::Domain::BOARD, "unspecified error");
		}

		layers[layer].walls.reserve(pts_total);
		add_path(layer, node->Contour);
		for(auto child: node->Childs) {
			add_path(layer, child->Contour);
		}

		for(auto child: node->Childs) {
			assert(child->IsHole()==true);
			for(auto child2: child->Childs) { //add fragments in holes
				polynode_to_tris(child2, layer);
			}
		}
	}

	void Canvas3D::update2(const Board &b) {
		brd = &b;
		update(*brd);
		prepare_packages();
		prepare();
	}

	void Canvas3D::prepare() {
		if(!brd)
			return;
		layers.clear();

		float board_thickness = -((float)brd->stackup.at(0).thickness);
		int n_inner_layers = brd->get_n_inner_layers();
		for(const auto &it: brd->stackup) {
			board_thickness += it.second.thickness + it.second.substrate_thickness;
		}
		board_thickness /= 1e6;

		int layer = BoardLayers::TOP_COPPER;
		layers[layer].offset=0;
		layers[layer].thickness = brd->stackup.at(0).thickness/1e6;
		layers[layer].color = {1, .8, 0};
		layers[layer].explode_mul=1;
		prepare_layer(layer);

		layer = BoardLayers::BOTTOM_COPPER;
		layers[layer].offset=-board_thickness;
		layers[layer].thickness = +(brd->stackup.at(layer).thickness/1e6);
		layers[layer].color = {1, .8, 0};
		layers[layer].explode_mul=-2*n_inner_layers-1;
		prepare_layer(layer);

		{
			float offset = -(brd->stackup.at(0).substrate_thickness/1e6);
			for(int i = 0; i<n_inner_layers; i++) {
				layer = -i-1;
				layers[layer].offset=offset;
				layers[layer].thickness=-(brd->stackup.at(layer).thickness/1e6);
				layers[layer].color = {1, .8, 0};
				layers[layer].explode_mul=-1-2*i;
				offset -= brd->stackup.at(layer).thickness/1e6+brd->stackup.at(layer).substrate_thickness/1e6;
				prepare_layer(layer);
			}
		}

		if(show_substrate) {
			layer = BoardLayers::L_OUTLINE;
			layers[layer].offset=0;
			layers[layer].thickness=-(brd->stackup.at(0).substrate_thickness/1e6);
			layers[layer].explode_mul=0;
			layers[layer].color = {.2, .15, 0};
			prepare_layer(layer);

			float offset = -(brd->stackup.at(0).substrate_thickness/1e6);
			for(int i = 0; i<n_inner_layers; i++) {
				int l = 10000+i;
				offset -= brd->stackup.at(-i-1).thickness/1e6;
				layers[l] = layers[layer];
				layers[l].offset = offset;
				layers[l].thickness = -(brd->stackup.at(-i-1).substrate_thickness/1e6);
				layers[l].explode_mul = -2-2*i;

				offset -= brd->stackup.at(-i-1).substrate_thickness/1e6;
			}
		}

		if(show_solder_mask) {
			layer = BoardLayers::TOP_MASK;
			layers[layer].offset=.036;
			layers[layer].thickness=0.035;
			layers[layer].color = solder_mask_color;
			layers[layer].alpha = .8;
			layers[layer].explode_mul=2;
			prepare_soldermask(layer);

			layer = BoardLayers::BOTTOM_MASK;
			layers[layer].offset=-board_thickness-.036;
			layers[layer].thickness=0.035;
			layers[layer].color = solder_mask_color;
			layers[layer].alpha = .8;
			layers[layer].explode_mul=-2*n_inner_layers-2;
			prepare_soldermask(layer);
		}

		if(show_silkscreen) {
			layer = BoardLayers::TOP_SILKSCREEN;
			layers[layer].offset=.07;
			layers[layer].thickness=0.035;
			layers[layer].color = {1, 1, 1};
			layers[layer].explode_mul=3;
			prepare_layer(layer);

			layer = BoardLayers::BOTTOM_SILKSCREEN;
			layers[layer].offset=-board_thickness-.07;
			layers[layer].thickness=-0.035;
			layers[layer].color = {1, 1, 1};
			layers[layer].explode_mul=-2*n_inner_layers-3;
			prepare_layer(layer);
		}

		layer = 20000; //pth holes
		layers[layer].offset = 0;
		layers[layer].thickness = -board_thickness;
		layers[layer].color = {1, .8, 0};
		for(const auto &it: patches) {
			if(it.first.layer == 10000 && it.first.type == PatchType::HOLE_PTH) {
				ClipperLib::ClipperOffset ofs;
				ofs.AddPaths(it.second, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
				ClipperLib::Paths res;
				ofs.Execute(res, -.001_mm);
				for(const auto &path: res) {
					add_path(layer, path);
				}
			}
		}

		bbox.first = glm::vec3();
		bbox.second = glm::vec3();
		for(const auto &it: patches) {
			for(const auto &path: it.second) {
				for(const auto &p: path) {
					glm::vec3 q(p.X/1e6, p.Y/1e6, 0);
					bbox.first = glm::min(bbox.first, q);
					bbox.second = glm::max(bbox.second, q);
				}
			}
		}
		request_push();
	}

	void Canvas3D::prepare_soldermask(int layer) {
		ClipperLib::Clipper cl;
		for(const auto &it: patches) {
			if(it.first.layer == BoardLayers::L_OUTLINE) { //add outline
				cl.AddPaths(it.second, ClipperLib::ptSubject, true);
			}
			else if(it.first.layer == layer) {
				cl.AddPaths(it.second, ClipperLib::ptClip, true);
			}
		}

		ClipperLib::PolyTree pt;
		cl.Execute(ClipperLib::ctDifference, pt, ClipperLib::pftEvenOdd, ClipperLib::pftNonZero);

		for(const auto node: pt.Childs) {
			polynode_to_tris(node, layer);
		}

	}

	void Canvas3D::prepare_layer(int layer) {
		ClipperLib::Clipper cl;
		for(const auto &it: patches) {
			if(it.first.layer == layer) {
				cl.AddPaths(it.second, ClipperLib::ptSubject, true);
			}
		}
		ClipperLib::Paths result;
		auto pft = ClipperLib::pftNonZero;
		if(layer == BoardLayers::L_OUTLINE) {
			pft = ClipperLib::pftEvenOdd;
		}
		cl.Execute(ClipperLib::ctUnion, result, pft);

		ClipperLib::PolyTree pt;
		cl.Clear();
		cl.AddPaths(result, ClipperLib::ptSubject, true);
		for(const auto &it: patches) {
			if(it.first.layer == 10000 && (it.first.type == PatchType::HOLE_NPTH || it.first.type == PatchType::HOLE_PTH)) {
				cl.AddPaths(it.second, ClipperLib::ptClip, true);
			}
		}
		cl.Execute(ClipperLib::ctDifference, pt, pft, ClipperLib::pftNonZero);

		for(const auto node: pt.Childs) {
			polynode_to_tris(node, layer);
		}
	}

	float Canvas3D::get_layer_offset(int layer) {
		return layers[layer].offset+layers[layer].explode_mul*explode;
	}

	void Canvas3D::load_3d_model(const UUID &uu, const std::string &filename) {
		if(models.count(uu))
			return;

		auto faces = STEPImporter::import(filename);
		//canvas->face_vertex_buffer.reserve(faces.size());
		size_t vertex_offset = face_vertex_buffer.size();
		size_t first_index = face_index_buffer.size();
		for(const auto &face: faces) {
			for(const auto &v: face.vertices) {
				face_vertex_buffer.emplace_back(v.x, v.y, v.z, face.color.r*255, face.color.g*255, face.color.b*255);
			}
			for(const auto &tri: face.triangle_indices) {
				size_t a,b,c;
				std::tie(a,b,c) = tri;
				face_index_buffer.push_back(a+vertex_offset);
				face_index_buffer.push_back(b+vertex_offset);
				face_index_buffer.push_back(c+vertex_offset);
			}
			vertex_offset += face.vertices.size();
		}
		size_t last_index = face_index_buffer.size();
		models.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(first_index, last_index-first_index));
	}

	void Canvas3D::load_models_thread(std::map<UUID, std::string> model_filenames) {
		std::lock_guard<std::mutex> lock(models_loading_mutex);
		for(const auto &it: model_filenames) {
			load_3d_model(it.first, it.second);
		}
		models_loading_dispatcher.emit();
	}

	void Canvas3D::load_models_async(Pool *pool, bool from_pool) {
		std::map<UUID, std::string> model_filenames;
		for(const auto &it: brd->packages) {
			std::string mfn;
			auto uu = it.second.pool_package->uuid;
			if(from_pool) {
				mfn = pool->get_3d_model_filename(uu);
			}
			else {
				if(it.second.package.model_filename.size())
					mfn = Glib::build_filename(pool->get_base_path(), it.second.package.model_filename);
			}
			if(mfn.size())
				model_filenames[uu] = mfn;
		}
		s_signal_models_loading.emit(true);
		std::thread thr(&Canvas3D::load_models_thread, this, model_filenames);

		thr.detach();
	}

	void Canvas3D::clear_3d_models() {
		face_vertex_buffer.clear();
		face_index_buffer.clear();
		models.clear();
	}

	void Canvas3D::prepare_packages() {
		package_transform_idxs.clear();
		package_transforms.clear();
		std::map<UUID, std::set<const BoardPackage*>> pkg_map;
		for(const auto &it: brd->packages) {
			pkg_map[it.second.pool_package->uuid].insert(&it.second);
		}

		for(const auto &it_pkg: pkg_map) {
			size_t size_before = package_transforms.size();
			for(const auto &it_brd_pkg: it_pkg.second) {
				const auto &pl = it_brd_pkg->placement;
				package_transforms.emplace_back(pl.shift.x/1e6, pl.shift.y/1e6, pl.get_angle(), it_brd_pkg->flip);
			}
			size_t size_after = package_transforms.size();
			package_transform_idxs[it_pkg.first] = {size_before, size_after-size_before};
		}
	}


	bool Canvas3D::on_render(const Glib::RefPtr<Gdk::GLContext> &context) {
		if(needs_push) {
			push();
			needs_push = false;
		}

		glClearColor(.5, .5, .5, 1.0);
		glClearDepth(10);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		GL_CHECK_ERROR

		glDisable(GL_DEPTH_TEST);
		background_renderer.render();
		glEnable(GL_DEPTH_TEST);


		layers[20000].offset = get_layer_offset(BoardLayers::TOP_COPPER);
		layers[20000].thickness = -(get_layer_offset(BoardLayers::TOP_COPPER)-get_layer_offset(BoardLayers::BOTTOM_COPPER));

		float r = cam_distance;
		float phi = glm::radians(cam_azimuth);
		float theta = glm::radians(90-cam_elevation);
		auto cam_offset = glm::vec3(r*sin(theta)*cos(phi), r*sin(theta)*sin(phi), r*cos(theta));
		auto cam_pos = cam_offset+glm::vec3(center, 0);

		glm::vec3 right(
			sin(phi - 3.14f/2.0f),
			cos(phi - 3.14f/2.0f),
			0
		);

		viewmat = glm::lookAt(
			cam_pos,
			glm::vec3(center, 0),
			glm::vec3(0,0,1)
		);

		float cam_dist_min = std::max(std::abs(cam_pos.z)-(10+explode*(brd->get_n_inner_layers()*2+3)), 1.0f);
		float cam_dist_max = 0;

		float zmin = -10-explode*(brd->get_n_inner_layers()*2+3+package_height_max);
		float zmax = 10+explode*2+package_height_max;
		std::array<glm::vec3, 8> bbs = {
			glm::vec3(bbox.first.x, bbox.first.y, zmin),
			glm::vec3(bbox.first.x, bbox.second.y, zmin),
			glm::vec3(bbox.second.x, bbox.first.y, zmin),
			glm::vec3(bbox.second.x, bbox.second.y, zmin),
			glm::vec3(bbox.first.x, bbox.first.y, zmax),
			glm::vec3(bbox.first.x, bbox.second.y, zmax),
			glm::vec3(bbox.second.x, bbox.first.y, zmax),
			glm::vec3(bbox.second.x, bbox.second.y, zmax)
		};

		for(const auto &bb: bbs) {
			float dist = glm::length(bb-cam_pos);
			cam_dist_max = std::max(dist, cam_dist_max);
			cam_dist_min = std::min(dist, cam_dist_min);
		}

		projmat = glm::perspective(glm::radians(cam_fov), width/height, cam_dist_min, cam_dist_max);

		cam_normal = glm::normalize(cam_offset);
		wall_renderer.render();

		if(show_models)
			face_renderer.render();

		cover_renderer.render();
		glFlush();

		return Gtk::GLArea::on_render(context);
	}


}
