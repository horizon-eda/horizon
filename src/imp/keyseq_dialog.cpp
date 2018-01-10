#include "keyseq_dialog.hpp"
#include <sstream>

namespace horizon {

	static void header_fun(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before) {
		if (before && !row->get_header())	{
			auto ret = Gtk::manage(new Gtk::Separator);
			row->set_header(*ret);
		}
	}

	KeySequenceDialog::KeySequenceDialog(Gtk::Window *parent) :
		Gtk::Dialog("Key Sequences", *parent, Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
	{
		lb = Gtk::manage(new Gtk::ListBox());
		lb->set_selection_mode(Gtk::SELECTION_NONE);
		lb->set_header_func(sigc::ptr_fun(&header_fun));

		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		sc->add(*lb);

		sg = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);

		get_content_area()->pack_start(*sc, true, true, 0);
		get_content_area()->set_border_width(0);
		get_content_area()->show_all();
		set_default_size(-1, 500);
	}

	void KeySequenceDialog::add_sequence(const std::string &seq, const std::string &label) {
		auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 20));
		box->set_margin_start(10);
		box->set_margin_end(10);
		box->set_margin_top(5);
		box->set_margin_bottom(5);
		auto l1 = Gtk::manage(new Gtk::Label());
		sg->add_widget(*l1);
		l1->set_xalign(0);
		l1->set_markup("<b>"+seq+"</b>");
		auto l2 = Gtk::manage(new Gtk::Label(label));
		l2->set_xalign(0);
		box->pack_start(*l1, false, false, 0);
		box->pack_start(*l2, true, true, 0);
		box->show_all();
		lb->append(*box);


	}

	void KeySequenceDialog::add_sequence(const std::vector<unsigned int > &seq, const std::string &label) {
		std::stringstream s;
		std::transform(seq.begin(), seq.end(), std::ostream_iterator<std::string>(s), [](const auto &x){return gdk_keyval_name(x);});
		//std::copy(part.begin(), part.tags.end(), std::ostream_iterator<std::string>(s, " "));
		add_sequence(s.str(), label);
	}
}
