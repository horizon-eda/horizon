#pragma once
#include "nlohmann/json.hpp"
#include <curl/curl.h>
#include <string>
#include <list>

namespace horizon::HTTP {
class Client {
    friend size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);

public:
    Client();
    void set_auth(const std::string &user, const std::string &passwd);
    void set_timeout(int timeout);
    void append_header(const char *header);
    void append_header(const std::string &header)
    {
        append_header(header.c_str());
    }
    void clear_headers();
    using ResponseHeaders = std::list<std::string>;

    std::string get(const std::string &url);
    std::string post(const std::string &url, const std::string &postdata = "");
    std::string post_form(const std::string &url, const std::vector<std::pair<std::string, std::string>> &fields);
    const ResponseHeaders &get_response_headers() const
    {
        return headers_received;
    }

    ~Client();

private:
    CURL *curl = nullptr;
    curl_slist *header_list = nullptr;
    char errbuf[CURL_ERROR_SIZE];

    std::string response;
    std::string postdata;

    class PostBuffer {
    public:
        const char *readptr = nullptr;
        size_t sizeleft = 0;
    };
    PostBuffer post_buffer;
    ResponseHeaders headers_received;
};

using json = nlohmann::json;

class RESTClient : public HTTP::Client {
public:
    RESTClient(const std::string &base);

    json get(const std::string &url);
    json post(const std::string &url, const json &postdata = json());

private:
    const std::string base_url;
};
} // namespace horizon::HTTP
