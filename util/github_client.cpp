#include "github_client.hpp"

namespace horizon {
	GitHubClient::GitHubClient(): client("https://api.github.com") {

	}

	json GitHubClient::login(const std::string &user, const std::string &passwd) {
		client.set_auth(user, passwd);
		login_user= user;
		return client.get("/user");
	}

	json GitHubClient::get_repo(const std::string &owner, const std::string &repo) {
		return client.get("/repos/"+owner+"/"+repo);
	}

	json GitHubClient::create_fork(const std::string &owner, const std::string &repo) {
		return client.post("/repos/"+owner+"/"+repo+"/forks");
	}

	json GitHubClient::create_pull_request(const std::string &owner, const std::string &repo, const std::string &title, const std::string &branch, const std::string &base, const std::string &body) {
		json j;
		j["title"] = title;
		j["head"] = login_user + ":" + branch;
		j["base"] = base;
		j["body"] = body;
		j["maintainer_can_modify"] = true;

		return client.post("/repos/"+owner+"/"+repo+"/pulls", j);
	}

	json GitHubClient::get_pull_requests(const std::string &owner, const std::string &repo) {
		return client.get("/repos/"+owner+"/"+repo+"/pulls");
	}
}
