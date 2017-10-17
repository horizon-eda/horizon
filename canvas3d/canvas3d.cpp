#include "canvas3d.hpp"
#include "canvas/gl_util.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <cmath>
#include "polygon.hpp"
#include "hole.hpp"
#include "canvas/poly2tri/poly2tri.h"
#include "board_layers.hpp"

namespace horizon {

	Canvas3D::Canvas3D(): Gtk::GLArea(), CanvasPatch::CanvasPatch(), cover_renderer(this), wall_renderer(this) {
		add_events(
		           Gdk::BUTTON_PRESS_MASK|
		           Gdk::BUTTON_RELEASE_MASK|
		           Gdk::BUTTON_MOTION_MASK|
				   Gdk::SCROLL_MASK|
				   Gdk::SMOOTH_SCROLL_MASK
		);
	}
	void Canvas3D::on_size_allocate(Gtk::Allocation &alloc) {
		width = alloc.get_width();
		height = alloc.get_height();


		//chain up
		Gtk::GLArea::on_size_allocate(alloc);

	}


	bool Canvas3D::on_button_press_event(GdkEventButton* button_event) {
		if(button_event->button==1) {
			pan_mode = PanMode::ROTATE;
			pointer_pos_orig = {button_event->x, button_event->y};
			cam_elevation_orig = cam_elevation;
			cam_azimuth_orig = cam_azimuth;
		}
		else if(button_event->button == 2) {
			pan_mode = PanMode::MOVE;
			pointer_pos_orig = {button_event->x, button_event->y};
			center_orig = center;
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
	}

	void Canvas3D::on_realize() {
		Gtk::GLArea::on_realize();
		make_current();
		set_has_depth_buffer(true);
		cover_renderer.realize();
		wall_renderer.realize();
		glEnable(GL_DEPTH_TEST);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		GL_CHECK_ERROR
	}

	void Canvas3D::polynode_to_tris(const ClipperLib::PolyNode *node, int layer) {
		assert(node->IsHole()==false);

		std::vector<p2t::Point> point_store;
		size_t pts_total = node->Contour.size();
		for(const auto child: node->Childs)
			pts_total += child->Contour.size();
		point_store.reserve(pts_total); //important so that iterators won't get invalidated

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

		auto add_path = [this, layer](const ClipperLib::Path &path) {
			for(size_t i = 0; i<path.size(); i++) {
				layers[layer].walls.emplace_back(path[i].X, path[i].Y);
				layers[layer].walls.emplace_back(path[(i+1)%path.size()].X, path[(i+1)%path.size()].Y);
			}
		};

		layers[layer].walls.reserve(pts_total);
		add_path(node->Contour);
		for(auto child: node->Childs) {
			add_path(child->Contour);
		}

		for(auto child: node->Childs) {
			assert(child->IsHole()==true);
			for(auto child2: child->Childs) { //add fragments in holes
				polynode_to_tris(child2, layer);
			}
		}
	}

	void Canvas3D::prepare() {
		layers.clear();

		float board_thickness = 2;

		int layer = BoardLayers::TOP_COPPER;
		layers[layer].offset=0;
		layers[layer].thickness=0.035;
		layers[layer].color = {1, .8, 0};
		layers[layer].explode_mul=1;
		prepare_layer(layer);

		layer = BoardLayers::BOTTOM_COPPER;
		layers[layer].offset=-board_thickness;
		layers[layer].thickness=-0.035;
		layers[layer].color = {1, .8, 0};
		layers[layer].explode_mul=-1;
		prepare_layer(layer);

		layer = BoardLayers::L_OUTLINE;
		layers[layer].offset=0;
		layers[layer].thickness=-board_thickness;
		layers[layer].explode_mul=0;
		layers[layer].color = {.2, .15, 0};
		prepare_layer(layer);

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
			layers[layer].explode_mul=-2;
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
			layers[layer].explode_mul=-3;
			prepare_layer(layer);
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


	bool Canvas3D::on_render(const Glib::RefPtr<Gdk::GLContext> &context) {
		if(needs_push) {
			push();
			needs_push = false;
		}

		glClearColor(.5, .5, .5, 1.0);
		glClearDepth(10);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		GL_CHECK_ERROR

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

		float cam_dist_min = std::max(std::abs(cam_pos.z)-(10+explode*3), 1.0f);
		float cam_dist_max = 0;

		std::array<glm::vec3, 4> bbs = {
			glm::vec3(bbox.first.x, bbox.first.y, 0),
			glm::vec3(bbox.first.x, bbox.second.y, 0),
			glm::vec3(bbox.second.x, bbox.first.y, 0),
			glm::vec3(bbox.second.x, bbox.second.y, 0)
		};

		for(const auto &bb: bbs) {
			float dist = glm::length(bb-cam_pos);
			cam_dist_max = std::max(dist, cam_dist_max);
			cam_dist_min = std::min(dist, cam_dist_min);
		}

		projmat = glm::perspective(glm::radians(cam_fov), width/height, cam_dist_min, cam_dist_max);

		cam_normal = glm::normalize(cam_offset);
		cover_renderer.render();
		wall_renderer.render();
		glFlush();

		return Gtk::GLArea::on_render(context);
	}


}
