#include "pns_horizon_iface.hpp"
#include "board/board.hpp"
#include "router/pns_topology.h"
#include "router/pns_via.h"
#include "canvas/canvas_gl.hpp"
#include "clipper/clipper.hpp"
#include "router/pns_solid.h"
#include "router/pns_debug_decorator.h"
#include "geometry/shape_convex.h"
#include "board/board_layers.hpp"
#include "board/via_padstack_provider.hpp"
#include "util/util.hpp"

namespace PNS {

	int PNS_HORIZON_IFACE::layer_to_router(int l) {
		switch(l) {
			case horizon::BoardLayers::TOP_COPPER:		return F_Cu;
			case horizon::BoardLayers::BOTTOM_COPPER:	return B_Cu;
			case horizon::BoardLayers::IN1_COPPER:		return In1_Cu;
			case horizon::BoardLayers::IN2_COPPER:		return In2_Cu;
			case horizon::BoardLayers::IN3_COPPER:		return In3_Cu;
			case horizon::BoardLayers::IN4_COPPER:		return In4_Cu;
			default : return UNDEFINED_LAYER;
		}

		return F_Cu;
	}

	int PNS_HORIZON_IFACE::layer_from_router(int l) {
		int lo = 0;
		switch(l) {
			case F_Cu:     lo=horizon::BoardLayers::TOP_COPPER; break;
			case B_Cu:     lo=horizon::BoardLayers::BOTTOM_COPPER; break;
			case In1_Cu:   lo=horizon::BoardLayers::IN1_COPPER; break;
			case In2_Cu:   lo=horizon::BoardLayers::IN2_COPPER; break;
			case In3_Cu:   lo=horizon::BoardLayers::IN3_COPPER; break;
			case In4_Cu:   lo=horizon::BoardLayers::IN4_COPPER; break;
			default:
				assert(false);
		}
		assert(l==layer_to_router(lo));
		return lo;
	}


	class PNS_HORIZON_RULE_RESOLVER : public PNS::RULE_RESOLVER {
		public:
			PNS_HORIZON_RULE_RESOLVER(const horizon::Board *aBoard, horizon::BoardRules *aRules, PNS::ROUTER* aRouter );
			virtual ~PNS_HORIZON_RULE_RESOLVER();

			virtual int Clearance( const PNS::ITEM* aA, const PNS::ITEM* aB ) const override;
			virtual int Clearance( int aNetCode ) const override;
			virtual void OverrideClearance( bool aEnable, int aNetA = 0, int aNetB = 0, int aClearance = 0 ) override;
			virtual void UseDpGap( bool aUseDpGap ) override { m_useDpGap = aUseDpGap; }
			virtual int DpCoupledNet( int aNet ) override;
			virtual int DpNetPolarity( int aNet ) override;
			virtual bool DpNetPair( PNS::ITEM* aItem, int& aNetP, int& aNetN ) override;

		private:
			PNS::ROUTER*    m_router;
			const horizon::Board* m_board;
			horizon::BoardRules *m_rules;
			PNS_HORIZON_IFACE *m_iface = nullptr;

		bool m_useDpGap=false;
	};


	PNS_HORIZON_RULE_RESOLVER::PNS_HORIZON_RULE_RESOLVER(const horizon::Board * aBoard, horizon::BoardRules *rules, PNS::ROUTER* aRouter ) :
    m_router( aRouter ),
    m_board( aBoard ),
	m_rules ( rules)
	{
    PNS::NODE* world = m_router->GetWorld();

    PNS::TOPOLOGY topo( world );

    m_iface = static_cast<PNS_HORIZON_IFACE*>(m_router->GetInterface());

	}


	PNS_HORIZON_RULE_RESOLVER::~PNS_HORIZON_RULE_RESOLVER()
	{
	}


	static horizon::PatchType patch_type_from_kind(PNS::ITEM::PnsKind kind) {
		switch(kind) {
			case PNS::ITEM::VIA_T :
				return horizon::PatchType::VIA;
			case PNS::ITEM::SOLID_T :
				return horizon::PatchType::PAD;
			case PNS::ITEM::LINE_T :
				return horizon::PatchType::TRACK;
			case PNS::ITEM::SEGMENT_T :
				return horizon::PatchType::TRACK;
		}
		assert(false);
		return horizon::PatchType::OTHER;
	}

	static const PNS_HORIZON_PARENT_ITEM parent_dummy_outline;

	int PNS_HORIZON_RULE_RESOLVER::Clearance( const PNS::ITEM* aA, const PNS::ITEM* aB ) const {
		auto net_a = m_iface->get_net_for_code(aA->Net());
		auto net_b = m_iface->get_net_for_code(aB->Net());

		auto pt_a = patch_type_from_kind(aA->Kind());
		auto pt_b = patch_type_from_kind(aB->Kind());

		auto parent_a = aA->Parent();
		auto parent_b = aB->Parent();

		auto layers_a = aA->Layers();
		auto layers_b = aB->Layers();

		if(parent_a && parent_a->pad && parent_a->pad->padstack.type == horizon::Padstack::Type::THROUGH)
			pt_a = horizon::PatchType::PAD_TH;
		else if(parent_a && parent_a->hole && parent_a->hole->padstack.type == horizon::Padstack::Type::HOLE)
			pt_a = horizon::PatchType::HOLE_PTH;
		else if(parent_a && parent_a->hole && parent_a->hole->padstack.type == horizon::Padstack::Type::MECHANICAL)
			pt_a = horizon::PatchType::HOLE_NPTH;

		if(parent_b && parent_b->pad && parent_b->pad->padstack.type == horizon::Padstack::Type::THROUGH)
			pt_b = horizon::PatchType::PAD_TH;
		else if(parent_b && parent_b->hole && parent_b->hole->padstack.type == horizon::Padstack::Type::HOLE)
			pt_b = horizon::PatchType::HOLE_PTH;
		else if(parent_b && parent_b->hole && parent_b->hole->padstack.type == horizon::Padstack::Type::MECHANICAL)
			pt_b = horizon::PatchType::HOLE_NPTH;

		std::cout << "pt " << static_cast<int>(pt_a) << " " << static_cast<int>(pt_b) << std::endl;

		if(parent_a == &parent_dummy_outline || parent_b == &parent_dummy_outline) { //one is the board edge
			auto a_is_edge = parent_a == &parent_dummy_outline;

			//use layers of non-edge thing
			auto layers = a_is_edge?layers_b:layers_a;
			auto pt = a_is_edge?pt_b:pt_a;
			auto net = net_a?net_a:net_b; //only one has net

			//fixme: handle multiple layers for non-edge thing
			auto layer = layers.Start();

			auto clearance = m_rules->get_clearance_copper_other(net, PNS_HORIZON_IFACE::layer_from_router(layer));
			return clearance->get_clearance(pt, horizon::PatchType::BOARD_EDGE)+clearance->routing_offset;
		}

		if((parent_a && parent_a->pad && parent_a->pad->padstack.type == horizon::Padstack::Type::MECHANICAL) ||
		   (parent_b && parent_b->pad && parent_b->pad->padstack.type == horizon::Padstack::Type::MECHANICAL) ||
		   (parent_a && parent_a->hole && parent_a->hole->padstack.type == horizon::Padstack::Type::MECHANICAL) ||
		   (parent_b && parent_b->hole && parent_b->hole->padstack.type == horizon::Padstack::Type::MECHANICAL)
		  )
		{
			std::cout << "npth" << std::endl;
			 //any is mechanical (NPTH)
			auto net = net_a?net_a:net_b; //only one has net
			auto a_is_npth = (parent_a && parent_a->pad && parent_a->pad->padstack.type == horizon::Padstack::Type::MECHANICAL) ||
					         (parent_a && parent_a->hole && parent_a->hole->padstack.type == horizon::Padstack::Type::MECHANICAL);

			//use layers of non-npth ting
			auto layers = a_is_npth?layers_b:layers_a;
			auto pt = a_is_npth?pt_b:pt_a;

			//fixme: handle multiple layers for non-npth thing
			auto layer = layers.Start();

			auto clearance = m_rules->get_clearance_copper_other(net, PNS_HORIZON_IFACE::layer_from_router(layer));
			return clearance->get_clearance(pt, horizon::PatchType::HOLE_NPTH)+clearance->routing_offset;
		}


		int layer = UNDEFINED_LAYER;
		if(!layers_a.IsMultilayer() && !layers_b.IsMultilayer()) //all on single layer
			layer = layers_b.Start();
		else if(!layers_a.IsMultilayer() && layers_b.IsMultilayer()) //b is multi
			layer = layers_a.Start();
		else if(layers_a.IsMultilayer() && !layers_b.IsMultilayer()) //a is muli
			layer = layers_b.Start();
		else if(layers_a.IsMultilayer() && layers_b.IsMultilayer()) //both are multi
			layer = layers_b.Start(); //fixme, good enough for now

		assert(layer != UNDEFINED_LAYER);

		auto clearance = m_rules->get_clearance_copper(net_a, net_b, PNS_HORIZON_IFACE::layer_from_router(layer));
		return clearance->get_clearance(pt_a, pt_b)+clearance->routing_offset;
	}


	int PNS_HORIZON_RULE_RESOLVER::Clearance( int aNetCode ) const {
		//only used for display purposes, can return dummy value
		return .1e6;
	}


	// fixme: ugly hack to make the optimizer respect gap width for currently routed differential pair.
	void PNS_HORIZON_RULE_RESOLVER::OverrideClearance( bool aEnable, int aNetA, int aNetB , int aClearance )
	{
		/*m_overrideEnabled = aEnable;
		m_overrideNetA = aNetA;
		m_overrideNetB = aNetB;
		m_overrideClearance = aClearance;*/
	}


	int PNS_HORIZON_RULE_RESOLVER::DpCoupledNet( int aNet )
	{
		auto net = m_iface->get_net_for_code(aNet);
		if(net->diffpair) {
			return m_iface->get_net_code(net->diffpair->uuid);
		}
		return -1;
	}


	int PNS_HORIZON_RULE_RESOLVER::DpNetPolarity( int aNet )
	{
	   if(m_iface->get_net_for_code(aNet)->diffpair_master)
		   return 1;
	   else
		   return -1;
	}


	bool PNS_HORIZON_RULE_RESOLVER::DpNetPair( PNS::ITEM* aItem, int& aNetP, int& aNetN )
	{
		if(!aItem)
			return false;
		auto net = m_iface->get_net_for_code(aItem->Net());
		if(net->diffpair) {
			if(net->diffpair_master) {
				aNetP = m_iface->get_net_code(net->uuid);
				aNetN = m_iface->get_net_code(net->diffpair->uuid);
			}
			else {
				aNetN = m_iface->get_net_code(net->uuid);
				aNetP = m_iface->get_net_code(net->diffpair->uuid);
			}
			return true;
		}
		return false;
	}

		class PNS_HORIZON_DEBUG_DECORATOR: public PNS::DEBUG_DECORATOR
	{
	public:
		PNS_HORIZON_DEBUG_DECORATOR( horizon::CanvasGL *canvas ): PNS::DEBUG_DECORATOR(),
			m_canvas( canvas )
		{

		}

		~PNS_HORIZON_DEBUG_DECORATOR()
		{
			Clear();
		}

		void AddPoint( VECTOR2I aP, int aColor ) override
		{
			SHAPE_LINE_CHAIN l;

			l.Append( aP - VECTOR2I( -50000, -50000 ) );
			l.Append( aP + VECTOR2I( -50000, -50000 ) );

			AddLine( l, aColor, 10000 );

			l.Clear();
			l.Append( aP - VECTOR2I( 50000, -50000 ) );
			l.Append( aP + VECTOR2I( 50000, -50000 ) );

			AddLine( l, aColor, 10000 );
		}

		void AddBox( BOX2I aB, int aColor ) override
		{
			SHAPE_LINE_CHAIN l;

			VECTOR2I o = aB.GetOrigin();
			VECTOR2I s = aB.GetSize();

			l.Append( o );
			l.Append( o.x + s.x, o.y );
			l.Append( o.x + s.x, o.y + s.y );
			l.Append( o.x, o.y + s.y );
			l.Append( o );

			AddLine( l, aColor, 10000 );
		}

		void AddSegment( SEG aS, int aColor ) override
		{
			SHAPE_LINE_CHAIN l;

			l.Append( aS.A );
			l.Append( aS.B );

			AddLine( l, aColor, 10000 );
		}

		void AddDirections( VECTOR2D aP, int aMask, int aColor ) override
		{
			BOX2I b( aP - VECTOR2I( 10000, 10000 ), VECTOR2I( 20000, 20000 ) );

			AddBox( b, aColor );
			for( int i = 0; i < 8; i++ )
			{
				if( ( 1 << i ) & aMask )
				{
					VECTOR2I v = DIRECTION_45( ( DIRECTION_45::Directions ) i ).ToVector() * 100000;
					AddSegment( SEG( aP, aP + v ), aColor );
				}
			}
		}

		void AddLine( const SHAPE_LINE_CHAIN& aLine, int aType, int aWidth ) override
		{
			auto npts = aLine.PointCount();
			std::deque<horizon::Coordi> pts;
			for(int i = 0; i<npts; i++) {
				auto pt = aLine.CPoint(i);
				pts.emplace_back(pt.x, pt.y);
			}
			lines.insert(m_canvas->add_line(pts, aWidth, horizon::ColorP::YELLOW, 10000));
		}

		void Clear() override
		{
			std::cout << "debug clear" << std::endl;
			for(auto &li: lines) {
				m_canvas->remove_obj(li);
			}
			lines.clear();
		}

	private:
		horizon::CanvasGL * m_canvas;
		std::set<horizon::ObjectRef> lines;
	};

	PNS_HORIZON_IFACE::PNS_HORIZON_IFACE() {

	}



	void PNS_HORIZON_IFACE::SetBoard(horizon::Board *brd) {
		board = brd;
	}

	void PNS_HORIZON_IFACE::SetCanvas( horizon::CanvasGL *ca) {
		canvas = ca;
	}

	void PNS_HORIZON_IFACE::SetRules( horizon::BoardRules *ru) {
		rules = ru;
	}

	void PNS_HORIZON_IFACE::SetViaPadstackProvider( horizon::ViaPadstackProvider *v) {
		vpp = v;
	}

	int PNS_HORIZON_IFACE::get_net_code(const horizon::UUID &uu) {
		if(net_code_map.count(uu)) {
			return net_code_map.at(uu);
		}
		else {
			net_code_max++;
			net_code_map.emplace(uu, net_code_max);
			net_code_map_r.emplace(net_code_max, uu);
			return net_code_max;
		}
	}

	horizon::Net *PNS_HORIZON_IFACE::get_net_for_code(int code) {
		if(code == PNS::ITEM::UnusedNet)
			return nullptr;
		if(!net_code_map_r.count(code))
			return nullptr;
		return &board->block->nets.at(net_code_map_r.at(code));
	}

	const PNS_HORIZON_PARENT_ITEM *PNS_HORIZON_IFACE::get_parent(const horizon::Track *track) {
		PNS_HORIZON_PARENT_ITEM it(track);
		return &*parents.insert(it).first;
	}

	const PNS_HORIZON_PARENT_ITEM *PNS_HORIZON_IFACE::get_parent(const horizon::BoardHole *hole) {
		PNS_HORIZON_PARENT_ITEM it(hole);
		return &*parents.insert(it).first;
	}

	const PNS_HORIZON_PARENT_ITEM *PNS_HORIZON_IFACE::get_parent(const horizon::Via *via) {
		PNS_HORIZON_PARENT_ITEM it(via);
		return &*parents.insert(it).first;
	}

	const PNS_HORIZON_PARENT_ITEM *PNS_HORIZON_IFACE::get_parent(const horizon::BoardPackage *pkg, const horizon::Pad *pad) {
		PNS_HORIZON_PARENT_ITEM it(pkg, pad);
		return &*parents.insert(it).first;
	}

	std::unique_ptr<PNS::SEGMENT> PNS_HORIZON_IFACE::syncTrack(const horizon::Track *track) {
		auto from = track->from.get_position();
		auto to = track->to.get_position();
		int net = PNS::ITEM::UnusedNet;
		if(track->net)
			net = get_net_code(track->net->uuid);
		std::unique_ptr< PNS::SEGMENT > segment(
			new PNS::SEGMENT( SEG(from.x, from.y, to.x, to.y),  net)
		);

		segment->SetWidth(track->width);
		segment->SetLayer(layer_to_router(track->layer));
		segment->SetParent( get_parent(track) );

		if(track->locked)
			segment->Mark( PNS::MK_LOCKED );

		return segment;
	}

	void PNS_HORIZON_IFACE::syncOutline(const horizon::Polygon *ipoly, PNS::NODE *aWorld) {
		auto poly = ipoly->remove_arcs();
		for(size_t i = 0; i<poly.vertices.size(); i++) {
			auto from = poly.vertices[i].position;
			auto to = poly.vertices[(i+1)%poly.vertices.size()].position;

			std::set<int> layers = {horizon::BoardLayers::TOP_COPPER, horizon::BoardLayers::BOTTOM_COPPER};
			for(unsigned int j = 0; j<board->get_n_inner_layers(); j++) {
				layers.insert(-j-1);
			}
			for(const auto layer: layers) {
				std::unique_ptr< PNS::SEGMENT > segment(
					new PNS::SEGMENT( SEG(from.x, from.y, to.x, to.y),  PNS::ITEM::UnusedNet)
				);
				segment->SetWidth(10);//very small
				segment->SetLayers(layer_to_router(layer));
				segment->SetParent(&parent_dummy_outline);
				segment->Mark( PNS::MK_LOCKED );
				aWorld->Add( std::move( segment ) );
			}

		}

	}

	std::unique_ptr<PNS::SOLID>   PNS_HORIZON_IFACE::syncPad(const horizon::BoardPackage *pkg, const horizon::Pad *pad) {
		horizon::Placement tr(pkg->placement);
		if(pkg->flip) {
			tr.invert_angle();
		}
		tr.accumulate(pad->placement);
		auto solid = syncPadstack(&pad->padstack, tr);

		if(pad->net)
			solid->SetNet( get_net_code(pad->net->uuid) );
		solid->SetParent( get_parent(pkg, pad) );

		return solid;
	}

	std::unique_ptr<PNS::SOLID>   PNS_HORIZON_IFACE::syncHole(const horizon::BoardHole *hole) {
		auto solid = syncPadstack(&hole->padstack, hole->placement);

		if(hole->net)
			solid->SetNet( get_net_code(hole->net->uuid) );
		solid->SetParent( get_parent(hole) );

		return solid;
	}

	std::unique_ptr<PNS::SOLID>   PNS_HORIZON_IFACE::syncPadstack(const horizon::Padstack *padstack, const horizon::Placement &tr) {
		int layer_min = 10000;
		int layer_max = -10000;

		ClipperLib::Clipper clipper;

		if(padstack->type != horizon::Padstack::Type::MECHANICAL) { //normal pad
			auto add_polygon = [&clipper, &tr, padstack](const auto &poly) {
				ClipperLib::Path path;
				if(poly.vertices.size() == 0) {
					throw std::runtime_error("polygon without vertices in padstack at" + horizon::coord_to_string(tr.shift));
				}
				for(auto &v: poly.vertices) {
					auto p = tr.transform(v.position);
					path.emplace_back(p.x, p.y);
				}
				if(ClipperLib::Orientation(path)) {
					std::reverse(path.begin(), path.end());
				}
				clipper.AddPath(path, ClipperLib::ptSubject, true);
			};
			for(auto &it: padstack->shapes) {
				if(horizon::BoardLayers::is_copper(it.second.layer)) { //on copper layer
					layer_min = std::min(layer_min, it.second.layer);
					layer_max = std::max(layer_max, it.second.layer);
					add_polygon(it.second.to_polygon().remove_arcs());
				}
			}
			for(auto &it: padstack->polygons) {
				if(horizon::BoardLayers::is_copper(it.second.layer)) { //on copper layer
					layer_min = std::min(layer_min, it.second.layer);
					layer_max = std::max(layer_max, it.second.layer);
					add_polygon(it.second.remove_arcs());
				}
			}


		}
		else { //npth pad
			layer_min = horizon::BoardLayers::BOTTOM_COPPER;
			layer_max = horizon::BoardLayers::TOP_COPPER;
			for(auto &it: padstack->holes) {
				auto poly = it.second.to_polygon().remove_arcs();
				ClipperLib::Path path;
				for(auto &v: poly.vertices) {
					auto p = tr.transform(v.position);
					path.emplace_back(p.x, p.y);
				}
				if(ClipperLib::Orientation(path)) {
					std::reverse(path.begin(), path.end());
				}
				clipper.AddPath(path, ClipperLib::ptSubject, true);
			}
		}

		ClipperLib::Paths poly_union;
		clipper.Execute(ClipperLib::ctUnion, poly_union, ClipperLib::pftNonZero);
		if(poly_union.size() != 1) {
			throw std::runtime_error("invalid pad polygons: "+ std::to_string(poly_union.size()));
		}

		std::unique_ptr< PNS::SOLID > solid( new PNS::SOLID );

		solid->SetLayers( LAYER_RANGE( layer_to_router(layer_max), layer_to_router(layer_min) ));

		solid->SetOffset( VECTOR2I( 0,0 ) );
		solid->SetPos ( VECTOR2I( tr.shift.x, tr.shift.y) );

		SHAPE_CONVEX* shape = new SHAPE_CONVEX();

		for(auto &pt: poly_union.front()) {
			shape->Append(pt.X, pt.Y);
		}
        solid->SetShape( shape );

		return solid;
	}

	std::unique_ptr<PNS::VIA>     PNS_HORIZON_IFACE::syncVia(const horizon::Via *via) {
		auto pos = via->junction->position;
		int net = PNS::ITEM::UnusedNet;
		if(via->junction->net)
			net = get_net_code(via->junction->net->uuid);
		std::unique_ptr<PNS::VIA> pvia( new PNS::VIA(
            VECTOR2I(pos.x, pos.y),
            LAYER_RANGE( layer_to_router(horizon::BoardLayers::TOP_COPPER),  layer_to_router(horizon::BoardLayers::BOTTOM_COPPER)),
            via->parameter_set.at(horizon::ParameterID::VIA_DIAMETER),
            via->parameter_set.at(horizon::ParameterID::HOLE_DIAMETER),
            net,
            VIA_THROUGH )
		);

		//via->SetParent( aVia );
		pvia->SetParent( get_parent(via) );

		if(via->locked)
			pvia->Mark( PNS::MK_LOCKED );

		return pvia;
	}

	void PNS_HORIZON_IFACE::SyncWorld( PNS::NODE *aWorld ) {
		std::cout << "!!!sync world" << std::endl;
		if(!board) {
			wxLogTrace( "PNS", "No board attached, aborting sync." );
			return;
		}
		parents.clear();

		for(const auto &it: board->tracks) {
			auto segment = syncTrack( &it.second );
            if( segment ) {
                aWorld->Add( std::move( segment ) );
            }
		}

		for(const auto &it: board->vias) {
			auto via = syncVia( &it.second );
            if( via ) {
                aWorld->Add( std::move( via ) );
            }
		}

		for(const auto &it: board->holes) {
			auto hole = syncHole( &it.second );
            if( hole ) {
                aWorld->Add( std::move( hole ) );
            }
		}

		for(const auto &it: board->packages) {
			for(auto &it_pad: it.second.package.pads) {
				auto pad = syncPad( &it.second, &it_pad.second );
				if( pad ) {
					aWorld->Add( std::move( pad ) );
				}
			}
		}
		for(const auto &it: board->polygons) {
			if(it.second.layer == horizon::BoardLayers::L_OUTLINE) {
				syncOutline(&it.second, aWorld);
			}
		}
		int worstClearance = rules->get_max_clearance();

		delete m_ruleResolver;
		m_ruleResolver = new PNS_HORIZON_RULE_RESOLVER( board, rules, m_router );

		aWorld->SetRuleResolver( m_ruleResolver );
		aWorld->SetMaxClearance( 4 * worstClearance );
	}


	void PNS_HORIZON_IFACE::EraseView() {
		std::cout << "iface erase view" << std::endl;

		canvas->show_all_obj();

		for(const auto &it: m_preview_items) {
			canvas->remove_obj(it);
		}
		m_preview_items.clear();
		if( m_debugDecorator )
			m_debugDecorator->Clear();

	}


	void PNS_HORIZON_IFACE::DisplayItem( const PNS::ITEM* aItem, int aColor, int aClearance ) {
		wxLogTrace( "PNS", "DisplayItem %p %s", aItem ,aItem->KindStr().c_str());
		if(aItem->Kind() == PNS::ITEM::LINE_T) {
			auto line_item = dynamic_cast<const PNS::LINE*>(aItem);
			auto npts = line_item->PointCount();
			std::deque<horizon::Coordi> pts;
			for(int i = 0; i<npts; i++) {
				auto pt = line_item->CPoint(i);
				pts.emplace_back(pt.x, pt.y);
			}
			int la = line_item->Layer();
			m_preview_items.insert(canvas->add_line(pts, line_item->Width(), horizon::ColorP::FROM_LAYER, layer_from_router(la)));
		}
		else if(aItem->Kind() == PNS::ITEM::SEGMENT_T) {
			auto seg_item = dynamic_cast<const PNS::SEGMENT*>(aItem);
			auto seg = seg_item->Seg();
			std::deque<horizon::Coordi> pts;
			pts.emplace_back(seg.A.x, seg.A.y);
			pts.emplace_back(seg.B.x, seg.B.y);
			int la = seg_item->Layer();
			m_preview_items.insert(canvas->add_line(pts, seg_item->Width(), horizon::ColorP::FROM_LAYER, layer_from_router(la)));
		}
		else if(aItem->Kind() == PNS::ITEM::VIA_T) {
			auto via_item = dynamic_cast<const PNS::VIA*>(aItem);
			auto pos = via_item->Pos();

			std::deque<horizon::Coordi> pts;
			pts.emplace_back(pos.x, pos.y);
			pts.emplace_back(pos.x+10, pos.y);
			int la = via_item->Layers().Start();
			m_preview_items.insert(canvas->add_line(pts, via_item->Diameter(), horizon::ColorP::FROM_LAYER, layer_from_router(la)));
			la = via_item->Layers().End();
			m_preview_items.insert(canvas->add_line(pts, via_item->Diameter(), horizon::ColorP::FROM_LAYER, layer_from_router(la)));
		}
		else {
			assert(false);
		}

	}


	void PNS_HORIZON_IFACE::HideItem( PNS::ITEM* aItem ) {
		std::cout << "iface hide item" << std::endl;
		auto parent = aItem->Parent();
		if(parent) {
			if(parent->track) {
				horizon::ObjectRef ref(horizon::ObjectType::TRACK, parent->track->uuid);
				canvas->hide_obj(ref);
			}
			else if(parent->via) {
				horizon::ObjectRef ref(horizon::ObjectType::VIA, parent->via->uuid);
				canvas->hide_obj(ref);
			}
		}
	}


	void PNS_HORIZON_IFACE::RemoveItem( PNS::ITEM* aItem ) {

		auto parent = aItem->Parent();
		std::cout << "!!!iface remove item " << parent << " " << aItem->KindStr()  << std::endl;
		if(parent) {
			if(parent->track) {
				board->tracks.erase(parent->track->uuid);
			}
			else if(parent->via) {
				board->vias.erase(parent->via->uuid);
			}
		}
		//board->expand(true);
	}

	std::pair<horizon::BoardPackage *, horizon::Pad *> PNS_HORIZON_IFACE::find_pad(int layer, const horizon::Coordi &c) {
		for(auto &it: board->packages) {
			for(auto &it_pad: it.second.package.pads) {
				auto pt = it_pad.second.padstack.type;
				if((pt == horizon::Padstack::Type::TOP && layer == horizon::BoardLayers::TOP_COPPER) ||
					(pt == horizon::Padstack::Type::BOTTOM && layer == horizon::BoardLayers::BOTTOM_COPPER) ||
					(pt == horizon::Padstack::Type::THROUGH)
				){
					auto tr = it.second.placement;
					if(it.second.flip) {
						tr.invert_angle();
					}
					auto pos = tr.transform(it_pad.second.placement.shift);
					if(pos == c) {
						return {&it.second, &it_pad.second};
					}
				}
			}
		}
		return {nullptr, nullptr};
	}

	horizon::Junction *PNS_HORIZON_IFACE::find_junction(int layer, const horizon::Coordi &c) {
		for(auto &it: board->junctions) {
			if((layer == 10000 || it.second.layer == layer || it.second.has_via) && it.second.position == c) {
				return &it.second;
			}
		}
		return nullptr;
	}

	void PNS_HORIZON_IFACE::AddItem( PNS::ITEM* aItem ) {
		std::cout << "!!!iface add item" << std::endl;
		switch( aItem->Kind() ) {
			case PNS::ITEM::SEGMENT_T: {
				PNS::SEGMENT* seg = static_cast<PNS::SEGMENT*>( aItem );
				const SEG& s = seg->Seg();
				auto uu = horizon::UUID::random();
				auto track = &board->tracks.emplace(uu, uu).first->second;
				horizon::Coordi from(s.A.x, s.A.y);
				horizon::Coordi to(s.B.x, s.B.y);
				track->width = seg->Width();

				auto layer = layer_from_router(seg->Layer());
				track->layer = layer;

				auto connect =  [this, &layer](horizon::Track::Connection &conn, const horizon::Coordi &c){
					auto p = find_pad(layer, c);
					if(p.first) {
						conn.connect(p.first, p.second);
					}
					else {
						auto j = find_junction(layer, c);
						if(j) {
							conn.connect(j);
						}
						else {
							auto juu = horizon::UUID::random();
							auto ju = &board->junctions.emplace(juu, juu).first->second;
							ju->layer = layer;
							ju->position = c;
							conn.connect(ju);
						}
					}

				};
				connect(track->from, from);
				connect(track->to, to);
				track->width_from_rules = m_router->Sizes().WidthFromRules();
				aItem->SetParent( get_parent(track) );

			} break;

			case PNS::ITEM::VIA_T: {
				PNS::VIA* pvia = static_cast<PNS::VIA*>( aItem );
				auto uu = horizon::UUID::random();
				auto net = get_net_for_code(pvia->Net());
				auto padstack = vpp->get_padstack(rules->get_via_padstack_uuid(net));
				if(padstack) {
					auto ps = rules->get_via_parameter_set(net);
					auto via = &board->vias.emplace(std::piecewise_construct, std::forward_as_tuple(uu), std::forward_as_tuple(uu, padstack)).first->second;
					via->parameter_set = ps;

					horizon::Coordi c(pvia->Pos().x, pvia->Pos().y);
					auto j = find_junction(10000, c);
					if(j) {
						via->junction = j;
					}
					else {
						auto juu = horizon::UUID::random();
						auto ju = &board->junctions.emplace(juu, juu).first->second;
						ju->position = c;
						via->junction = ju;
					}
					via->junction->has_via = true;
				}
			} break;

			default:
				std::cout << "!!!unhandled add " << aItem->KindStr() << std::endl;
		}

	}


	void PNS_HORIZON_IFACE::Commit() {
		board->expand(true);
		EraseView();
	}


	void PNS_HORIZON_IFACE::UpdateNet( int aNetCode )
	{
		wxLogTrace( "PNS", "Update-net %d", aNetCode );
	}


	PNS::RULE_RESOLVER* PNS_HORIZON_IFACE::GetRuleResolver()
	{
		return m_ruleResolver;
	}



	void PNS_HORIZON_IFACE::create_debug_decorator(horizon::CanvasGL *ca) {
		if(!m_debugDecorator) {
			m_debugDecorator = new PNS_HORIZON_DEBUG_DECORATOR(ca);
		}
	}

	PNS::DEBUG_DECORATOR *PNS_HORIZON_IFACE::GetDebugDecorator()
	{
		return m_debugDecorator;
	}


	PNS_HORIZON_IFACE::~PNS_HORIZON_IFACE() {
		delete m_ruleResolver;
		delete m_debugDecorator;
	}


	void PNS_HORIZON_IFACE::SetRouter( PNS::ROUTER* aRouter )
	{
		m_router = aRouter;
	}
}
