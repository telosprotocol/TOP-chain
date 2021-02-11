#include "u_system.h"
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "u_string.h"

int u_system::system(const char* cmd, ...)
{
	char cmdline[2048];
	int cmdlen, status;
	va_list vlist;

	va_start(vlist, cmd);
	cmdlen = vsnprintf(cmdline, 2047, cmd, vlist);
	if (cmdlen < 0) cmdlen = 0;
	va_end(vlist);
	cmdline[cmdlen] = 0;

	typedef void (*sighandler_t)(int);
	sighandler_t old_handler;
	old_handler = ::signal(SIGCHLD, SIG_DFL);
	status = ::system(cmdline);

	signal(SIGCHLD, old_handler);
	if (status != -1){
		if(WIFEXITED(status)){
			status = WEXITSTATUS(status);
		}
	}

	return status;
}

int u_system::popen_fill( char* data, int len, const char* matchline, const char* cmd, ... )
{
	FILE* output;
	va_list vlist;
	char line[2048];
	int line_len, line_count, total_len, id;

	if (!data || len <= 0 || !cmd)
		return -1;

	va_start(vlist, cmd);
	line_len = vsnprintf(line, 2047, cmd, vlist);
	if (line_len < 0) line_len = 0;
	va_end(vlist);
	line[line_len] = 0;

	line_count = id = total_len = 0;
	if ((output = popen(line, "r"))) {
		while (!feof(output) && fgets(line, sizeof(line)-1, output)){
			line_len = (int)strlen(line);
			if (len - total_len - 1 < line_len){
				break;
			}
			if (!matchline || u_string::match_line(id, line, line_len-1, matchline)){
				memcpy(data + total_len, line, line_len);
				total_len += line_len;
				line_count++;
			}
			id++;
		}
		pclose(output);
	}
	data[total_len] = 0;
	if (line_count == 1 && total_len > 0){
		u_string::skip_chr(data, 0, "\n");
	}
	return total_len;
}

