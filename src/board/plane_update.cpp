#include "board/board.hpp"
#include "canvas/canvas_patch.hpp"
#include "canvas/canvas_pads.hpp"
#include "board/board_layers.hpp"

namespace horizon {

	static void polynode_to_fragment(Plane *plane, const ClipperLib::PolyNode *node) {
		assert(node->IsHole()==false);
		plane->fragments.emplace_back();
		auto &fragment = plane->fragments.back();
		fragment.paths.emplace_back();
		auto &outer = fragment.paths.back();
		outer = node->Contour; //first path is countour

		for(auto child: node->Childs) {
			assert(child->IsHole()==true);

			fragment.paths.emplace_back();
			auto &hole = fragment.paths.back();
			hole = child->Contour;

			for(auto child2: child->Childs) { //add fragments in holes
				polynode_to_fragment(plane, child2);
			}
		}
	}


	static void transform_path(ClipperLib::Path &path, const Placement &tr) {
		for(auto &it: path) {
			Coordi p(it.X, it.Y);
			auto q = tr.transform(p);
			it.X = q.x;
			it.Y = q.y;
		}
	}

	void Board::update_plane(Plane *plane, CanvasPatch *ca_ext, CanvasPads *ca_pads_ext) {
		CanvasPatch ca_my;
		CanvasPatch *ca = ca_ext;
		if(!ca_ext) {
			ca_my.update(*this);
			ca = &ca_my;
		}

		CanvasPads ca_pads_my;
		CanvasPads *ca_pads = ca_pads_ext;
		if(!ca_pads_ext) {
			ca_pads_my.update(*this);
			ca_pads = &ca_pads_my;
		}

		plane->fragments.clear();

		ClipperLib::Clipper cl_plane;
		ClipperLib::Path poly_path; //path from polygon contour
		auto poly = plane->polygon->remove_arcs();
		poly_path.reserve(poly.vertices.size());
		for(const auto &pt: poly.vertices) {
			poly_path.emplace_back(ClipperLib::IntPoint(pt.position.x, pt.position.y));
		}
		ClipperLib::JoinType jt  = ClipperLib::jtRound;
		switch(plane->settings.style) {
			case PlaneSettings::Style::ROUND :
				jt = ClipperLib::jtRound;
			break;

			case PlaneSettings::Style::SQUARE :
				jt = ClipperLib::jtSquare;
			break;

			case PlaneSettings::Style::MITER :
				jt = ClipperLib::jtMiter;
			break;
		}

		{
			ClipperLib::ClipperOffset ofs_poly; //shrink polygon contour by min_width/2
			ofs_poly.ArcTolerance = 2e3;
			ofs_poly.AddPath(poly_path, jt, ClipperLib::etClosedPolygon);
			ClipperLib::Paths poly_shrink;
			ofs_poly.Execute(poly_shrink, -((double)plane->settings.min_width)/2);
			cl_plane.AddPaths(poly_shrink, ClipperLib::ptSubject, true);
		}

		double twiddle = .005_mm;

		for(const auto &patch: ca->patches) { // add cutouts
			if((patch.first.layer == poly.layer && patch.first.net != plane->net->uuid && patch.first.type != PatchType::OTHER && patch.first.type != PatchType::TEXT) ||
			   (patch.first.layer == 10000 && patch.first.type == PatchType::HOLE_NPTH)) {
				ClipperLib::ClipperOffset ofs; //expand patch for cutout
				ofs.ArcTolerance = 2e3;
				ofs.AddPaths(patch.second, jt, ClipperLib::etClosedPolygon);
				ClipperLib::Paths patch_exp;

				int64_t clearance = 0;
				if(patch.first.type != PatchType::HOLE_NPTH) { //copper
					auto patch_net= patch.first.net?&block->nets.at(patch.first.net):nullptr;
					auto rule_clearance = rules.get_clearance_copper(plane->net, patch_net, poly.layer);
					if(rule_clearance) {
						clearance = rule_clearance->get_clearance(patch.first.type, PatchType::PLANE);
					}
				}
				else { //npth
					clearance = rules.get_clearance_copper_other(plane->net, poly.layer)->get_clearance(PatchType::PLANE, PatchType::HOLE_NPTH);
				}
				double expand = clearance+plane->settings.min_width/2+twiddle;

				ofs.Execute(patch_exp, expand);
				cl_plane.AddPaths(patch_exp, ClipperLib::ptClip, true);
			}
		}

		auto text_clearance = rules.get_clearance_copper_other(plane->net, poly.layer)->get_clearance(PatchType::PLANE, PatchType::TEXT);

		//add text cutouts
		if(plane->settings.text_style == PlaneSettings::TextStyle::EXPAND) {
			for(const auto &patch: ca->patches) { // add cutouts
				if(patch.first.layer == poly.layer && patch.first.type == PatchType::TEXT) {
					ClipperLib::ClipperOffset ofs; //expand patch for cutout
					ofs.ArcTolerance = 2e3;
					ofs.AddPaths(patch.second, jt, ClipperLib::etClosedPolygon);
					ClipperLib::Paths patch_exp;

					double expand = text_clearance+plane->settings.min_width/2+twiddle;

					ofs.Execute(patch_exp, expand);
					cl_plane.AddPaths(patch_exp, ClipperLib::ptClip, true);
				}
			}
		}
		else {
			for(const auto &ext: ca->text_extents) { // add cutouts
				Coordi a,b;
				int text_layer;
				std::tie(text_layer, a, b) = ext;
				if(text_layer == poly.layer) {
					ClipperLib::Path p = {{a.x, a.y}, {b.x, a.y}, {b.x, b.y}, {a.x, b.y}};

					ClipperLib::ClipperOffset ofs; //expand patch for cutout
					ofs.ArcTolerance = 2e3;
					ofs.AddPath(p, jt, ClipperLib::etClosedPolygon);
					ClipperLib::Paths patch_exp;

					double expand = text_clearance+plane->settings.min_width/2+twiddle;

					ofs.Execute(patch_exp, expand);
					cl_plane.AddPaths(patch_exp, ClipperLib::ptClip, true);
				}
			}
		}

		//add thermal coutouts
		if(plane->settings.connect_style == PlaneSettings::ConnectStyle::THERMAL) {
			for(const auto &patch: ca->patches) {
				if((patch.first.layer == poly.layer && patch.first.net == plane->net->uuid && (patch.first.type == PatchType::PAD || patch.first.type == PatchType::PAD_TH))) {
					ClipperLib::ClipperOffset ofs; //expand patch for cutout
					ofs.ArcTolerance = 2e3;
					ClipperLib::Paths pad;
					ClipperLib::SimplifyPolygons(patch.second, pad, ClipperLib::pftNonZero);

					ofs.AddPaths(pad, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
					ClipperLib::Paths patch_exp;

					double expand = plane->settings.thermal_gap_width+plane->settings.min_width/2;
					ofs.Execute(patch_exp, expand);
					cl_plane.AddPaths(patch_exp, ClipperLib::ptClip, true);
				}
			}
		}
		ClipperLib::Paths out; //the plane before expanding by min_width
		cl_plane.Execute(ClipperLib::ctDifference, out, ClipperLib::pftNonZero); //do cutouts

		//do board outline clearance
		CanvasPatch::PatchKey outline_key;
		outline_key.layer = BoardLayers::L_OUTLINE;
		outline_key.net = UUID();
		outline_key.type = PatchType::OTHER;
		if(ca->patches.count(outline_key) != 0) {
			auto &patch_outline = ca->patches.at(outline_key);
			//cleanup board outline so that it conforms to nonzero filling rule
			ClipperLib::Paths board_outline;
			{
				ClipperLib::Clipper cl_outline;
				cl_outline.AddPaths(patch_outline, ClipperLib::ptSubject, true);
				cl_outline.Execute(ClipperLib::ctUnion, board_outline, ClipperLib::pftEvenOdd);
			}

			//board outline contracted by clearance
			ClipperLib::Paths paths_ofs;
			{
				ClipperLib::ClipperOffset ofs;
				ofs.ArcTolerance = 10e3;
				ofs.AddPaths(board_outline, jt, ClipperLib::etClosedPolygon);
				auto clearance = rules.get_clearance_copper_other(plane->net, poly.layer)->get_clearance(PatchType::PLANE, PatchType::BOARD_EDGE);
				ofs.Execute(paths_ofs, -1.0*(clearance+plane->settings.min_width/2+twiddle*2));
			}

			//intersect polygon with contracted board outline
			ClipperLib::Paths temp;
			{
				ClipperLib::Clipper isect;
				isect.AddPaths(paths_ofs, ClipperLib::ptClip, true);
				isect.AddPaths(out, ClipperLib::ptSubject, true);
				isect.Execute(ClipperLib::ctIntersection, temp, ClipperLib::pftNonZero);
			}
			out = temp;
		}


		ClipperLib::PolyTree tree;
		ClipperLib::ClipperOffset ofs;
		ofs.ArcTolerance = 2e3;
		ofs.AddPaths(out, jt, ClipperLib::etClosedPolygon);

		if(plane->settings.connect_style == PlaneSettings::ConnectStyle::THERMAL) {
			ClipperLib::Paths expanded;
			ofs.Execute(expanded, plane->settings.min_width/2);

			ClipperLib::Clipper cl_add_spokes;
			cl_add_spokes.AddPaths(expanded, ClipperLib::ptSubject, true);
			auto thermals = get_thermals(plane, ca_pads);
			for(const auto &spoke: thermals) {
				ClipperLib::Paths test_result;
				ClipperLib::Clipper cl_test;
				cl_test.AddPaths(expanded, ClipperLib::ptSubject, true);
				cl_test.AddPath(spoke, ClipperLib::ptClip, true);
				cl_test.Execute(ClipperLib::ctIntersection, test_result, ClipperLib::pftNonZero);

				if(test_result.size())
					cl_add_spokes.AddPath(spoke, ClipperLib::ptClip, true);
			}
			cl_add_spokes.Execute(ClipperLib::ctUnion, tree, ClipperLib::pftNonZero);
		}
		else {
			ofs.Execute(tree, plane->settings.min_width/2);
		}

		for(const auto node: tree.Childs) {
			polynode_to_fragment(plane, node);
		}

		//orphan all
		for(auto &it: plane->fragments) {
			it.orphan = true;
		}

		//find orphans
		for(auto &frag: plane->fragments) {
			for(const auto &it: junctions) {
				if(it.second.net == plane->net && (it.second.layer == plane->polygon->layer || it.second.has_via) && frag.contains(it.second.position)) {
					frag.orphan = false;
					break;
				}
			}
			if(frag.orphan) { //still orphan, check pads
				for(auto &it: packages) {
					auto pkg = &it.second;
					for(auto &it_pad: it.second.package.pads) {
						auto pad = &it_pad.second;
						if(pad->net == plane->net) {
							Track::Connection conn(pkg, pad);
							auto pos = conn.get_position();
							auto ps_type = pad->padstack.type;
							auto layer = plane->polygon->layer;
							if(frag.contains(pos) && (ps_type == Padstack::Type::THROUGH || (ps_type == Padstack::Type::TOP && layer == BoardLayers::TOP_COPPER) || (ps_type == Padstack::Type::BOTTOM && layer == BoardLayers::BOTTOM_COPPER))) {
								frag.orphan = false;
								break;
							}
						}
					}
					if(!frag.orphan) { //don't need to check other packages
						break;
					}
				}
			}
		}

		if(!plane->settings.keep_orphans) { //delete orphans
			plane->fragments.erase(std::remove_if(plane->fragments.begin(), plane->fragments.end(), [](const auto &x){return x.orphan;}), plane->fragments.end());
		}
	}

	void Board::update_planes() {
		std::vector<Plane*> planes_sorted;
		planes_sorted.reserve(planes.size());
		for(auto &it: planes) {
			it.second.fragments.clear();
			planes_sorted.push_back(&it.second);
		}
		std::sort(planes_sorted.begin(), planes_sorted.end(), [](const auto a, const auto b){return a->priority < b->priority;});

		CanvasPads cp;
		cp.update(*this);

		CanvasPatch ca;
		ca.update(*this);
		for(auto plane: planes_sorted) {
			update_plane(plane, &ca, &cp);
			ca.patches.clear();
			ca.text_extents.clear();
			ca.update(*this); //update so that new plane sees other planes
		}
	}


	ClipperLib::Paths Board::get_thermals(Plane *plane, CanvasPads *cp) const {
		ClipperLib::Paths ret;
		for(const auto &it: cp->pads) {
			if(it.first.layer != plane->polygon->layer)
				continue;

			auto net_from_pad = packages.at(it.first.package).package.pads.at(it.first.pad).net;
			if(net_from_pad != plane->net)
				continue;

			ClipperLib::Paths uni; //union of all off the pads polygons
			ClipperLib::SimplifyPolygons(it.second.second, uni, ClipperLib::pftNonZero);

			ClipperLib::ClipperOffset ofs; //
			ofs.ArcTolerance = 2e3;
			ofs.AddPaths(uni, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
			ClipperLib::Paths pad_exp; //pad expanded by gap width

			double expand = plane->settings.thermal_gap_width+.01_mm;
			ofs.Execute(pad_exp, expand);

			auto p0 = pad_exp.front().front();
			Coordi pc0(p0.X, p0.Y);
			std::pair<Coordi, Coordi> bb(pc0, pc0);
			for(const auto &itp: pad_exp) {
				for(const auto &itc: itp) {
					Coordi p(itc.X, itc.Y);
					bb.first = Coordi::min(bb.first, p);
					bb.second = Coordi::max(bb.second, p);
				}
			}
			auto w = bb.second.x - bb.first.x;
			auto h = bb.second.y - bb.first.y;
			auto l = std::max(w,h);


			int64_t spoke_width = plane->settings.thermal_spoke_width;
			ClipperLib::Path spoke;
			spoke.emplace_back(-spoke_width/2, -spoke_width/2);
			spoke.emplace_back(-spoke_width/2, spoke_width/2);
			spoke.emplace_back(l, spoke_width/2);
			spoke.emplace_back(l, -spoke_width/2);

			ClipperLib::Paths antipad;
			{

				for(int angle = 0; angle<360; angle+=90) {
					ClipperLib::Clipper cl;
					cl.AddPaths(pad_exp, ClipperLib::ptSubject, true);

					ClipperLib::Path r1 = spoke;
					Placement tr;
					tr.set_angle_deg(angle);
					transform_path(r1, tr);
					transform_path(r1, it.second.first);
					cl.AddPath(r1, ClipperLib::ptClip, true);

					cl.Execute(ClipperLib::ctIntersection, antipad, ClipperLib::pftNonZero);
					ret.insert(ret.end(), antipad.begin(), antipad.end());
				}
			}
		}
		return ret;
	}
}
