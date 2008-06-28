//===----------------- debug.h - Debug facilities -------------------------===//
//
//                               Mvm
//
// This file is distributed under the University of Illinois Open Source 
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef DEBUG_H
#define DEBUG_H

#define ESC "\033["
#define COLOR_NORMAL ""
#define END "m"

#define _BLACK "00"
#define _RED "01"
#define _GREEN "02"
#define _YELLOW "03"
#define _BLUE "04"
#define _MAGENTA "05"
#define _CYAN "06"
#define _WHITE "07"

#define _NORMAL "00"
#define _BOLD "01"
#define _SOULIGNE "04"

#define MK_COLOR(type, bg, fg) type";"bg";"fg

#define WHITE MK_COLOR(_NORMAL, _BLACK, _BLACK)

#define DARK_MAGENTA MK_COLOR(_NORMAL, _BLACK, _MAGENTA)
#define DARK_YELLOW MK_COLOR(_NORMAL, _BLACK, _YELLOW)
#define DARK_CYAN MK_COLOR(_NORMAL, _BLACK, _CYAN)
#define DARK_BLUE MK_COLOR(_NORMAL, _BLACK, _BLUE)
#define DARK_GREEN MK_COLOR(_NORMAL, _BLACK, _GREEN)

#define LIGHT_MAGENTA MK_COLOR(_BOLD, _BLACK, _MAGENTA)
#define LIGHT_YELLOW MK_COLOR(_BOLD, _BLACK, _YELLOW)
#define LIGHT_CYAN MK_COLOR(_BOLD, _BLACK, _CYAN)
#define LIGHT_BLUE MK_COLOR(_BOLD, _BLACK, _BLUE)
#define LIGHT_GREEN MK_COLOR(_BOLD, _BLACK, _GREEN)
#define LIGHT_RED MK_COLOR(_BOLD, _BLACK, _RED)

#if DEBUG > 0

  #ifdef WITH_COLOR
    #define PRINT_DEBUG(symb, level, color, ...) \
      if (symb > level) { \
          printf("%s%s%s", ESC, color, END); \
          printf(__VA_ARGS__); \
          printf("%s%s%s", ESC, COLOR_NORMAL, END); \
          fflush(stdout); \
      }
  #else
    #define PRINT_DEBUG(symb, level, color, ...) \
      if (symb > level) { \
        printf(__VA_ARGS__); \
        fflush(stdout); \
      }
  #endif

#else
#define PRINT_DEBUG(symb, level, color, args...) do {} while(0);
#endif


#endif
