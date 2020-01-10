#include "about_dialog.hpp"
#include "util/version.hpp"

namespace horizon {

AboutDialog::AboutDialog() : Gtk::AboutDialog()
{
    std::string version = Version::get_string() + " " + Version::name;
    if (strlen(Version::commit)) {
        version += "\nCommit " + std::string(Version::commit);
    }
    set_version(version);
    set_program_name("Horizon EDA");
    std::vector<Glib::ustring> authors;
    authors.push_back("Lukas K. <lukas@horizon-eda.org>");
    set_authors(authors);
    set_license_type(Gtk::LICENSE_GPL_3_0);
    set_copyright("Copyright © 2017-2020 Lukas K., et al.");
    set_website("https://horizon-eda.org/");
    set_website_label("horizon-eda.org");
    set_comments("a free EDA package");

    set_logo_icon_name("horizon-eda");
}

} // namespace horizon
