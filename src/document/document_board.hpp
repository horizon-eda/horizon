#pragma once
#include "document.hpp"
#include "idocument_board.hpp"
#include "common/junction.hpp"
#include "common/line.hpp"
#include "common/arc.hpp"
#include "common/text.hpp"
#include "common/hole.hpp"
#include "common/dimension.hpp"
#include "common/keepout.hpp"
#include "common/picture.hpp"

namespace horizon {
class DocumentBoard : public virtual Document, public virtual IDocumentBoard {
public:
    bool has_object_type(ObjectType type) const;
    std::string get_display_name(ObjectType type, const UUID &uu) override;

protected:
    std::map<UUID, Polygon> *get_polygon_map() override;
    std::map<UUID, Junction> *get_junction_map() override;
    std::map<UUID, Text> *get_text_map() override;
    std::map<UUID, Line> *get_line_map() override;
    std::map<UUID, Dimension> *get_dimension_map() override;
    std::map<UUID, Arc> *get_arc_map() override;
    std::map<UUID, Keepout> *get_keepout_map() override;
    std::map<UUID, Picture> *get_picture_map() override;
};
} // namespace horizon
