#pragma once
#include <nlohmann/json_fwd.hpp>
#include "http_client.hpp"

namespace horizon {
using json = nlohmann::json;
class GitHubClient {
public:
    GitHubClient();
    json login(const std::string &user, const std::string &passwd);
    json login_token(const std::string &token);
    json get_repo(const std::string &owner, const std::string &repo);
    json create_fork(const std::string &owner, const std::string &repo);
    json create_pull_request(const std::string &owner, const std::string &repo, const std::string &title,
                             const std::string &branch, const std::string &base, const std::string &body);
    json get_pull_requests(const std::string &owner, const std::string &repo);
    json get_pull_request(const std::string &owner, const std::string &repo, unsigned int pr);

    json add_issue_comment(const std::string &owner, const std::string &repo, unsigned int id, const std::string &body);

    HTTP::RESTClient client;

private:
    std::string login_user;
};
} // namespace horizon
