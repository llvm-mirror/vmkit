#ifndef _UTIL_H_
#define _UTIL_H_

namespace vmkit {
	class Util {
	public:
		struct char_less_t {
			bool operator()(const char* s1, const char* s2) const;
		};

		struct char_less_t_dbg {
			bool operator()(const char* s1, const char* s2) const;
		};

		struct wchar_t_less_t {
			bool operator()(const wchar_t* lhs, const wchar_t* rhs) const;
		};

		static struct char_less_t     char_less;
		static struct char_less_t_dbg char_less_dbg;
		static struct wchar_t_less_t  wchar_t_less;
	};
};

#endif
