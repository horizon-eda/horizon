#include "pool_notebook.hpp"
#include "widgets/pool_browser_parametric.hpp"
#include "widgets/part_preview.hpp"

namespace horizon {
void PoolNotebook::construct_parametric()
{
    for (const auto &it_tab : pool_parametric.get_tables()) {
        auto br = Gtk::manage(new PoolBrowserParametric(pool, pool_parametric, it_tab.first, "pool_notebook"));
        br->show();
        add_context_menu(br);
        br->signal_activated().connect([this, br] { go_to(ObjectType::PART, br->get_selected()); });
        auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));
        paned->add1(*br);
        paned->child_property_shrink(*br) = false;

        auto preview = Gtk::manage(new PartPreview(pool, true, "pool_notebook_parametric_" + it_tab.first));
        preview->signal_goto().connect(sigc::mem_fun(*this, &PoolNotebook::go_to));
        br->signal_selected().connect([this, br, preview] {
            auto sel = br->get_selected();
            if (!sel) {
                preview->load(nullptr);
                return;
            }
            auto part = pool.get_part(sel);
            preview->load(part);
        });
        paned->add2(*preview);
        paned->show_all();

        create_paned_state_store(paned, "parametric_" + it_tab.first);

        append_page(*paned, "Param: " + it_tab.second.display_name);
        install_search_once(paned, br);
        browsers_parametric.emplace(it_tab.first, br);
    }
}
} // namespace horizon
