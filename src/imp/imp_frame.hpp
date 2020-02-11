#pragma once
#include "imp.hpp"
#include "core/core_frame.hpp"

namespace horizon {
class ImpFrame : public ImpBase {
public:
    ImpFrame(const std::string &frame_filename, const std::string &pool_path);

protected:
    void construct() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const override
    {
        return ActionCatalogItem::AVAILABLE_IN_FRAME;
    };
    ObjectType get_editor_type() const override
    {
        return ObjectType::FRAME;
    }

private:
    void canvas_update() override;
    CoreFrame core_frame;

    class HeaderButton *header_button = nullptr;
    Gtk::Entry *name_entry = nullptr;

    void update_header();
};
} // namespace horizon
