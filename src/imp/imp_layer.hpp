#pragma once
#include "imp.hpp"

namespace horizon {
class ImpLayer : public ImpBase {
    friend class ImpInterface;

public:
    using ImpBase::ImpBase;

    bool is_layered() const override
    {
        return true;
    };

protected:
    void construct_layer_box(bool pack = true);
    class LayerBox *layer_box;
    Glib::RefPtr<Glib::Binding> work_layer_binding;
    Glib::RefPtr<Glib::Binding> layer_opacity_binding;
    void apply_preferences() override;
    void get_save_meta(json &j) override;
    virtual void load_default_layers();
    void add_view_angle_actions();

    const CanvasPreferences &get_canvas_preferences() override
    {
        return preferences.canvas_layer;
    }

    void handle_extra_button(const GdkEventButton *button_event) override;

    ~ImpLayer()
    {
    }

    std::vector<std::string> get_view_hints() override;

    int view_angle = 1;
    void set_view_angle(int angle);
    Gtk::Label *view_angle_label = nullptr;
    Gtk::Button *view_angle_button = nullptr;

    class ViewAngleWindow *view_angle_window = nullptr;
    sigc::connection view_angle_window_conn;
};
} // namespace horizon
