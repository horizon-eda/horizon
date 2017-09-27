#pragma once
#include "router/pns_router.h"
#include "uuid.hpp"
#include "canvas/selectables.hpp"

namespace horizon {
	class Board;
	class BoardPackage;
	class Pad;
	class Track;
	class Via;
	class CanvasGL;
	class Junction;
	class Net;
	class BoardRules;
	template<typename T> class Coord;
}


namespace PNS {
	class PNS_HORIZON_IFACE : public PNS::ROUTER_IFACE {
		public:
			PNS_HORIZON_IFACE();
			~PNS_HORIZON_IFACE();

			void SetRouter( PNS::ROUTER* aRouter ) override;
			void SetBoard ( horizon::Board *brd);
			void SetCanvas (class horizon::CanvasGL *ca);
			void SetRules (horizon::BoardRules *rules);

			void SyncWorld( PNS::NODE* aWorld ) override;
			void EraseView() override;
			void HideItem( PNS::ITEM* aItem ) override;
			void DisplayItem( const PNS::ITEM* aItem, int aColor = 0, int aClearance = 0 ) override;
			void AddItem( PNS::ITEM* aItem ) override;
			void RemoveItem( PNS::ITEM* aItem ) override;
			void Commit() override;

			void UpdateNet( int aNetCode ) override;

			PNS::RULE_RESOLVER* GetRuleResolver() override;
			PNS::DEBUG_DECORATOR* GetDebugDecorator() override;

			void create_debug_decorator(horizon::CanvasGL *ca);

			static int layer_to_router(int l);
			static int layer_from_router(int l);
			horizon::Net *get_net_for_code(int code);
			horizon::SelectableRef get_ref_for_parent(uint32_t parent);


		private:
			class PNS_HORIZON_RULE_RESOLVER* m_ruleResolver = nullptr;
			class PNS_HORIZON_DEBUG_DECORATOR *m_debugDecorator = nullptr;
			std::set<horizon::SelectableRef> m_preview_items;

			horizon::Board *board = nullptr;
			class horizon::CanvasGL *canvas = nullptr;
			class horizon::BoardRules *rules = nullptr;
			PNS::NODE* m_world;
			PNS::ROUTER* m_router;

			std::unique_ptr<PNS::SOLID>   syncPad(const horizon::BoardPackage *pkg, const horizon::Pad *pad);
			std::unique_ptr<PNS::SEGMENT> syncTrack(const horizon::Track *track);
			std::unique_ptr<PNS::VIA>     syncVia(const horizon::Via *via);
			std::map<horizon::UUID, int> net_code_map;
			std::map<int, horizon::UUID> net_code_map_r;
			int net_code_max = 0;
			int get_net_code(const horizon::UUID &uu);



			std::map<uint32_t, horizon::SelectableRef> selectable_ref_map;
			std::map<horizon::SelectableRef, uint32_t> selectable_ref_map_r;
			uint32_t selectable_ref_max = 0;
			uint32_t get_ref_code(const horizon::SelectableRef &ref);

			std::pair<horizon::BoardPackage *, horizon::Pad *> find_pad(int layer, const horizon::Coord<int64_t> &c);
			horizon::Junction *find_junction(int layer, const horizon::Coord<int64_t> &c);

	};
}
