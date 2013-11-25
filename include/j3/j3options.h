#ifndef _J3_CMD_LINE_H_
#define _J3_CMD_LINE_H_

#include <stdint.h>

namespace j3 {
	class J3Options;

	class J3CmdLineParser {
		J3Options* options;
		char**     argv;
		int        argc;
		int        cur;

		void       help();
	public:
		J3CmdLineParser(J3Options* _options, int _argc, char** _argv);

		void       process();
	};

	class J3Options {
		friend class J3CmdLineParser;

	public:
		bool           assertionsEnabled;
		const char*    selfBitCodePath;
		const char*    rtJar;
		bool           debugEnterIndent;
		uint32_t       genDebugExecute;
		uint32_t       debugExecute;
		uint32_t       debugLoad;
		uint32_t       debugResolve;
		uint32_t       debugIniting;
		uint32_t       debugTranslate;
		uint32_t       debugLinking;

		J3Options();

		void process(int argc, char** argv);
	};
} 

#endif
