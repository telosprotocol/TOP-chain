#ifndef U_SYSTEM_H_
#define U_SYSTEM_H_

class u_system
{
public:
	static int system(const char* cmd, ...);
	static int popen_fill(char* data, int len, const char* matchline, const char* cmd, ... ); //return succ >= 0 fail -1
};
#endif
