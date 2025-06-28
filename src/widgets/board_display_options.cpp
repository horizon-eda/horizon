#include "board_display_options.hpp"
#include "common/layer_provider.hpp"
#include "board/board_layers.hpp"
#include <set>
#include <iostream>
#include <nlohmann/json.hpp>

namespace horizon {

class LayerOptionsExp : public Gtk::Expander {
    friend BoardDisplayOptionsBox;

public:
    LayerOptionsExp(BoardDisplayOptionsBox *p, int l)
        : Gtk::Expander(p->lp.get_layers().at(l).name), parent(p), layer(l)
    {
        box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2));
        add(*box);
        box->show();
        box->set_margin_top(2);
        box->set_margin_start(10);
        property_expanded().signal_changed().connect([this] { parent->update_expand_all(); });
    }

    int get_layer() const
    {
        return layer;
    }

    virtual json serialize() = 0;
    virtual void load_from_json(const json &j) = 0;

protected:
    Gtk::CheckButton *add_checkbutton(const std::string &la)
    {
        auto cb = Gtk::manage(new Gtk::CheckButton(la));
        cb->show();
        box->pack_start(*cb, false, false, 0);
        return cb;
    }

    void emit_layer_display(const LayerDisplay &ld)
    {
        parent->s_signal_set_layer_display.emit(layer, ld);
    }

    BoardDisplayOptionsBox *parent;
    const int layer;
    Gtk::Box *box = nullptr;
};

class LayerOptionsExpSilkscreen : public LayerOptionsExp {
    using LayerOptionsExp::LayerOptionsExp;

public:
    void construct()
    {
        cb_text = add_checkbutton("Text");
        cb_text->set_active(true);
        cb_gfx = add_checkbutton("Graphics");
        cb_gfx->set_active(true);

        cb_text->signal_toggled().connect(sigc::mem_fun(*this, &LayerOptionsExpSilkscreen::emit));
        cb_gfx->signal_toggled().connect(sigc::mem_fun(*this, &LayerOptionsExpSilkscreen::emit));
    }

    json serialize() override
    {
        json j;
        j["text"] = cb_text->get_active();
        j["gfx"] = cb_gfx->get_active();
        return j;
    }

    virtual void load_from_json(const json &j) override
    {
        if (j.count("text"))
            cb_text->set_active(j.at("text").get<bool>());
        if (j.count("gfx"))
            cb_gfx->set_active(j.at("gfx").get<bool>());
    }

private:
    Gtk::CheckButton *cb_gfx = nullptr;
    Gtk::CheckButton *cb_text = nullptr;
    void emit()
    {
        LayerDisplay ld;
        if (!cb_text->get_active()) {
            ld.types_visible &= ~(1 << static_cast<int>(TriangleInfo::Type::TEXT));
        }
        if (!cb_gfx->get_active()) {
            ld.types_visible &= ~(1 << static_cast<int>(TriangleInfo::Type::GRAPHICS));
        }
        emit_layer_display(ld);
    }
};
class LayerOptionsCopper : public LayerOptionsExp {
    using LayerOptionsExp::LayerOptionsExp;

public:
    void construct()
    {
        cb_planes_outline = add_checkbutton("Don't fill planes");
        cb_planes_outline->signal_toggled().connect(sigc::mem_fun(*this, &LayerOptionsCopper::emit));

        cb_keepouts_outline = add_checkbutton("Don't fill keepouts");
        cb_keepouts_outline->set_active(true);
        cb_keepouts_outline->signal_toggled().connect(sigc::mem_fun(*this, &LayerOptionsCopper::emit));
    }

    json serialize() override
    {
        json j;
        j["planes_outline"] = cb_planes_outline->get_active();
        j["keepouts_outline"] = cb_keepouts_outline->get_active();
        return j;
    }

    virtual void load_from_json(const json &j) override
    {
        cb_planes_outline->set_active(j.value("planes_outline", false));
        cb_keepouts_outline->set_active(j.value("keepouts_outline", true));
    }

private:
    Gtk::CheckButton *cb_planes_outline = nullptr;
    Gtk::CheckButton *cb_keepouts_outline = nullptr;
    void emit()
    {
        LayerDisplay ld;
        if (cb_planes_outline->get_active()) {
            ld.types_visible &= ~(1 << static_cast<int>(TriangleInfo::Type::PLANE_FILL));
        }
        if (!cb_keepouts_outline->get_active()) {
            ld.types_visible |= (1 << static_cast<int>(TriangleInfo::Type::KEEPOUT_FILL));
        }
        emit_layer_display(ld);
    }
};

BoardDisplayOptionsBox::BoardDisplayOptionsBox(LayerProvider &lpi) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 2), lp(lpi)
{
    expander_all = Gtk::manage(new Gtk::Expander("Expand all"));
    expander_all->get_style_context()->add_class("dim-label");
    expander_all->property_expanded().signal_changed().connect([this] {
        if (all_updating)
            return;
        expanding = true;
        for (auto child : get_children()) {
            if (auto e = dynamic_cast<LayerOptionsExp *>(child)) {
                e->set_expanded(expander_all->get_expanded());
            }
        }
        expanding = false;
        update_expand_all();
    });
    pack_start(*expander_all, false, false, 0);
    expander_all->set_margin_bottom(2);
    expander_all->show();

    set_margin_start(2);
    set_margin_end(2);
    set_margin_bottom(2);
    set_margin_top(2);

    {
        auto optns = Gtk::manage(new LayerOptionsExpSilkscreen(this, BoardLayers::TOP_SILKSCREEN));
        optns->construct();
        pack_start(*optns, false, false, 0);
        optns->show();
    }
    {
        auto optns = Gtk::manage(new LayerOptionsExpSilkscreen(this, BoardLayers::BOTTOM_SILKSCREEN));
        optns->construct();
        pack_start(*optns, false, false, 0);
        optns->show();
    }
    update();
}

void BoardDisplayOptionsBox::update()
{
    std::set<int> layers_from_lp;
    std::set<int> layers_from_box;
    for (const auto &it : lp.get_layers()) {
        layers_from_lp.emplace(it.first);
    }

    auto children = get_children();
    for (auto child : children) {
        if (auto e = dynamic_cast<LayerOptionsExp *>(child)) {
            if (layers_from_lp.count(e->layer) == 0) { // layer has been deleted
                delete child;
            }
            else {
                layers_from_box.insert(e->layer);
            }
        }
    }
    for (auto layer : layers_from_lp) {
        if (layers_from_box.count(layer) == 0 && lp.get_layers().at(layer).copper) { // need to add if copper
            auto optns = Gtk::manage(new LayerOptionsCopper(this, layer));
            optns->construct();
            pack_start(*optns, false, false, 0);
            optns->show();
        }
    }

    std::vector<LayerOptionsExp *> widgets;
    for (auto child : get_children()) {
        if (auto e = dynamic_cast<LayerOptionsExp *>(child)) {
            widgets.push_back(e);
        }
    }
    std::sort(widgets.begin(), widgets.end(), [](auto a, auto b) { return a->layer > b->layer; });

    int i = 1;
    for (auto e : widgets) {
        reorder_child(*e, i++);
    }
    update_expand_all();
}

json BoardDisplayOptionsBox::serialize()
{
    json j;
    for (auto child : get_children()) {
        if (auto e = dynamic_cast<LayerOptionsExp *>(child)) {
            j[std::to_string(e->get_layer())] = e->serialize();
        }
    }
    return j;
}

void BoardDisplayOptionsBox::load_from_json(const json &j)
{
    for (auto child : get_children()) {
        if (auto e = dynamic_cast<LayerOptionsExp *>(child)) {
            const std::string layer = std::to_string(e->get_layer());
            if (j.count(layer))
                e->load_from_json(j.at(layer));
        }
    }
}

void BoardDisplayOptionsBox::update_expand_all()
{
    if (expanding)
        return;
    bool all_expanded = true;
    for (auto child : get_children()) {
        if (auto e = dynamic_cast<LayerOptionsExp *>(child)) {
            if (e->get_expanded() == false)
                all_expanded = false;
        }
    }
    all_updating = true;
    if (all_expanded) {
        expander_all->set_expanded(true);
        expander_all->set_label("Collapse all");
    }
    else {
        expander_all->set_expanded(false);
        expander_all->set_label("Expand all");
    }
    all_updating = false;
}
} // namespace horizon
