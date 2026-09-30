/***************************************************************************
 *   Copyright © 2026 by Andrey Afletdinov <public.irkutsk@gmail.com>      *
 *                                                                         *
 *   https://github.com/AndreyBarmaley/sdbus-cpp-asio                      *
 *                                                                         *
 *   MIT License                                                           *
 *                                                                         *
 ***************************************************************************/

#ifndef _TEST_SYSLOG_
#define _TEST_SYSLOG_

#include <cstdio>

namespace Syslog {
    enum class DebugLevel { None, Info, Debug, Trace };

    template<typename... Values>
    static void info(const char* format, Values && ... vals) {
        fprintf(stderr, "[info] ");
        fprintf(stderr, format, vals...);
        fprintf(stderr, "\n");
    }

    template<typename... Values>
    static void error(const char* format, Values && ... vals) {
        fprintf(stderr, "[error] ");
        fprintf(stderr, format, vals...);
        fprintf(stderr, "\n");
    }

    template<typename... Values>
    static void debug(const char* format, Values && ... vals) {
        fprintf(stderr, "[debug] ");
        fprintf(stderr, format, vals...);
        fprintf(stderr, "\n");
    }
}

#endif // _TEST_SYSLOG_
