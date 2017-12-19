#pragma once
#include "rest_client.hpp"
#include "json.hpp"

namespace horizon {
	using json = nlohmann::json;
	class GitHubClient {
		public:
			GitHubClient();
			void login(const std::string &user, const std::string &passwd);
			json get_repo(const std::string &owner, const std::string &repo);

			REST::Client client;
	};
}
