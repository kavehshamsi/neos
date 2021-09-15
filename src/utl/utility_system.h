/*
 * neosutil_system.h
 *
 *  Created on: Jul 6, 2021
 *      Author: kaveh
 */

#ifndef SRC_UTL_UTILITY_SYSTEM_H_
#define SRC_UTL_UTILITY_SYSTEM_H_

#include <unistd.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <memory>
#include <assert.h>

namespace utl {

struct subproc_t {
	std::string path;
	enum status_t {UNINIT, INITIALIZED, ERROR} stat = UNINIT;

	FILE *p2cfd = nullptr, *c2pfd = nullptr;

	subproc_t() {}
	subproc_t(const std::string& epath) { init(epath); }

	int init(const std::string& execpath = "") {
		if (stat == INITIALIZED)
			return 1;

		if (!execpath.empty()) {
			path = execpath;
		}

	    int parent_to_child[2];
	    int child_to_parent[2];

	    pipe(parent_to_child);
	    pipe(child_to_parent);

	    int childPID = fork();

	    if(childPID == 0) {
	        //this is child
	        close(parent_to_child[1]);//Close the writing end of the incoming pipe
	        close(child_to_parent[0]);//Close the reading end of the outgoing pipe

	        dup2(parent_to_child[0], STDIN_FILENO);//replace stdin with incoming pipe
	        dup2(child_to_parent[1], STDOUT_FILENO);//replace stdout with outgoing pipe

	        //exec child process
	        char *newargv[] = { NULL };
	        char *newenviron[] = { NULL };
	        if (execve(path.c_str(), newargv, newenviron) == -1) {
	        	return 0;
	        }
	    }
	    else {
	        //this is parent
	        close(parent_to_child[0]);//Close the reading end of the outgoing pipe.
	        close(child_to_parent[1]);//Close the writing side of the incoming pipe.

	        char reading_buffer;
	        std::string received_str;

	        p2cfd = fdopen(parent_to_child[1], "w");
	        c2pfd = fdopen(child_to_parent[0], "r");
	    }

	    stat = INITIALIZED;

	    return 1;
	}

	void write_to_in(const std::string& str) {

		std::string sstr = str;
		sstr += '\n';

		fwrite(sstr.c_str(), 1, sstr.size(), p2cfd);
		//std::cout << "Parent sent: "<< str << std::endl;
		fflush(p2cfd);

	}

	std::string read_line() {

		assert(stat == INITIALIZED);

		char *line = nullptr;
		size_t n = 0;
		getline(&line, &n, c2pfd);
		//std::cout << "read " << line << " at " << n << "bytes" << std::endl;
		//std::cout << "Parent received: "<< line << std::endl;

		std::string ret(line);

		if (line)
			free(line);

		return ret;
	}

};

/*
 * get time and date
 */

inline std::string time_and_date() {

	time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return std::string(buf);
}

/*
 * execute command line and return output
 */
inline std::string _exec_cmd(const std::string& cmd) {

	std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe)
    	throw std::runtime_error("popen() failed!");

    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }

    return result;
}

} // namespace utl

#endif /* SRC_UTL_UTILITY_SYSTEM_H_ */
