#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <string>

#if defined(__linux__) || defined(__FreeBSD__)
#define LINUX_OS 1
#elif defined(__APPLE__)
#define MACOS_OS 1
#else
#error OS detection failed.
#endif

#include <cxxabi.h>

#if defined(MACOS_OS)
#define SELF_HANDLE RTLD_SELF
#elif defined(LINUX_OS)
#define SELF_HANDLE 0
#else
#error Please define constants for your OS.
#endif

namespace vmkit {
	class System {
	public:
		static const char* mcjitSymbol(const std::string &Name) {
#if defined(MACOS_OS)
			return Name.c_str() + 1;
#elif defined(LINUX_OS)
			return Name.c_str();
#else
#error "what is the correct symbol for your os?"
#endif
		}
	};
}

#endif
