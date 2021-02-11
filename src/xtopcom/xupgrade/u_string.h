#ifndef U_STRING_H_
#define U_STRING_H_
#include <string>
#include <algorithm>

class u_string{
public:
	static int skip_chr(const char* str, int len/*Allow 0*/, const char* skip);
	static bool match_line(int id, const char* line, int len, const char* match/* all | *- |-* | *-* | 0-2 */);
	static char* strcasestr(const char* str, int len, const char* sub, int sub_len, bool nocase);
	static char* strchr(const char* str, int len, char ch);

	// for std::string
	// trim from start 
	static inline std::string &ltrim(std::string &s) { 
	        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace)))); 
	        return s; 
	} 

	// trim from end 
	static inline std::string &rtrim(std::string &s) { 
	        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end()); 
	        return s; 
	} 

	static inline std::string &trim(std::string &s) { 
	        return ltrim(rtrim(s)); 
	} 
	static std::string HexEncode(const std::string& str);
	static std::string HexDecode(const std::string& str);
};
#endif
