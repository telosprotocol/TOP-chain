#include "u_string.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int u_string::skip_chr( const char* str, int len, const char* skip )
{
	bool match;
	char* p, *s;

	if (len <= 0){
		len = (int)strlen(str);
		if (len == 0) return 0;
	}
	if (!skip) return len;

	p = (char*)str + len;
	while (p > str){
		if (*p != 0){
			match = 0;
			s = (char*)skip;
			while (*s){
				if (*p == *s){
					match = 1;
					break;
				}
				s++;
			}
			if (match) *p = 0;
			else break;
		}
		p--;
	}
	return (int)(p-str);
}

bool u_string::match_line(int id, const char* line, int len, const char* match)
{
	char search[100];
	int search_len;
	char* p, *p_end, *sub, *sub_end;
	int start, end;

	if (!line) return 0;
	if (!match) return 1;
	if (*match == 0 || strcasecmp(match, "all") == 0 || strcasecmp(match, "*") == 0)
		return 1;
	if (len <= 0) len = (int)strlen(line);
	p = (char*)match;
	while (*p){
		p_end = ::strchr(p, '|');
		sub_end = p_end ? p_end : (p + strlen(p));
		if (*p == '*'){
			if (*(sub_end-1) == '*'){
				search_len = (int)(sub_end - p) -2;
				memcpy(search, p+1, search_len);
				search[search_len] = 0;
				if (u_string::strcasestr(line, len, search, 0, 1))
					return 1;
			}else{
				search_len = (int)(sub_end - p) -1;
				memcpy(search, p+1, search_len);
				search[search_len] = 0;
				if (len >= search_len && strcasecmp(line + len - search_len, search) == 0)
					return 1;
			}
		}else if (*(sub_end-1) == '*'){
			search_len = (int)(sub_end - p) -1;
			memcpy(search, p, search_len);
			search[search_len] = 0;
			if (len >= search_len && strncasecmp(line, search, search_len) == 0)
				return 1;
		}else{
			sub = u_string::strchr(p, (int)(sub_end - p), '-');
			if (sub){
				start = atoi(p);
				end = atoi(sub+1);
				if (end == 0) end = -1;
				if (id >= start && id <= end)
					return 1;
			}else{
				start = atoi(p);
				if (id == start)
					return 1;
			}
		}

		if (p_end) p = p_end + 1;
		else break;
	}
	return 0;
}

char* u_string::strcasestr(const char* str, int len, const char* sub, int sub_len, bool nocase)
{
	int i ,ii, iii;
	char *cp, *s1, *s2;

	if (!str || !*str)
		return 0;
	if ( !sub || !*sub)
		return ((char *)str);

	if (len < 0)
		len = (int)strlen(str);
	if (sub_len < 0)
		sub_len = (int)strlen(sub);

	cp = (char*)str;
	i = ii = iii = 0;

	while ((len == 0) ? (*cp) : (i<len)){
		s1 = cp;
		ii = i;
		s2 = (char*)sub;
		iii = 0;
		while (((len == 0) ? (*s1) : (ii<len)) && ((sub_len == 0) ? (*s2) : (iii<sub_len)) && !(nocase ? (::tolower(*s1)-::tolower(*s2)) : (*s1-*s2)) ){
			s1++, ii++, s2++, iii++;
		}
		if ((sub_len == 0) ? !(*s2) : (iii>=sub_len))
			return(cp);
		cp++;
		i++;
	}
	return 0;
}
char* u_string::strchr(const char* str, int len, char ch)
{
	int i = 0;

	if (!str) return 0;

	while (((len == 0) ? (*str) : (i<len)) && *str != ch){
		str++, i++;
	}
	if (*str == ch)
		return((char*)str);
	return 0;
}


const char kHexAlphabet[] = "0123456789abcdef";
const char kHexLookup[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7,  8,  9,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };


std::string u_string::HexEncode(const std::string& str) {
    auto size(str.size());
    std::string hex_output(size * 2, 0);
    for (size_t i(0), j(0); i != size; ++i) {
        hex_output[j++] = kHexAlphabet[static_cast<unsigned char>(str[i]) / 16];
        hex_output[j++] = kHexAlphabet[static_cast<unsigned char>(str[i]) % 16];
    }
    return hex_output;
}

std::string u_string::HexDecode(const std::string& str) {
    auto size(str.size());
    if (size % 2) return "";

    std::string non_hex_output(size / 2, 0);
    for (size_t i(0), j(0); i != size / 2; ++i) {
        non_hex_output[i] = (kHexLookup[static_cast<int>(str[j++])] << 4);
        non_hex_output[i] |= kHexLookup[static_cast<int>(str[j++])];
    }
    return non_hex_output;
}

