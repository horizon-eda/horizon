#pragma once
#include "rest_client.hpp"
#include "json.hpp"

namespace horizon {
	using json = nlohmann::json;
	class GitHubClient {
		public:
			GitHubClient();
			json login(const std::string &user, const std::string &passwd);
			json get_repo(const std::string &owner, const std::string &repo);
			json create_fork(const std::string &owner, const std::string &repo);
			json create_pull_request(const std::string &owner, const std::string &repo, const std::string &title, const std::string &branch, const std::string &base, const std::string &body);
			json get_pull_requests(const std::string &owner, const std::string &repo);

			REST::Client client;

		private:
			std::string login_user;
	};
}
