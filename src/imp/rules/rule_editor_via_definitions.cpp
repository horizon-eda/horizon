#include "rule_editor_via_definitions.hpp"
#include "board/rule_via_definitions.hpp"
#include "document/idocument.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include "util/changeable.hpp"
#include "util/gtk_util.hpp"
#include "util/once.hpp"
#include "widgets/layer_range_editor.hpp"
#include "widgets/pool_browser_button.hpp"
#include "widgets/pool_browser_padstack.hpp"
#include "pool/ipool.hpp"
#include "pool/pool_info.hpp"
#include "widgets/parameter_set_editor.hpp"
#include <iostream>

namespace horizon {

static Gdk::RGBA get_copper_rgba()
{
    Gdk::RGBA rgba;
    rgba.set_rgba(1, .8, 0);
    return rgba;
}

static Gdk::RGBA get_substrate_rgba()
{
    Gdk::RGBA rgba("#0E7948");
    return rgba;
}

class ViaDefinitionEditor : public Gtk::Grid, public Changeable {
public:
    ViaDefinitionEditor(ViaDefinition &adef, LayerProvider &lprv, IPool &pool) : def(adef)
    {
        set_row_spacing(10);
        set_column_spacing(10);
        set_hexpand_set(true);
        sg_label = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_VERTICAL);

        int top = 0;
        {
            auto name_entry = Gtk::manage(new Gtk::Entry);
            name_entry->set_hexpand(true);
            sg_label->add_widget(*name_entry);
            bind_widget(name_entry, def.name, [this](const auto &x) { s_signal_changed.emit(); });
            grid_attach_label_and_widget(this, "Name", name_entry, top);
        }
        {
            ps_button = Gtk::manage(new PoolBrowserButton(ObjectType::PADSTACK, pool));
            {
                auto &br = dynamic_cast<PoolBrowserPadstack &>(ps_button->get_browser());
                br.set_padstacks_included({Padstack::Type::VIA});
            }
            ps_button->property_selected_uuid().set_value(def.padstack);
            ps_button->property_selected_uuid().signal_changed().connect([this] {
                def.padstack = ps_button->property_selected_uuid().get_value();
                s_signal_changed.emit();
            });

            grid_attach_label_and_widget(this, "Padstack", ps_button, top);
        }
        {
            span_editor = Gtk::manage(new LayerRangeEditor);
            span_editor->set_orientation(Gtk::ORIENTATION_VERTICAL);
            for (auto [li, layer] : lprv.get_layers()) {
                if (layer.copper)
                    span_editor->add_layer(layer);
            }
            span_editor->set_layer_range(adef.span);
            span_editor->signal_changed().connect([this] {
                def.span = span_editor->get_layer_range();
                s_signal_changed.emit();
            });
            auto la = grid_attach_label_and_widget(this, "Span", span_editor, top);
            sg_label->add_widget(*la);
            la->set_valign(Gtk::ALIGN_START);
        }
        {
            auto pse = Gtk::manage(new ParameterSetEditor(&def.parameters, true, ParameterSetEditor::Style::BORDER));
            pse->set_vexpand(true);
            attach(*pse, 0, top, 2, 1);
            pse->show();
            pse->signal_changed().connect([this] { s_signal_changed.emit(); });
        }
    }

private:
    ViaDefinition &def;
    LayerRangeEditor *span_editor = nullptr;
    PoolBrowserButton *ps_button = nullptr;

    Glib::RefPtr<Gtk::SizeGroup> sg_label;
};

class ViaDefinitionLayerWidget : public Gtk::Widget {
public:
    enum class Style { EMPTY, COPPER, SUBSTRATE, BARREL };
    ViaDefinitionLayerWidget(Style astyle) : style(astyle)
    {
        set_has_window(false);
        get_style_context()->add_class("via-def-layer");
    }


    bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override
    {
        cr->save();
        const auto width = get_allocated_width();
        const auto height = get_allocated_height();
        auto style_ctx = get_style_context();
        auto padding = style_ctx->get_padding(style_ctx->get_state());
        auto border = style_ctx->get_border(style_ctx->get_state());
        style_ctx->get_border();

        auto draw_height = height - padding.get_top() - padding.get_bottom() - border.get_top() - border.get_bottom();

        auto draw_background_rectangle = [&] {
            cr->rectangle(padding.get_left(), padding.get_top(), width - padding.get_left() - padding.get_right(),
                          draw_height);
            cr->fill();
        };

        switch (style) {
        case Style::COPPER:
            Gdk::Cairo::set_source_rgba(cr, get_copper_rgba());
            draw_background_rectangle();
            break;

        case Style::SUBSTRATE:
            Gdk::Cairo::set_source_rgba(cr, get_substrate_rgba());
            draw_background_rectangle();
            break;

        case Style::EMPTY:
            // nop
            break;

        case Style::BARREL: {
            Gdk::Cairo::set_source_rgba(cr, get_substrate_rgba());
            draw_background_rectangle();

            Gdk::Cairo::set_source_rgba(cr, get_copper_rgba());
            const int barrel_width = 40;
            cr->rectangle(width / 2 - barrel_width / 2, padding.get_top(), barrel_width, draw_height);
            cr->fill();
        } break;
        }


        cr->restore();
        style_ctx->render_frame(cr, 0, 0, width, height);
        return false;
    }

private:
    const Style style;
};


class ViaDefinitionButton : public Gtk::RadioButton {
public:
    ViaDefinitionButton(const ViaDefinition &adef) : def(adef)
    {
        set_mode(false);
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
        add(*box);
        box->show();
        label = Gtk::manage(new Gtk::Label);
        label->get_style_context()->add_class("via-def-name");
        label->set_width_chars(10);
        box->pack_start(*label, false, false, 0);
        label->show();

        layer_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
        layer_box->get_style_context()->add_class("via-def-layer-box");
        layer_box->show();
        box->pack_start(*layer_box, true, true, 0);
    }

    void update(unsigned int n_inner_layers)
    {
        label->set_markup("<b>" + Glib::Markup::escape_text(def.name) + "</b>");
        {
            auto children = layer_box->get_children();
            for (auto ch : children) {
                delete ch;
            }
        }

        add_layer_for_def(BoardLayers::TOP_COPPER);

        for (int i = 0; i < (int)n_inner_layers; i++) {
            add_layer_for_def(-1 - i);
        }

        add_layer_for_def(BoardLayers::BOTTOM_COPPER);
    }

    const UUID &get_uuid() const
    {
        return def.uuid;
    }

private:
    Gtk::Label *label = nullptr;
    Gtk::Box *layer_box = nullptr;

    void add_layer_for_def(int layer)
    {
        if (def.span.overlaps(layer))
            add_layer(ViaDefinitionLayerWidget::Style::COPPER);
        else
            add_layer(ViaDefinitionLayerWidget::Style::EMPTY);

        if (layer == BoardLayers::BOTTOM_COPPER)
            return; // has no substrate

        if (def.span.overlaps(layer) && def.span.overlaps(layer - 1))
            add_layer(ViaDefinitionLayerWidget::Style::BARREL);
        else
            add_layer(ViaDefinitionLayerWidget::Style::SUBSTRATE);
    }


    void add_layer(ViaDefinitionLayerWidget::Style style)
    {
        auto box = Gtk::manage(new ViaDefinitionLayerWidget(style));
        layer_box->pack_start(*box, true, true, 0);
        box->show();
    }

    const ViaDefinition &def;
};

ViaDefinitionButton &RuleEditorViaDefinitions::add_via_definition_button(const ViaDefinition &def)
{
    auto button = Gtk::manage(new ViaDefinitionButton(def));
    button->join_group(via_definitions_rb_group);
    via_definitions_box->pack_start(*button, true, true, 0);
    button->show();
    return *button;
}


void RuleEditorViaDefinitions::update_definition_editor(class ViaDefinition &def)
{
    if (defintion_editor) {
        delete defintion_editor;
        defintion_editor = nullptr;
    }
    defintion_editor = Gtk::manage(new ViaDefinitionEditor(def, core.get_layer_provider(), core.get_pool()));
    defintion_editor->signal_changed().connect([this] {
        for (auto ch : via_definitions_box->get_children()) {
            const auto &brd = *dynamic_cast<IDocumentBoard &>(core).get_board();
            if (auto bu = dynamic_cast<ViaDefinitionButton *>(ch)) {
                bu->update(brd.get_n_inner_layers());
            }
        }
        s_signal_updated.emit();
    });
    defintion_editor->show();
    via_definition_box->pack_start(*defintion_editor, true, true, 0);
}

void RuleEditorViaDefinitions::populate()
{
    rule2 = &dynamic_cast<RuleViaDefinitions &>(rule);

    builder = Gtk::Builder::create_from_resource(
            "/org/horizon-eda/horizon/imp/rules/"
            "rule_editor_via_definitions.ui");
    Gtk::Grid *editor;
    builder->get_widget("editor", editor);
    pack_start(*editor, true, true, 0);

    builder->get_widget("via_definitions_box", via_definitions_box);
    builder->get_widget("via_definition_box", via_definition_box);
    const auto &brd = *dynamic_cast<IDocumentBoard &>(core).get_board();

    {
        Gtk::Box *via_definitions_header_box;
        builder->get_widget("via_definitions_header_box", via_definitions_header_box);
        auto add_layer = [via_definitions_header_box, &brd](int layer, bool substrate) {
            std::string name = brd.get_layers().at(layer).name;
            if (substrate)
                name += " (substrate)";
            auto la = Gtk::manage(new Gtk::Label(name));
            la->set_xalign(1);
            la->show();
            via_definitions_header_box->pack_start(*la, false, false, 0);
        };
        add_layer(BoardLayers::TOP_COPPER, false);
        add_layer(BoardLayers::TOP_COPPER, true);
        for (int i = 0; i < (int)brd.get_n_inner_layers(); i++) {
            add_layer(-1 - i, false);
            add_layer(-1 - i, true);
        }
        add_layer(BoardLayers::BOTTOM_COPPER, false);
    }

    Once once;
    for (auto &[uu, it] : rule2->via_definitions) {
        auto &bu = add_via_definition_button(it);
        bu.update(brd.get_n_inner_layers());
        auto &def = it;
        bu.signal_toggled().connect([this, &bu, &def] {
            if (bu.get_active()) {
                update_definition_editor(def);
            }
        });
        if (once()) { // show first editor
            bu.set_active(true);
        }
    }

    {
        Gtk::Button *add_button = nullptr;
        builder->get_widget("add_button", add_button);
        add_button->signal_clicked().connect([this, &brd] {
            auto uu = UUID::random();
            auto &vdef = rule2->via_definitions.emplace(uu, uu).first->second;
            vdef.name = "new via";
            vdef.span = BoardLayers::layer_range_through;
            auto &bu = add_via_definition_button(vdef);
            bu.update(brd.get_n_inner_layers());
            bu.signal_toggled().connect([this, &bu, &vdef] {
                if (bu.get_active()) {
                    update_definition_editor(vdef);
                }
            });
            bu.set_active(true);
            update_definition_editor(vdef);
        });
    }
    {
        Gtk::Button *remove_button = nullptr;
        builder->get_widget("remove_button", remove_button);
        remove_button->signal_clicked().connect([this] {
            size_t idx = 0;
            for (auto ch : via_definitions_box->get_children()) {
                if (auto bu = dynamic_cast<ViaDefinitionButton *>(ch)) {
                    if (bu->get_active()) {
                        delete defintion_editor;
                        defintion_editor = nullptr;
                        UUID uu = bu->get_uuid();
                        delete bu;
                        rule2->via_definitions.erase(uu);
                        break;
                    }
                    idx++;
                }
            }
            {
                auto children = via_definitions_box->get_children();
                if (children.size()) {
                    auto ch = children.at(std::min(idx, children.size() - 1));
                    if (auto bu = dynamic_cast<ViaDefinitionButton *>(ch)) {
                        bu->set_active(true);
                    }
                }
            }
        });
    }

    editor->show();
}
} // namespace horizon
