#pragma once
#include "imp.hpp"
#include "core/core_frame.hpp"

namespace horizon {
class ImpFrame : public ImpBase {
public:
    ImpFrame(const std::string &frame_filename, const std::string &pool_path, TempMode temp_mode);

protected:
    void construct() override;

    ActionCatalogItem::Availability get_editor_type_for_action() const override
    {
        return ActionCatalogItem::AVAILABLE_IN_FRAME;
    };

private:
    void canvas_update() override;
    CoreFrame core_frame;
    Frame &frame;

    class HeaderButton *header_button = nullptr;
    Gtk::Entry *name_entry = nullptr;

    void update_header();

    bool set_filename() override;
};
} // namespace horizon
