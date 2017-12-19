#include "github_client.hpp"

namespace horizon {
	GitHubClient::GitHubClient(): client("https://api.github.com") {

	}

	void GitHubClient::login(const std::string &user, const std::string &passwd) {
		client.set_auth(user, passwd);
		client.get("/user");
	}

	json GitHubClient::get_repo(const std::string &owner, const std::string &repo) {
		return client.get("/repos/"+owner+"/"+repo);
	}
}
