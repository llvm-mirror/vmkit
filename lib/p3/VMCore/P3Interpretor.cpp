#include "P3Interpretor.h"
#include "P3Object.h"

#include "opcode.h"

using namespace p3;

#define fetch_arg() (pc+=2, bc->content[pc-1]<<8+bc->content[pc-2])
#define push(val)   stack->content[stack->length++]=val
#define pop()       stack->content[--stack->length]

P3Object* P3Interpretor::execute() {
	printf("Execute: ");
	code->print();
	printf("\n");
	P3Stack*   stack = new P3Stack(code->py_nstacks);
	P3String*  bc = code->py_code;
	uint32     pc=0;

	while(1) {
		uint8 op = bc->content[pc++];
		
		printf("  -- stack: ");
		stack->print();
		printf("\n");
		printf("  -- fetch opcode %d (%s)\n", op, opcodeNames[op]);

		switch(op) {
			case STOP_CODE: //0
			case POP_TOP: //1
			case ROT_TWO: //2
			case ROT_THREE: //3
			case DUP_TOP: //4
			case ROT_FOUR: //5
			case NOP: //9

			case UNARY_POSITIVE: //10
			case UNARY_NEGATIVE: //11
			case UNARY_NOT: //12
			case UNARY_CONVERT: //13
			
			case UNARY_INVERT: //15
			
			case LIST_APPEND: //18
			case BINARY_POWER: //19
			
			case BINARY_MULTIPLY: //20
			case BINARY_DIVIDE: //21
			case BINARY_MODULO: //22
			case BINARY_ADD: //23
			case BINARY_SUBTRACT: //24
			case BINARY_SUBSCR: //25
			case BINARY_FLOOR_DIVIDE: //26
			case BINARY_TRUE_DIVIDE: //27
			case INPLACE_FLOOR_DIVIDE: //28
			case INPLACE_TRUE_DIVIDE: //29
			
			case SLICE: //30
				/* Also uses : 31-33 */
			
			case STORE_SLICE: //40
				/* Also uses : 41-43 */
			
			case DELETE_SLICE: //50
				/* Also uses : 51-53 */
			
			case STORE_MAP: //54
			case INPLACE_ADD: //55
			case INPLACE_SUBTRACT: //56
			case INPLACE_MULTIPLY: //57
			case INPLACE_DIVIDE: //58
			case INPLACE_MODULO: //59
			case STORE_SUBSCR: //60
			case DELETE_SUBSCR: //61
			
			case BINARY_LSHIFT: //62
			case BINARY_RSHIFT: //63
			case BINARY_AND: //64
			case BINARY_XOR: //65
			case BINARY_OR: //66
			case INPLACE_POWER: //67
			case GET_ITER: //68
			
			case PRINT_EXPR: //70
				NI();

			case PRINT_ITEM: //71
				pop()->print();
				break;
				
			case PRINT_NEWLINE: //72
				printf("\n");
				break;

			case PRINT_ITEM_TO: //73
			case PRINT_NEWLINE_TO: //74
			case INPLACE_LSHIFT: //75
			case INPLACE_RSHIFT: //76
			case INPLACE_AND: //77
			case INPLACE_XOR: //78
			case INPLACE_OR: //79
			case BREAK_LOOP: //80
			case WITH_CLEANUP: //81
			case LOAD_LOCALS: //82
				NI();

			case RETURN_VALUE: //83
				return pop();

			case IMPORT_STAR: //84
			case EXEC_STMT: //85
			case YIELD_VALUE: //86
			case POP_BLOCK: //87
			case END_FINALLY: //88
			case BUILD_CLASS: //89

			case HAVE_ARGUMENT: //90
				/* Opcodes from here have an argument: */

				//			case STORE_NAME: //90
				/* Index in name list */
			case DELETE_NAME: //91
				/* "" */
			case UNPACK_SEQUENCE: //92
				/* Number of sequence items */
			case FOR_ITER: //93
			
			case STORE_ATTR: //95
				/* Index in name list */
			case DELETE_ATTR: //96
				/* "" */
			case STORE_GLOBAL: //97
				/* "" */
			case DELETE_GLOBAL: //98
				/* "" */
			case DUP_TOPX: //99
				/* number of items to duplicate */
				NI();
				
			case LOAD_CONST: // 100
				push(code->py_const->at(fetch_arg()));
				break;

				/* Index in const list */
			case LOAD_NAME: //101
				/* Index in name list */
			case BUILD_TUPLE: //102
				/* Number of tuple items */
			case BUILD_LIST: //103
				/* Number of list items */
			case BUILD_MAP: //104
				/* Always zero for now */
			case LOAD_ATTR: //105
				/* Index in name list */
			case COMPARE_OP: //106
				/* Comparison operator */
			case IMPORT_NAME: //107
				/* Index in name list */
			case IMPORT_FROM: //108
				/* Index in name list */
			
			case JUMP_FORWARD: //110
				/* Number of bytes to skip */
			case JUMP_IF_FALSE: //111
				/* "" */
			case JUMP_IF_TRUE: //112
				/* "" */
			case JUMP_ABSOLUTE: //113
				/* Target byte offset from beginning of code */

			case LOAD_GLOBAL: //116
				/* Index in name list */

			case CONTINUE_LOOP: //119
				/* Start of loop (absolute) */
			case SETUP_LOOP: //120
				/* Target address (relative) */
			case SETUP_EXCEPT: //121
				/* "" */
			case SETUP_FINALLY: //122
				/* "" */

			case LOAD_FAST: //124
				/* Local variable number */
			case STORE_FAST: //125
				/* Local variable number */
			case DELETE_FAST: //126
				/* Local variable number */

			case RAISE_VARARGS: //130
				/* Number of raise arguments (1, 2 or 3) */
				/* CALL_FUNCTION_XXX opcodes defined below depend on this definition */
			case CALL_FUNCTION: //131
				/* #args + (#kwargs<<8) */
			case MAKE_FUNCTION: //132
				/* #defaults */
			case BUILD_SLICE: //133
				/* Number of items */

			case MAKE_CLOSURE: //134
				/* #free vars */
			case LOAD_CLOSURE: //135
				/* Load free variable from closure */
			case LOAD_DEREF: //136
				/* Load and dereference from closure cell */ 
			case STORE_DEREF: //137
				/* Store into cell */ 

				/* The next 3 opcodes must be contiguous and satisfy
					 (CALL_FUNCTION_VAR - CALL_FUNCTION) & 3 == 1  */
			case CALL_FUNCTION_VAR: //140
				/* #args + (#kwargs<<8) */
			case CALL_FUNCTION_KW: //141
				/* #args + (#kwargs<<8) */
			case CALL_FUNCTION_VAR_KW: //142
				/* #args + (#kwargs<<8) */

				/* Support for opargs more than : //16 bits long */
			case EXTENDED_ARG: //143

			default:
				fatal("unable to interpret opcode: %d", op);

		}
	}
}

const char* P3Interpretor::opcodeNames[] = {
	"STOP_CODE",              // 0
	"POP_TOP",                // 1
	"ROT_TWO",                // 2
	"ROT_THREE",              // 3
	"DUP_TOP",                // 4
	"ROT_FOUR",               // 5
	"6__unknow",              // 6
	"7__unknow",              // 7
	"8__unknow",              // 8
	"NOP",                    // 9

	"UNARY_POSITIVE",         // 10
	"UNARY_NEGATIVE",         // 11
	"UNARY_NOT",              // 12
	"UNARY_CONVERT",          // 13
	"14__unknow",             // 14
	"UNARY_INVERT",           // 15
	"16__unknow",             // 16
	"17__unknow",             // 17
	"LIST_APPEND",            // 18
	"BINARY_POWER",           // 19

	"BINARY_MULTIPLY",        // 20
	"BINARY_DIVIDE",          // 21
	"BINARY_MODULO",          // 22
	"BINARY_ADD",             // 23
	"BINARY_SUBTRACT",        // 24
	"BINARY_SUBSCR",          // 25
	"BINARY_FLOOR_DIVIDE",    // 26
	"BINARY_TRUE_DIVIDE",     // 27
	"INPLACE_FLOOR_DIVIDE",   // 28
	"INPLACE_TRUE_DIVIDE",    // 29

	"SLICE",                  // 30
	/* Also uses , // 31-, // 33 */
	"31__SLICE",              // 31
	"32__SLICE",              // 32
	"33__SLICE",              // 33
	"34__unknow",             // 34
	"35__unknow",             // 35
	"36__unknow",             // 36
	"37__unknow",             // 37
	"38__unknow",             // 38
	"39__unknow",             // 39

	"STORE_SLICE",            // 40
	/* Also uses , // 41-, // 43 */
	"41__STORE_SLICE",        // 41
	"42__STORE_SLICE",        // 42
	"43__STORE_SLICE",        // 43
	"44__unknow",             // 44
	"45__unknow",             // 45
	"46__unknow",             // 46
	"47__unknow",             // 47
	"48__unknow",             // 48
	"49__unknow",             // 49

	"DELETE_SLICE",           // 50
	/* Also uses , // 51-, // 53 */
	"51__DELETE_SLICE",       // 51
	"52__DELETE_SLICE",       // 52
	"53__DELETE_SLICE",       // 53
	"STORE_MAP",              // 54
	"INPLACE_ADD",            // 55
	"INPLACE_SUBTRACT",       // 56
	"INPLACE_MULTIPLY",       // 57
	"INPLACE_DIVIDE",         // 58
	"INPLACE_MODULO",         // 59

	"STORE_SUBSCR",           // 60
	"DELETE_SUBSCR",          // 61
	"BINARY_LSHIFT",          // 62
	"BINARY_RSHIFT",          // 63
	"BINARY_AND",             // 64
	"BINARY_XOR",             // 65
	"BINARY_OR",              // 66
	"INPLACE_POWER",          // 67
	"GET_ITER",               // 68
	"69__unknow",             // 69

	"PRINT_EXPR",             // 70
	"PRINT_ITEM",             // 71
	"PRINT_NEWLINE",          // 72
	"PRINT_ITEM_TO",          // 73
	"PRINT_NEWLINE_TO",       // 74
	"INPLACE_LSHIFT",         // 75
	"INPLACE_RSHIFT",         // 76
	"INPLACE_AND",            // 77
	"INPLACE_XOR",            // 78
	"INPLACE_OR",             // 79

	"BREAK_LOOP",             // 80
	"WITH_CLEANUP",           // 81
	"LOAD_LOCALS",            // 82
	"RETURN_VALUE",           // 83
	"IMPORT_STAR",            // 84
	"EXEC_STMT",              // 85
	"YIELD_VALUE",            // 86
	"POP_BLOCK",              // 87
	"END_FINALLY",            // 88
	"BUILD_CLASS",            // 89

	"HAVE_ARGUMENT",          // 90	/* Opcodes from here have an argument: */
	//	"STORE_NAME",             // 90	/* Index in name list */
	"DELETE_NAME",            // 91	/* "", */
	"UNPACK_SEQUENCE",        // 92	/* Number of sequence items */
	"FOR_ITER",               // 93
	"94__unknow",             // 94
	"STORE_ATTR",             // 95	/* Index in name list */
	"DELETE_ATTR",            // 96	/* "" */
	"STORE_GLOBAL",           // 97	/* "" */
	"DELETE_GLOBAL",          // 98	/* "" */
	"DUP_TOPX",               // 99	/* number of items to duplicate */

	"LOAD_CONST",             // 100	/* Index in const list */
	"LOAD_NAME",              // 101	/* Index in name list */
	"BUILD_TUPLE",            // 102	/* Number of tuple items */
	"BUILD_LIST",             // 103	/* Number of list items */
	"BUILD_MAP",              // 104	/* Always zero for now */
	"LOAD_ATTR",              // 105	/* Index in name list */
	"COMPARE_OP",             // 106	/* Comparison operator */
	"IMPORT_NAME",            // 107	/* Index in name list */
	"IMPORT_FROM",            // 108	/* Index in name list */
	"109__unknow",            // 109

	"JUMP_FORWARD",           // 110	/* Number of bytes to skip */
	"JUMP_IF_FALSE",          // 111	/* "" */
	"JUMP_IF_TRUE",           // 112	/* "" */
	"JUMP_ABSOLUTE",          // 113	/* Target byte offset from beginning of code */
	"114__unknow",            // 114
	"115__unknow",            // 115
	"LOAD_GLOBAL",            // 116	/* Index in name list */
	"117__unknow",            // 117
	"118__unknow",            // 118
	"CONTINUE_LOOP",          // 119	/* Start of loop (absolute) */

	"SETUP_LOOP",             // 120	/* Target address (relative) */
	"SETUP_EXCEPT",           // 121	/* "" */
	"SETUP_FINALLY",          // 122	/* "" */
	"123__unknow",            // 123
	"LOAD_FAST",              // 124	/* Local variable number */
	"STORE_FAST",             // 125	/* Local variable number */
	"DELETE_FAST",            // 126	/* Local variable number */
	"127__unknow",            // 127
	"128__unknow",            // 128
	"129__unknow",            // 129

	"RAISE_VARARGS",          // 130	/* Number of raise arguments (, // 1, , // 2 or , // 3) */
	/* CALL_FUNCTION_XXX opcodes defined below depend on this definition */
	"CALL_FUNCTION",          // 131	/* #args + (#kwargs<<, // 8) */
	"MAKE_FUNCTION",          // 132	/* #defaults */
	"BUILD_SLICE",            // 133	/* Number of items */
	"MAKE_CLOSURE",           // 134  /* #free vars */
	"LOAD_CLOSURE",           // 135  /* Load free variable from closure */
	"LOAD_DEREF",             // 136  /* Load and dereference from closure cell */ 
	"STORE_DEREF",            // 137  /* Store into cell */ 
	"138__unknow",            // 138
	"139__unknow",            // 139

	/* The next , // 3 opcodes must be contiguous and satisfy
		 (CALL_FUNCTION_VAR - CALL_FUNCTION) &  == 1  */
	"CALL_FUNCTION_VAR",      // 140	/* #args + (#kwargs<<8) */
	"CALL_FUNCTION_KW",       // 141	/* #args + (#kwargs<<, // 8) */
	"CALL_FUNCTION_VAR_KW",   // 142	/* #args + (#kwargs<<, // 8) */
	/* Support for opargs more than 16 bits long */
	"EXTENDED_ARG",           // 143
};
