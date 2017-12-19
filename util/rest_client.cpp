#include "rest_client.hpp"
#include <curl/curl.h>
#include <stdexcept>
#include <glibmm/miscutils.h>
#include "util.hpp"

namespace REST {

	static size_t write_callback (void *contents, size_t size, size_t nmemb, void *userp) {
	  size_t realsize = size * nmemb;
	  std::string *response = reinterpret_cast<std::string*>(userp);
	  response->append(reinterpret_cast<char*>(contents), realsize);
	  return realsize;
	}

	 size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp) {
		size_t buffer_size = size*nmemb;
		Client::PostBuffer *buf = reinterpret_cast<Client::PostBuffer*>(userp);

		if(buf->sizeleft) {
			/* copy as much as possible from the source to the destination */
			size_t copy_this_much = buf->sizeleft;
			if(copy_this_much > buffer_size)
				copy_this_much = buffer_size;
			memcpy(dest, buf->readptr, copy_this_much);

			buf->readptr += copy_this_much;
			buf->sizeleft -= copy_this_much;
			return copy_this_much; /* we copied this many bytes */
		}
		return 0;
	}


	Client::Client(const std::string &base): base_url(base) {
		curl = curl_easy_init();
		if(!curl)
			throw std::runtime_error("curl_easy_init failed");
		header_list = curl_slist_append(header_list, "Accept: application/vnd.github.v3+json");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "carrotIndustries/horizon");

		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_READDATA, &post_buffer);

		//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		
		#ifdef G_OS_WIN32
		{
			std::string cert_file = Glib::build_filename(horizon::get_exe_dir(), "ca-bundle.crt");
			curl_easy_setopt(curl, CURLOPT_CAINFO, cert_file.c_str());
		}
		#endif
	}

	void Client::set_auth(const std::string &user, const std::string &passwd) {
		curl_easy_setopt(curl, CURLOPT_USERNAME, user.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, passwd.c_str());
	}

	json Client::get(const std::string &url) {
		std::string full_url = base_url+url;
		curl_easy_setopt(curl, CURLOPT_POST, 0);
		curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
		response.clear();
		auto res = curl_easy_perform(curl);
		if(res != CURLE_OK) {
			std::string errorstr = curl_easy_strerror(res);
			std::string errorstr_buf(errbuf);
			throw std::runtime_error("error performing GET to"+full_url + " :"+errorstr + " " + errorstr_buf);
		}
		long code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
		if(code < 200 || code > 299) {
			throw std::runtime_error("unexpected HTTP response code " + std::to_string(code) + " when accessing "+full_url);
		}
		return json::parse(response);
	}

	json Client::post(const std::string &url, const json &postdata_j) {
		std::string full_url = base_url+url;
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
		postdata = postdata_j.dump();
		post_buffer.readptr = postdata.c_str();
		post_buffer.sizeleft = postdata.size();
		response.clear();
		auto res = curl_easy_perform(curl);
		if(res != CURLE_OK) {
			std::string errorstr = curl_easy_strerror(res);
			std::string errorstr_buf(errbuf);
			throw std::runtime_error("error performing POST to"+full_url + " :"+errorstr + " " + errorstr_buf);
		}
		long code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
		if(code < 200 || code > 299) {
			throw std::runtime_error("unexpected HTTP response code " + std::to_string(code) + " when accessing "+full_url);
		}
		return json::parse(response);
	}

	Client::~Client() {
		curl_slist_free_all(header_list);
		curl_easy_cleanup(curl);
	}

}
