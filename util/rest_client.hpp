#pragma once
#include <string>
#include "json.hpp"
#include <curl/curl.h>

namespace REST {
	using json = nlohmann::json;

	class Client {
		friend size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);
		public:
			Client(const std::string &base);
			void set_auth(const std::string &user, const std::string &passwd);

			json get(const std::string &url);
			json post(const std::string &url, const json &postdata=nullptr);


			~Client();
		private:
			const std::string base_url;
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
	};

};
