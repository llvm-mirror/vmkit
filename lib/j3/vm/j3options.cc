#include "j3/j3options.h"
#include "j3/j3config.h"
#include "j3/j3lib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace j3;

J3Options::J3Options() {
	assertionsEnabled = 1;
	selfBitCodePath = SELF_BITCODE;
	rtJar = J3Lib::systemClassesArchives();

	debugEnterIndent = 1;

	debugExecute = 0;
	debugLoad = 0;
	debugResolve = 0;
	debugIniting = 0;
	debugTranslate = 1;
	debugLinking = 0;

	genDebugExecute = debugExecute ? 1 : 0;
}

#define nyi(cmd) ({ fprintf(stderr, "option '%s' not yet implemented\n", cmd); })

J3CmdLineParser::J3CmdLineParser(J3Options* _options, int _argc, char** _argv) {
	options = _options;
	argc = _argc;
	argv = _argv;
}

#define opteq(opt)   (!strcmp(argv[cur], opt))
#define optbeg(opt)  (!memcmp(argv[cur], opt, strlen(opt)))

void J3CmdLineParser::process() {
	cur = 1;

	while(cur < argc) {
		if(opteq("-jar")) {
			nyi("-jar");
			return;
		} else if(opteq("-cp") || opteq("-classpath"))
			nyi("-cp/-classpath");
		else if(optbeg("-D"))
			nyi("-D<name>=<value>");
		else if(opteq("-verbose:class"))
			nyi("-versbose:class");
		else if(opteq("-verbose:gc"))
			nyi("-versbose:gc");
		else if(opteq("-verbose:jni"))
			nyi("-versbose:jni");
		else if(opteq("-verbose"))
			nyi("-versbose");
		else if(optbeg("-version:"))
			nyi("-version:<value>");
		else if(opteq("-version"))
			nyi("-version");
		else if(opteq("-showversion"))
			nyi("showversion");
		else if(opteq("-jre-restrict-search"))
			nyi("-jre-restrict-search");
		else if(opteq("-no-jre-restrict-search"))
			nyi("-no-jre-restrict-search");
		else if(opteq("-?") || opteq("-help"))
			help();
		else if(optbeg("-X"))
			nyi("-X");
		else if(optbeg("-ea:") || optbeg("-enableassertions:"))
			nyi("-ea:/-enableassertions:");
		else if(opteq("-ea") || opteq("-enableassertions"))
			options->assertionsEnabled = 1;
		else if(optbeg("-da:") || optbeg("-disableassertions:"))
			nyi("-da:/-disableassertions:");
		else if(opteq("-da") || opteq("-disableassertions"))
			options->assertionsEnabled = 0;
		else if(opteq("-esa") || opteq("-enablesystemassertions"))
			nyi("-esa/-enablesystemassertions");
		else if(opteq("-dsa") || opteq("-disablesystemassertions"))
			nyi("-dsa/-disablesystemassertions");
		else if(optbeg("-agentlib:"))
			nyi("-agentlib:");
		else if(optbeg("-agentpath:"))
			nyi("-agentpath:");
		else if(optbeg("-javaagent:"))
			nyi("-javaagent:");
		else if(optbeg("-"))
			help();
		else {
			nyi("class args");
			return;
		}
		cur++;
	}

	help();
}

void J3CmdLineParser::help() {
	const char* cmd = "j3";
	fprintf(stdout,
					"Usage: %s [-options] class [args...]\n"
					"           (to execute a class)\n"
					"   or  %s [-options] -jar jarfile [args...]\n"
					"           (to execute a jar file)\n"
					"where options include:\n"
					//					"    -d32          use a 32-bit data model if available\n"
					//					"    -d64          use a 64-bit data model if available\n"
					//					"    -server       to select the \"server\" VM\n"
					//					"    -zero         to select the \"zero\" VM\n"
					//					"    -jamvm        to select the \"jamvm\" VM\n"
					//					"    -avian        to select the \"avian\" VM\n"
					//					"					The default VM is server,\n"
					//					"                  because you are running on a server-class machine.\n"
					"         -cp <class search path of directories and zip/jar files>\n"
					"         -classpath <class search path of directories and zip/jar files>\n"
					"                  separated list of directories, JAR archives,\n"
					"                  and ZIP archives to search for class files.\n"
					"         -D<name>=<value>\n"
					"                  set a system property\n"
					//					"					-verbose:[class|gc|jni]\n"
					//					"                  enable verbose output\n"
					"         -version      print product version and exit\n"
					//					"					-version:<value>\n"
					//					"                  require the specified version to run\n"
					//					"    -showversion  print product version and continue\n"
					//					"    -jre-restrict-search | -no-jre-restrict-search\n"
					//					"                  include/exclude user private JREs in the version search\n"
					"         -? -help      print this help message\n"
					//					"    -X            print help on non-standard options\n"
					"         -ea[:<packagename>...|:<classname>]\n"
					"         -enableassertions[:<packagename>...|:<classname>]\n"
					"                  enable assertions with specified granularity\n"
					"         -da[:<packagename>...|:<classname>]\n"
					"         -disableassertions[:<packagename>...|:<classname>]\n"
					"                  disable assertions with specified granularity\n"
					//					"         -esa | -enablesystemassertions\n"
					//					"                  enable system assertions\n"
					//					"         -dsa | -disablesystemassertions\n"
					//					"                  disable system assertions\n"
					//					"					-agentlib:<libname>[=<options>]\n"
					//					"					load native agent library <libname>, e.g. -agentlib:hprof\n"
					//					"					see also, -agentlib:jdwp=help and -agentlib:hprof=help\n"
					//					"					-agentpath:<pathname>[=<options>]\n"
					//					"                  load native agent library by full pathname\n"
					//					"					-javaagent:<jarpath>[=<options>]\n"
					//					"					load Java programming language agent, see java.lang.instrument\n"
					//					"					-splash:<imagepath>\n"
					//					"                  show splash screen with specified image\n"
					, cmd, cmd
					);
	exit(0);
}
					
void J3Options::process(int argc, char** argv) {
	J3CmdLineParser* p = new J3CmdLineParser(this, argc, argv);
	p->process();
	delete p;
}

