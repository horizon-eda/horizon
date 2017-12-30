#include "canvas/canvas.hpp"
#include "pool_notebook.hpp"
#include "widgets/pool_browser_unit.hpp"
#include "widgets/pool_browser_symbol.hpp"
#include "widgets/pool_browser_entity.hpp"
#include "widgets/pool_browser_padstack.hpp"
#include "widgets/pool_browser_package.hpp"
#include "widgets/pool_browser_part.hpp"
#include "dialogs/pool_browser_dialog.hpp"
#include "util/util.hpp"
#include "pool-update/pool-update.hpp"
#include <zmq.hpp>
#include "editor_window.hpp"
#include "pool-mgr-app_win.hpp"
#include "duplicate/duplicate_window.hpp"
#include "duplicate/duplicate_unit.hpp"
#include "duplicate/duplicate_part.hpp"
#include "object_descr.hpp"
#include "pool_remote_box.hpp"
#include "pool_merge_dialog.hpp"
#include <git2.h>
#include "util/autofree_ptr.hpp"
#include <thread>

namespace horizon {

	PoolManagerProcess::PoolManagerProcess(PoolManagerProcess::Type ty,  const std::vector<std::string>& args, const std::vector<std::string>& ienv, Pool *pool): type(ty) {
		std::cout << "create proc" << std::endl;
		if(type == PoolManagerProcess::Type::IMP_SYMBOL || type == PoolManagerProcess::Type::IMP_PADSTACK || type == PoolManagerProcess::Type::IMP_PACKAGE) { //imp
			std::vector<std::string> argv;
			std::vector<std::string> env = ienv;
			auto envs = Glib::listenv();
			for(const auto &it: envs) {
				env.push_back(it+"="+Glib::getenv(it));
			}
			auto exe_dir = get_exe_dir();
			auto imp_exe = Glib::build_filename(exe_dir, "horizon-imp");
			argv.push_back(imp_exe);
			switch(type) {
				case PoolManagerProcess::Type::IMP_SYMBOL :
					argv.push_back("-y");
					argv.insert(argv.end(), args.begin(), args.end());
				break;
				case PoolManagerProcess::Type::IMP_PADSTACK :
					argv.push_back("-a");
					argv.insert(argv.end(), args.begin(), args.end());
				break;
				case PoolManagerProcess::Type::IMP_PACKAGE :
					argv.push_back("-k");
					argv.insert(argv.end(), args.begin(), args.end());
				break;
				default:;
			}
			proc = std::make_unique<EditorProcess>(argv, env);
			proc->signal_exited().connect([this](auto rc){s_signal_exited.emit(rc, true);});
		}
		else {
			switch(type) {
				case PoolManagerProcess::Type::UNIT :
					win = new EditorWindow(ObjectType::UNIT, args.at(0), pool);
				break;
				case PoolManagerProcess::Type::ENTITY :
					win = new EditorWindow(ObjectType::ENTITY, args.at(0), pool);
				break;
				case PoolManagerProcess::Type::PART :
					win = new EditorWindow(ObjectType::PART, args.at(0), pool);
				break;
				default:;
			}
			win->present();

			win->signal_hide().connect([this] {
				auto need_update = win->get_need_update();
				delete win;
				s_signal_exited.emit(0, need_update);
			});
		}
	}

	void PoolManagerProcess::reload() {
		if(auto w = dynamic_cast<EditorWindow*>(win)) {
			w->reload();
		}
	}

	void PoolNotebook::spawn(PoolManagerProcess::Type type, const std::vector<std::string> &args) {
		if(processes.count(args.at(0)) == 0) { //need to launch imp
			std::vector<std::string> env = {"HORIZON_POOL="+base_path};
			std::string filename = args.at(0);
			if(filename.size()) {
				if(!Glib::file_test(filename, Glib::FILE_TEST_IS_REGULAR)) {
					auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
					Gtk::MessageDialog md(*top,  "File not found", false /* use_markup */, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
					md.set_secondary_text("Try updating the pool");
					md.run();
					return;
				}
			}
			auto &proc = processes.emplace(std::piecewise_construct, std::forward_as_tuple(filename),
					std::forward_as_tuple(type, args, env, &pool)).first->second;

			proc.signal_exited().connect([filename, this](int status, bool need_update) {
				std::cout << "exit stat " << status << std::endl;
				/*if(status != 0) {
					view_project.info_bar_label->set_text("Editor for '"+filename+"' exited with status "+std::to_string(status));
					view_project.info_bar->show();

					//ugly workaround for making the info bar appear
					auto parent = dynamic_cast<Gtk::Box*>(view_project.info_bar->get_parent());
					parent->child_property_padding(*view_project.info_bar) = 1;
					parent->child_property_padding(*view_project.info_bar) = 0;
				}*/
				processes.erase(filename);
				if(need_update)
					pool_update();
			});
		}
		else { //present imp
			//auto &proc = processes.at(args.at(0));
			//auto pid = proc.proc->get_pid();
			//app->send_json(pid, {{"op", "present"}});
		}
	}

	void PoolNotebook::pool_updated(bool success) {
		pool_updating = false;
		appwin->set_pool_updating(false, success);
		pool.clear();
		for(auto &br: browsers) {
			br->search();
		}
		for(auto &it: processes) {
			it.second.reload();
		}
		if(success && pool_update_done_cb) {
			pool_update_done_cb();
			pool_update_done_cb = nullptr;
		}
	}

	PoolNotebook::~PoolNotebook() {
		sock_pool_update_conn.disconnect();
	}

	typedef struct {
		PoolUpdateStatus status;
		char msg[1];
	} pool_update_msg_t;

	Gtk::Button *PoolNotebook::add_action_button(const std::string &label, Gtk::Box *bbox, sigc::slot0<void> cb) {
		auto bu = Gtk::manage(new Gtk::Button(label));
		bbox->pack_start(*bu, false, false,0);
		bu->signal_clicked().connect(cb);
		return bu;
	}

	Gtk::Button *PoolNotebook::add_action_button(const std::string &label, Gtk::Box *bbox, class PoolBrowser *br, sigc::slot1<void, UUID> cb) {
		auto bu = Gtk::manage(new Gtk::Button(label));
		bbox->pack_start(*bu, false, false,0);
		bu->signal_clicked().connect([br, cb] {
			cb(br->get_selected());
		});
		br->signal_selected().connect([bu, br] {
			bu->set_sensitive(br->get_selected());
		});
		bu->set_sensitive(br->get_selected());
		return bu;
	}

	PoolNotebook::PoolNotebook(const std::string &bp, class PoolManagerAppWindow *aw): Gtk::Notebook(), base_path(bp), pool(bp), appwin(aw), zctx(aw->zctx), sock_pool_update(zctx, ZMQ_SUB) {
		sock_pool_update_ep = "inproc://pool-update-"+((std::string)UUID::random());
		sock_pool_update.bind(sock_pool_update_ep);
		{
			int dummy = 0;
			sock_pool_update.setsockopt(ZMQ_SUBSCRIBE, &dummy, 0);
			Glib::RefPtr<Glib::IOChannel> chan;
			#ifdef G_OS_WIN32
				SOCKET fd = sock_pool_update.getsockopt<SOCKET>(ZMQ_FD);
				chan = Glib::IOChannel::create_from_win32_socket(fd);
			#else
				int fd = sock_pool_update.getsockopt<int>(ZMQ_FD);
				chan = Glib::IOChannel::create_from_fd(fd);
			#endif

			sock_pool_update_conn = Glib::signal_io().connect([this](Glib::IOCondition cond){
				while(sock_pool_update.getsockopt<int>(ZMQ_EVENTS) & ZMQ_POLLIN) {
					zmq::message_t zmsg;
					sock_pool_update.recv(&zmsg);
					auto msg = reinterpret_cast<const pool_update_msg_t *>(zmsg.data());
					std::cout << "pool sock rx" << std::endl;
					appwin->set_pool_update_status_text(msg->msg);
					if(msg->status == PoolUpdateStatus::DONE) {
						pool_updated(true);
						pool_update_n_files_last = pool_update_n_files;
					}
					else if(msg->status == PoolUpdateStatus::FILE) {
						pool_update_last_file = msg->msg;
						pool_update_n_files++;
						if(pool_update_n_files_last) {
							appwin->set_pool_update_progress((float)pool_update_n_files/pool_update_n_files_last);
						}
						else {
							appwin->set_pool_update_progress(-1);
						}
					}
					else if(msg->status == PoolUpdateStatus::ERROR) {
						appwin->set_pool_update_status_text(std::string(msg->msg) + " Last file: "+pool_update_last_file);
						pool_updated(false);
					}
				}
				return true;
			}, chan, Glib::IO_IN | Glib::IO_HUP);
		}
		remote_repo = Glib::build_filename(base_path, ".remote");
		if(!Glib::file_test(remote_repo, Glib::FILE_TEST_IS_DIR)) {
			remote_repo = "";
		}

		{
			auto br = Gtk::manage(new PoolBrowserUnit(&pool));
			br->set_show_path(true);
			br->signal_activated().connect([this, br] {
				auto uu = br->get_selected();
				auto path = pool.get_filename(ObjectType::UNIT, uu);
				spawn(PoolManagerProcess::Type::UNIT, {path});
			});

			br->show();
			browsers.insert(br);

			auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
			auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
			bbox->set_margin_bottom(8);
			bbox->set_margin_top(8);
			bbox->set_margin_start(8);
			bbox->set_margin_end(8);

			add_action_button("Create Unit", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_unit));
			add_action_button("Edit Unit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_unit));
			add_action_button("Duplicate Unit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_unit));
			add_action_button("Create Symbol for Unit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_create_symbol_for_unit));
			add_action_button("Create Entity for Unit", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_create_entity_for_unit));
			if(remote_repo.size())
				add_action_button("Merge Unit", bbox, br, [this](const UUID &uu){remote_box->merge_item(ObjectType::UNIT, uu);});

			bbox->show_all();

			box->pack_start(*bbox, false, false, 0);

			auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
			sep->show();
			box->pack_start(*sep, false, false, 0);
			box->pack_start(*br, true, true, 0);
			box->show();

			append_page(*box, "Units");
		}
		{
			auto br = Gtk::manage(new PoolBrowserSymbol(&pool));
			br->set_show_path(true);
			browsers.insert(br);
			br->show();
			auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));
			paned->add1(*br);
			auto canvas = Gtk::manage(new CanvasGL());
			canvas->set_selection_allowed(false);
			paned->add2(*canvas);
			paned->show_all();
			br->signal_selected().connect([this, br, canvas]{
				auto sel = br->get_selected();
				std::cout << "sym sel " << (std::string) sel << std::endl;
				if(!sel) {
					canvas->clear();
					return;
				}
				Symbol sym = *pool.get_symbol(sel);
				for(const auto &la: sym.get_layers()) {
					canvas->set_layer_display(la.first, LayerDisplay(true, LayerDisplay::Mode::FILL, la.second.color));
				}
				sym.expand();
				canvas->update(sym);
				auto bb = sym.get_bbox();
				int64_t pad = 1_mm;
				bb.first.x -= pad;
				bb.first.y -= pad;

				bb.second.x += pad;
				bb.second.y += pad;
				canvas->zoom_to_bbox(bb.first, bb.second);
			});
			br->signal_activated().connect([this, br] {
				auto uu = br->get_selected();
				auto path = pool.get_filename(ObjectType::SYMBOL, uu);
				spawn(PoolManagerProcess::Type::IMP_SYMBOL, {path});
			});

			auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
			auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
			bbox->set_margin_bottom(8);
			bbox->set_margin_top(8);
			bbox->set_margin_start(8);
			bbox->set_margin_end(8);

			add_action_button("Create Symbol", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_symbol));
			add_action_button("Edit Symbol", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_symbol));
			add_action_button("Duplicate Symbol", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_symbol));
			if(remote_repo.size())
				add_action_button("Merge Symbol", bbox, br, [this](const UUID &uu){remote_box->merge_item(ObjectType::SYMBOL, uu);});
			bbox->show_all();

			box->pack_start(*bbox, false, false, 0);

			auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
			sep->show();
			box->pack_start(*sep, false, false, 0);
			box->pack_start(*paned, true, true, 0);
			box->show();

			append_page(*box, "Symbols");
		}


		{
			auto br = Gtk::manage(new PoolBrowserEntity(&pool));
			br->set_show_path(true);
			br->signal_activated().connect([this, br] {
				auto uu = br->get_selected();
				if(!uu)
						return;
				auto path = pool.get_filename(ObjectType::ENTITY, uu);
				spawn(PoolManagerProcess::Type::ENTITY, {path});
			});

			br->show();
			browsers.insert(br);

			auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
			auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
			bbox->set_margin_bottom(8);
			bbox->set_margin_top(8);
			bbox->set_margin_start(8);
			bbox->set_margin_end(8);

			add_action_button("Create Entity", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_entity));
			add_action_button("Edit Entity", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_entity));
			add_action_button("Duplicate Entity", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_entity));
			if(remote_repo.size())
				add_action_button("Merge Entity", bbox, br, [this](const UUID &uu){remote_box->merge_item(ObjectType::ENTITY, uu);});

			bbox->show_all();

			box->pack_start(*bbox, false, false, 0);

			auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
			sep->show();
			box->pack_start(*sep, false, false, 0);
			box->pack_start(*br, true, true, 0);
			box->show();

			append_page(*box, "Entities");
		}

		{
			auto br = Gtk::manage(new PoolBrowserPadstack(&pool));
			br->set_show_path(true);
			br->set_include_padstack_type(Padstack::Type::VIA, true);
			br->set_include_padstack_type(Padstack::Type::MECHANICAL, true);
			br->signal_activated().connect([this, br] {
				auto uu = br->get_selected();
				auto path = pool.get_filename(ObjectType::PADSTACK, uu);
				spawn(PoolManagerProcess::Type::IMP_PADSTACK, {path});
			});

			br->show();
			browsers.insert(br);

			auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));
			paned->add1(*br);
			auto canvas = Gtk::manage(new CanvasGL());
			canvas->set_selection_allowed(false);
			paned->add2(*canvas);
			paned->show_all();

			br->signal_selected().connect([this, br, canvas]{
				auto sel = br->get_selected();
				if(!sel) {
					canvas->clear();
					return;
				}
				Padstack ps = *pool.get_padstack(sel);
				for(const auto &la: ps.get_layers()) {
					canvas->set_layer_display(la.first, LayerDisplay(true, LayerDisplay::Mode::FILL_ONLY, la.second.color));
				}
				canvas->property_layer_opacity() = 75;
				canvas->update(ps);
				auto bb = ps.get_bbox();
				int64_t pad = .1_mm;
				bb.first.x -= pad;
				bb.first.y -= pad;

				bb.second.x += pad;
				bb.second.y += pad;
				canvas->zoom_to_bbox(bb.first, bb.second);
			});


			auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
			auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
			bbox->set_margin_bottom(8);
			bbox->set_margin_top(8);
			bbox->set_margin_start(8);
			bbox->set_margin_end(8);

			add_action_button("Create Padstack", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_padstack));
			add_action_button("Edit Padstack", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_padstack));
			add_action_button("Duplicate Padstack", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_padstack));
			if(remote_repo.size())
				add_action_button("Merge Padstack", bbox, br, [this](const UUID &uu){remote_box->merge_item(ObjectType::PADSTACK, uu);});

			bbox->show_all();

			box->pack_start(*bbox, false, false, 0);

			auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
			sep->show();
			box->pack_start(*sep, false, false, 0);
			box->pack_start(*paned, true, true, 0);
			box->show();

			append_page(*box, "Padstacks");
		}

		{
			auto br = Gtk::manage(new PoolBrowserPackage(&pool));
			br->set_show_path(true);
			br->signal_activated().connect([this, br] {
				auto uu = br->get_selected();
				auto path = pool.get_filename(ObjectType::PACKAGE, uu);
				spawn(PoolManagerProcess::Type::IMP_PACKAGE, {path});
			});

			br->show();
			browsers.insert(br);

			auto paned = Gtk::manage(new Gtk::Paned(Gtk::ORIENTATION_HORIZONTAL));
			paned->add1(*br);
			auto canvas = Gtk::manage(new CanvasGL());
			canvas->set_selection_allowed(false);
			paned->add2(*canvas);
			paned->show_all();

			br->signal_selected().connect([this, br, canvas]{
				auto sel = br->get_selected();
				if(!sel) {
					canvas->clear();
					return;
				}
				Package pkg = *pool.get_package(sel);
				for(const auto &la: pkg.get_layers()) {
					auto ld = LayerDisplay::Mode::OUTLINE;
					if(la.second.copper)
						ld = LayerDisplay::Mode::FILL_ONLY;

					canvas->set_layer_display(la.first, LayerDisplay(true, ld, la.second.color));
				}
				pkg.apply_parameter_set({});
				canvas->property_layer_opacity() = 75;
				canvas->update(pkg);
				auto bb = pkg.get_bbox();
				int64_t pad = 1_mm;
				bb.first.x -= pad;
				bb.first.y -= pad;

				bb.second.x += pad;
				bb.second.y += pad;
				canvas->zoom_to_bbox(bb.first, bb.second);
			});


			auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
			auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
			bbox->set_margin_bottom(8);
			bbox->set_margin_top(8);
			bbox->set_margin_start(8);
			bbox->set_margin_end(8);

			add_action_button("Create Package", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_package));
			add_action_button("Edit Package", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_package));
			add_action_button("Duplicate Package", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_package));
			add_action_button("Create Padstack for Package", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_create_padstack_for_package));
			if(remote_repo.size())
				add_action_button("Merge Package", bbox, br, [this](const UUID &uu){remote_box->merge_item(ObjectType::PACKAGE, uu);});
			add_action_button("Part Wizard...", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_part_wizard))->get_style_context()->add_class("suggested-action");

			bbox->show_all();

			box->pack_start(*bbox, false, false, 0);

			auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
			sep->show();
			box->pack_start(*sep, false, false, 0);
			box->pack_start(*paned, true, true, 0);
			box->show();

			append_page(*box, "Packages");
		}

		{
			auto br = Gtk::manage(new PoolBrowserPart(&pool));
			br->set_show_path(true);
			br->signal_activated().connect([this, br] {
				auto uu = br->get_selected();
				if(!uu)
						return;
				auto path = pool.get_filename(ObjectType::PART, uu);
				spawn(PoolManagerProcess::Type::PART, {path});
			});

			br->show();
			browsers.insert(br);

			auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0));
			auto bbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
			bbox->set_margin_bottom(8);
			bbox->set_margin_top(8);
			bbox->set_margin_start(8);
			bbox->set_margin_end(8);

			add_action_button("Create Part", bbox, sigc::mem_fun(this, &PoolNotebook::handle_create_part));
			add_action_button("Edit Part", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_edit_part));
			add_action_button("Duplicate Part", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_duplicate_part));
			add_action_button("Create Part from Part", bbox, br, sigc::mem_fun(this, &PoolNotebook::handle_create_part_from_part));
			if(remote_repo.size())
				add_action_button("Merge Part", bbox, br, [this](const UUID &uu){remote_box->merge_item(ObjectType::PART, uu);});


			bbox->show_all();

			box->pack_start(*bbox, false, false, 0);

			auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
			sep->show();
			box->pack_start(*sep, false, false, 0);
			box->pack_start(*br, true, true, 0);
			box->show();

			append_page(*box, "Parts");
		}

		if(remote_repo.size()) {
			remote_box = PoolRemoteBox::create(this);

			remote_box->show();
			append_page(*remote_box, "Remote");
			remote_box->unreference();

			signal_switch_page().connect([this] (Gtk::Widget *page, int page_num) {
				if(page == remote_box && !remote_box->prs_refreshed_once) {
					remote_box->handle_refresh_prs();
					remote_box->prs_refreshed_once = true;
				}
			});
		}

		for(auto br: browsers) {
			add_context_menu(br);
		}

	}

	void PoolNotebook::add_context_menu(PoolBrowser *br) {
		ObjectType ty = br->get_type();
		br->add_context_menu_item("Delete", [this, ty](const UUID &uu) {
			handle_delete(ty, uu);
		});
		br->add_context_menu_item("Copy path", [this, ty](const UUID &uu) {
			handle_copy_path(ty, uu);
		});
	}

	void PoolNotebook::handle_delete(ObjectType ty, const UUID &uu) {
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
		Gtk::MessageDialog md(*top,  "Permanently delete "+object_descriptions.at(ty).name + "?", false /* use_markup */, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
		md.add_button("Cancel", Gtk::RESPONSE_CANCEL);
		md.add_button("Delete", Gtk::RESPONSE_OK)->get_style_context()->add_class("destructive-action");
		if(md.run() == Gtk::RESPONSE_OK) {
			auto filename = pool.get_filename(ty, uu);
			Gio::File::create_for_path(filename)->remove();
			pool_update();
		}
	}

	void PoolNotebook::handle_copy_path(ObjectType ty, const UUID &uu) {
		auto filename = pool.get_filename(ty, uu);
		auto clip = Gtk::Clipboard::get();
		clip->set_text(filename);
	}

	void PoolNotebook::show_duplicate_window(ObjectType ty, const UUID &uu) {
		if(!uu)
			return;
		if(!duplicate_window) {
			duplicate_window = new DuplicateWindow(&pool, ty, uu);
			duplicate_window->present();
			duplicate_window->signal_hide().connect([this]{
				if(duplicate_window->get_duplicated()) {
					pool_update();
				}
				delete duplicate_window;
				duplicate_window = nullptr;
			});
		}
		else {
			duplicate_window->present();
		}
	}

	bool PoolNotebook::can_close() {
		return processes.size() == 0 && part_wizard==nullptr && !pool_updating && duplicate_window == nullptr;
	}

	void PoolNotebook::prepare_close() {
		if(remote_box)
			remote_box->prs_refreshed_once = true;
	}

	static void send_msg(zmq::socket_t &sock, PoolUpdateStatus st, const std::string &s) {
		size_t sz = sizeof(pool_update_msg_t)+s.size()+1;
		auto msg = reinterpret_cast<pool_update_msg_t *>(alloca(sz));
		msg->status = st;
		strcpy(msg->msg, s.data());
		zmq::message_t zmsg(sz);
		memcpy(zmsg.data(), msg, sz);
		sock.send(zmsg);
	}

	static void pool_update_thread(const std::string &pool_base_path, zmq::context_t &zctx, const std::string &ep) {
		std::cout << "hello from thread" << std::endl;
		zmq::socket_t sock(zctx, ZMQ_PUB);
		sock.connect(ep);

		try {
			horizon::pool_update(pool_base_path, [&sock](PoolUpdateStatus st, std::string s){
				send_msg(sock, st, s);
			});
		}
		catch(const std::runtime_error &e) {
			send_msg(sock, PoolUpdateStatus::ERROR, std::string("runtime exception: ")+e.what());
		}
		catch(const std::exception &e) {
			send_msg(sock, PoolUpdateStatus::ERROR, std::string("generic exception: ")+e.what());
		}
		catch(...) {
			send_msg(sock, PoolUpdateStatus::ERROR, "unknown exception");
		}
	}

	void PoolNotebook::pool_update(std::function<void()> cb) {
		if(pool_updating)
			return;
		appwin->set_pool_updating(true, true);
		pool_update_n_files = 0;
		pool_updating = true;
		pool_update_done_cb = cb;
		std::thread thr(pool_update_thread, std::ref(base_path), std::ref(zctx), std::ref(sock_pool_update_ep));
		thr.detach();
	}

}
