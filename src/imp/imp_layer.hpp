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

    CanvasPreferences *get_canvas_preferences() override
    {
        return &preferences.canvas_layer;
    }

    ~ImpLayer()
    {
    }
};
} // namespace horizon
