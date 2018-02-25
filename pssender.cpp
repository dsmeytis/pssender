//============================================================================
// Name        : pssender.cpp
// Author      : D.Meytis
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <curl/curl.h>

#include "json.hpp"
#include "fifo_map.hpp"

using namespace std;

#define MAXLINE 512

template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using my_json = nlohmann::basic_json<my_workaround_fifo_map>;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
   return size * nmemb;
}

int main(int argc, char** argv) {
	if (argc != 2) {
		cout << "Usage: pssender N [N - interval between messages]" << endl;
		return EINVAL;
	}
	long int sleepval = strtol(argv[1], NULL, 0);

	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	if (curl == nullptr) {
		cerr << "curl_easy_init() error";
		return -1;
	}

	curl_easy_setopt(curl, CURLOPT_URL, "http://uek4lvwoyyoosvlq.onion/stats");
	curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5_HOSTNAME);
	curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:9050");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); /* dummy func to ignore POST response */

	for (;;) {
		FILE *ps = popen("ps l", "r");
		if (ps == nullptr) {
			perror("Unable to get current processes list");
			return -1;
		}
		char buf[MAXLINE];
		int d, uid, pid;
		char s[10], ccmd[64];
		my_json j;
		fgets(buf, MAXLINE, ps);
		while (fgets(buf, MAXLINE, ps) != nullptr) {
			sscanf(buf, "%d %d %d %d %d %d %d %d %s %s %s %s %s",
					&d, &uid, &pid, &d, &d, &d, &d, &d, s, s, s, s, ccmd);
			string cmd(ccmd);
			j += {{"uid", uid}, {"pid", pid}, {"cmd", cmd}};
		}
		pclose(ps);
		string js = j.dump();
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, js.data());

		do {
			res = curl_easy_perform(curl);
			if (res == CURLE_OK)
				cout << "Processes list successfully sent" << endl;
			else
				cerr << "Sending error: " << curl_easy_strerror(res) << endl;
		} while (res != CURLE_OK);

		sleep(sleepval);
	}
	curl_easy_cleanup(curl);
	return 0;
}
