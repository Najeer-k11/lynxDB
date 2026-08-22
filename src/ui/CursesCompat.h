#ifndef DBTERM_CURSES_COMPAT_H
#define DBTERM_CURSES_COMPAT_H

#if defined(__has_include)
  #if __has_include(<ncursesw/ncurses.h>)
    #include <ncursesw/ncurses.h>
  #elif __has_include(<ncurses/ncurses.h>)
    #include <ncurses/ncurses.h>
  #elif __has_include(<ncurses.h>)
    #include <ncurses.h>
  #elif __has_include(<curses.h>)
    #include <curses.h>
  #else
    #error "Neither <ncursesw/ncurses.h>, <ncurses/ncurses.h>, nor <ncurses.h> found."
  #endif
#else
  #include <ncurses.h>
#endif

#endif // DBTERM_CURSES_COMPAT_H
