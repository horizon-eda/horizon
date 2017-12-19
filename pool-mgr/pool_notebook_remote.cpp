#include "pool_notebook.hpp"
#include "pool-update/pool-update.hpp"
#include <thread>
#include <git2.h>

namespace horizon {
	void PoolNotebook::handle_remote_upgrade() {
		if(pool_updating)
			return;
		pool_updating = true;
		remote_upgrading = true;
		remote_upgrade_error = false;

		std::thread thr(&PoolNotebook::remote_upgrade_thread, this);

		thr.detach();

	}

	template <typename T> class autofree_ptr {
		public:
			autofree_ptr(T *p, std::function<void(T*)> ffn): ptr(p), free_fn(ffn) {

			}
			autofree_ptr(std::function<void(T*)> ffn): free_fn(ffn) {

			}
			T *ptr = nullptr;
			std::function<void(T*)> free_fn;

			T& operator* () {
				return *ptr;
			}

			T* operator-> () const {
				return ptr;
			}

			operator T*() const {
				return ptr;
			}

			~autofree_ptr() {
				free_fn(ptr);
			}
	};

	void PoolNotebook::remote_upgrade_thread() {
		try {
			std::string remote_path = Glib::build_filename(base_path, ".remote");

			{
				std::lock_guard<std::mutex> lock(remote_upgrade_mutex);
				remote_upgrade_status = "Opening repository...";
			}
			remote_upgrade_dispatcher.emit();

			autofree_ptr<git_repository> repo(git_repository_free);
			if(git_repository_open(&repo.ptr, remote_path.c_str()) != 0) {
				throw std::runtime_error("error opening repo");
			}

			autofree_ptr<git_remote> remote(git_remote_free);
			if(git_remote_lookup(&remote.ptr, repo, "origin") != 0) {
				throw std::runtime_error("error looking up remote");
			}

			{
				std::lock_guard<std::mutex> lock(remote_upgrade_mutex);
				remote_upgrade_status = "Fetching...";
			}
			remote_upgrade_dispatcher.emit();

			git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
			if(git_remote_fetch(remote, NULL, &fetch_opts, NULL) != 0) {
				throw std::runtime_error("error fetching");
			}

			autofree_ptr<git_annotated_commit> latest_commit(git_annotated_commit_free);
			if(git_annotated_commit_from_revspec(&latest_commit.ptr, repo, "origin/master") != 0) {
				throw std::runtime_error("error getting latest commit ");
			}

			auto oid = git_annotated_commit_id(latest_commit);

			git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
			checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

			const git_annotated_commit *com = latest_commit.ptr;
			git_merge_analysis_t merge_analysis;
			git_merge_preference_t merge_prefs = GIT_MERGE_PREFERENCE_FASTFORWARD_ONLY;
			if(git_merge_analysis(&merge_analysis, &merge_prefs, repo, &com, 1) != 0) {
				throw std::runtime_error("error getting merge analysis");
			}

			if(!(merge_analysis&GIT_MERGE_ANALYSIS_UP_TO_DATE)) {
				if(!(merge_analysis&GIT_MERGE_ANALYSIS_FASTFORWARD)) {
					throw std::runtime_error("can't fast-forward");
				}

				autofree_ptr<git_object> obj(git_object_free);
				if(git_object_lookup(&obj.ptr, repo, oid, GIT_OBJ_COMMIT) != 0) {
					throw std::runtime_error("error lookup");
				}

				if(git_checkout_tree(repo, obj, &checkout_opts) != 0) {
					throw std::runtime_error("error checkout");
				}

				autofree_ptr<git_reference> ref_head(git_reference_free);
				if(git_repository_head(&ref_head.ptr, repo) != 0) {
					throw std::runtime_error("error head");
				}

				autofree_ptr<git_reference> ref_new(git_reference_free);
				if(git_reference_set_target(&ref_new.ptr, ref_head, oid, "test") != 0) {
					throw std::runtime_error("error target");
				}
			}
			else {
				std::cout << "up to date" << std::endl;
			}

			{
				std::lock_guard<std::mutex> lock(remote_upgrade_mutex);
				remote_upgrade_status = "Updating remote pool...";
			}
			remote_upgrade_dispatcher.emit();
			horizon::pool_update(remote_path);

			{
				std::lock_guard<std::mutex> lock(remote_upgrade_mutex);
				remote_upgrade_status = "Updating local pool...";
			}
			remote_upgrade_dispatcher.emit();
			horizon::pool_update(base_path);

			{
				std::lock_guard<std::mutex> lock(remote_upgrade_mutex);
				remote_upgrading = false;
				remote_upgrade_status = "Done";
			}
			remote_upgrade_dispatcher.emit();
		}
		catch(const std::exception &e) {
			{
				std::lock_guard<std::mutex> lock(remote_upgrade_mutex);
				remote_upgrading = false;
				remote_upgrade_error = true;
				remote_upgrade_status = "Error: "+std::string(e.what());
			}
			remote_upgrade_dispatcher.emit();
		}

	}
}
