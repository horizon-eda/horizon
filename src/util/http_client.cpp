#include "http_client.hpp"
#include "util.hpp"
#include <curl/curl.h>
#include <glibmm/miscutils.h>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace horizon::HTTP {

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    std::string *response = reinterpret_cast<std::string *>(userp);
    response->append(reinterpret_cast<char *>(contents), realsize);
    return realsize;
}

static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    auto headers = reinterpret_cast<std::list<std::string> *>(userdata);
    assert(size == 1);
    headers->emplace_back(buffer, nitems);
    return nitems * size;
}

Client::Client()
{
    curl = curl_easy_init();
    if (!curl)
        throw std::runtime_error("curl_easy_init failed");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "horizon-eda/horizon");

    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers_received);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
}

void Client::set_timeout(int timeout)
{
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
}

void Client::append_header(const char *header)
{
    header_list = curl_slist_append(header_list, header);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
}

void Client::clear_headers()
{
    curl_slist_free_all(header_list);
    header_list = nullptr;
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, NULL);
}

void Client::set_auth(const std::string &user, const std::string &passwd)
{
    curl_easy_setopt(curl, CURLOPT_USERNAME, user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, passwd.c_str());
}

std::string Client::get(const std::string &url)
{
    std::string full_url = url;
    curl_easy_setopt(curl, CURLOPT_POST, 0);
    curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
    response.clear();
    headers_received.clear();
    auto res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string errorstr = curl_easy_strerror(res);
        std::string errorstr_buf(errbuf);
        throw std::runtime_error("error performing GET to" + full_url + " :" + errorstr + " " + errorstr_buf);
    }
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if (code < 200 || code > 299) {
        throw std::runtime_error("unexpected HTTP response code " + std::to_string(code) + " when accessing "
                                 + full_url);
    }
    return response;
}

std::string Client::post(const std::string &url, const std::string &postdata_i)
{
    std::string full_url = url;
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
    postdata = postdata_i;
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, postdata.size());
    response.clear();
    headers_received.clear();
    auto res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string errorstr = curl_easy_strerror(res);
        std::string errorstr_buf(errbuf);
        throw std::runtime_error("error performing POST to" + full_url + " :" + errorstr + " " + errorstr_buf);
    }
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if (code < 200 || code > 299) {
        throw std::runtime_error("unexpected HTTP response code " + std::to_string(code) + " when accessing " + full_url
                                 + " response: " + response);
    }
    return response;
}

std::string Client::post_form(const std::string &url, const std::vector<std::pair<std::string, std::string>> &fields)
{
    std::string full_url = url;
    curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
    auto mime = curl_mime_init(curl);
    for (const auto &[key, value] : fields) {
        auto part = curl_mime_addpart(mime);
        curl_mime_name(part, key.c_str());
        curl_mime_data(part, value.c_str(), CURL_ZERO_TERMINATED);
    }
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    response.clear();
    headers_received.clear();
    auto res = curl_easy_perform(curl);
    curl_mime_free(mime);
    if (res != CURLE_OK) {
        std::string errorstr = curl_easy_strerror(res);
        std::string errorstr_buf(errbuf);
        throw std::runtime_error("error performing POST to" + full_url + " :" + errorstr + " " + errorstr_buf);
    }
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if (code < 200 || code > 299) {
        throw std::runtime_error("unexpected HTTP response code " + std::to_string(code) + " when accessing " + full_url
                                 + " response: " + response);
    }
    return response;
}


Client::~Client()
{
    curl_slist_free_all(header_list);
    curl_easy_cleanup(curl);
}

RESTClient::RESTClient(const std::string &base) : Client(), base_url(base)
{
}

json RESTClient::get(const std::string &url)
{
    return json::parse(Client::get(base_url + url));
}

json RESTClient::post(const std::string &url, const json &postdata_j)
{
    return json::parse(Client::post(base_url + url, postdata_j.dump()));
}

} // namespace horizon::HTTP
