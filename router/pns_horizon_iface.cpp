#include "pns_horizon_iface.h"
#include "board.hpp"
#include "router/pns_topology.h"
#include "router/pns_via.h"
#include "canvas/canvas.hpp"
#include "clipper/clipper.hpp"
#include "router/pns_solid.h"
#include "router/pns_debug_decorator.h"
#include "geometry/shape_convex.h"
#include "board_layers.hpp"
#include "board/via_padstack_provider.hpp"

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
			case F_Cu: lo=horizon::BoardLayers::TOP_COPPER; break;
			case B_Cu: lo=horizon::BoardLayers::BOTTOM_COPPER; break;
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

	int PNS_HORIZON_RULE_RESOLVER::Clearance( const PNS::ITEM* aA, const PNS::ITEM* aB ) const {
		auto net_a = m_iface->get_net_for_code(aA->Net());
		auto net_b = m_iface->get_net_for_code(aB->Net());

		auto pt_a = patch_type_from_kind(aA->Kind());
		auto pt_b = patch_type_from_kind(aB->Kind());

		auto layers_a = aA->Layers();
		auto layers_b = aB->Layers();

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
		/*wxString refName = m_board->FindNet( aNet )->GetNetname();
		wxString dummy, coupledNetName;

		if( matchDpSuffix( refName, coupledNetName, dummy ) )
		{
			NETINFO_ITEM* net = m_board->FindNet( coupledNetName );

			if( !net )
				return -1;

			return net->GetNet();
		}*/

		return -1;
	}


	int PNS_HORIZON_RULE_RESOLVER::DpNetPolarity( int aNet )
	{
	   return 0;
	}


	bool PNS_HORIZON_RULE_RESOLVER::DpNetPair( PNS::ITEM* aItem, int& aNetP, int& aNetN )
	{
		return true;
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
		std::set<horizon::SelectableRef> lines;
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

	horizon::SelectableRef PNS_HORIZON_IFACE::get_ref_for_parent(uint32_t parent) {
		if(selectable_ref_map.count(parent))
			return selectable_ref_map.at(parent);
		return horizon::SelectableRef(horizon::UUID(), horizon::ObjectType::INVALID);
	}

	uint32_t PNS_HORIZON_IFACE::get_ref_code(const horizon::SelectableRef &ref) {
		if(selectable_ref_map_r.count(ref)) {
			return selectable_ref_map_r.at(ref);
		}
		else {
			selectable_ref_max++;
			selectable_ref_map.emplace(selectable_ref_max, ref);
			selectable_ref_map_r.emplace(ref, selectable_ref_max);
			return selectable_ref_max;
		}
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
		segment->SetParent( get_ref_code(horizon::SelectableRef(track->uuid, horizon::ObjectType::TRACK)) );

		//if( aTrack->IsLocked() )
			//segment->Mark( PNS::MK_LOCKED );

		return segment;
	}

	std::unique_ptr<PNS::SOLID>   PNS_HORIZON_IFACE::syncPad(const horizon::BoardPackage *pkg, const horizon::Pad *pad) {
		horizon::Placement tr(pkg->placement);
		if(pkg->flip) {
			tr.invert_angle();
		}
		tr.accumulate(pad->placement);
		int layer_min = 10000;
		int layer_max = -10000;

		ClipperLib::Clipper clipper;

		for(auto &it: pad->padstack.shapes) {
			if(horizon::BoardLayers::is_copper(it.second.layer)) { //on copper layer
				layer_min = std::min(layer_min, it.second.layer);
				layer_max = std::max(layer_max, it.second.layer);
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
		assert(poly_union.size()==1);

		std::unique_ptr< PNS::SOLID > solid( new PNS::SOLID );

		solid->SetLayers( LAYER_RANGE( layer_to_router(layer_max), layer_to_router(layer_min) ));
		if(pad->net)
			solid->SetNet( get_net_code(pad->net->uuid) );
		//solid->SetParent( aPad );

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
		std::unique_ptr<PNS::VIA> pvia( new PNS::VIA(
            VECTOR2I(pos.x, pos.y),
            LAYER_RANGE( layer_to_router(horizon::BoardLayers::TOP_COPPER),  layer_to_router(horizon::BoardLayers::BOTTOM_COPPER)),
            via->parameter_set.at(horizon::ParameterID::VIA_DIAMETER),
            via->parameter_set.at(horizon::ParameterID::HOLE_DIAMETER),
            get_net_code(via->junction->net->uuid),
            VIA_THROUGH )
		);

		//via->SetParent( aVia );
		pvia->SetParent( get_ref_code(horizon::SelectableRef(via->uuid, horizon::ObjectType::VIA)) );

		//if( aVia->IsLocked() )
        //via->Mark( PNS::MK_LOCKED );

		return pvia;
	}

	void PNS_HORIZON_IFACE::SyncWorld( PNS::NODE *aWorld ) {
		std::cout << "!!!sync world" << std::endl;
		if(!board) {
			wxLogTrace( "PNS", "No board attached, aborting sync." );
			return;
		}

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

		for(const auto &it: board->packages) {
			for(auto &it_pad: it.second.package.pads) {
				auto pad = syncPad( &it.second, &it_pad.second );
				if( pad ) {
					aWorld->Add( std::move( pad ) );
				}
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
			auto ref = selectable_ref_map.at(parent);
			canvas->hide_obj(ref);
		}
	}


	void PNS_HORIZON_IFACE::RemoveItem( PNS::ITEM* aItem ) {

		auto parent = aItem->Parent();
		std::cout << "!!!iface remove item " << parent << " " << aItem->KindStr()  << std::endl;
		if(parent) {
			auto ref = selectable_ref_map.at(parent);
			switch(ref.type) {
				case horizon::ObjectType::TRACK :
					board->tracks.erase(ref.uuid);
				break;
				default:;
			}
		}
		board->expand(true);
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
				aItem->SetParent( get_ref_code(horizon::SelectableRef(track->uuid, horizon::ObjectType::TRACK)) );

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
				}
			} break;

			default:
				std::cout << "!!!unhandled add " << aItem->KindStr() << std::endl;
		}
		board->expand(true);
	}


	void PNS_HORIZON_IFACE::Commit() {
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
