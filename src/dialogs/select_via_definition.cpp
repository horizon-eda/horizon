#include "select_via_definition.hpp"
#include "board/rule_via_definitions.hpp"
#include "common/layer_provider.hpp"
#include "pool/ipool.hpp"
#include "pool/padstack.hpp"
#include "parameter/set.hpp"
#include "util/geom_util.hpp"

namespace horizon {

void SelectViaDefinitionDialog::ok_clicked()
{
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        valid = true;
        selected_uuid = row.get_value(list_columns.via_def)->uuid;
    }
}

void SelectViaDefinitionDialog::row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        valid = true;
        selected_uuid = row.get_value(list_columns.via_def)->uuid;
        response(Gtk::ResponseType::RESPONSE_OK);
    }
}

SelectViaDefinitionDialog::SelectViaDefinitionDialog(Gtk::Window *parent, const RuleViaDefinitions &rule,
                                                     const class LayerProvider &lprv, IPool &apool)
    : Gtk::Dialog("Select via definition", *parent,
                  Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
      layer_provider(lprv), pool(apool)
{
    Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(300, 300);

    button_ok->signal_clicked().connect(sigc::mem_fun(*this, &SelectViaDefinitionDialog::ok_clicked));

    store = Gtk::ListStore::create(list_columns);
    Gtk::TreeModel::Row row;
    std::set<ParameterID> params;
    for (const auto &[uu, def] : rule.via_definitions) {
        row = *(store->append());
        row[list_columns.name] = def.name;
        row[list_columns.via_def] = &def;
        for (const auto &[param, value] : def.parameters) {
            params.insert(param);
        }
    }

    view = Gtk::manage(new Gtk::TreeView(store));
    view->append_column("Via def.", list_columns.name);
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Span", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            const auto &def = *row.get_value(list_columns.via_def);
            mcr->property_text() = layer_provider.get_layer_name(def.span);
        });
        view->append_column(*tvc);
    }
    {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Padstack", *cr));
        tvc->set_cell_data_func(*cr, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            const auto &def = *row.get_value(list_columns.via_def);
            mcr->property_text() = pool.get_padstack(def.padstack)->name;
        });
        view->append_column(*tvc);
    }
    for (const auto param : params) {
        auto cr = Gtk::manage(new Gtk::CellRendererText());
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn(parameter_id_to_name(param), *cr));
        tvc->set_cell_data_func(*cr, [this, param](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            const auto &def = *row.get_value(list_columns.via_def);
            if (def.parameters.count(param))
                mcr->property_text() = dim_to_string(def.parameters.at(param));
            else
                mcr->property_text() = "";
        });
        view->append_column(*tvc);
    }
    view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
    view->signal_row_activated().connect(sigc::mem_fun(*this, &SelectViaDefinitionDialog::row_activated));


    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->add(*view);
    get_content_area()->pack_start(*sc, true, true, 0);
    get_content_area()->set_border_width(0);

    show_all();
}
} // namespace horizon
