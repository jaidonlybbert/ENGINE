#pragma once
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

static constexpr int ENG_LOG_LEVEL{3};

#define ENG_LOG_AT(spdlog_level, msg)                       \
    do {                                                    \
        std::ostringstream _eng_log_stream;                 \
        _eng_log_stream << msg;                             \
        ::spdlog::log(spdlog_level, _eng_log_stream.str()); \
    } while (0)

#define ENG_LOG_ERROR(msg)                         \
    do {                                           \
        if (ENG_LOG_LEVEL > 0) {                   \
            ENG_LOG_AT(::spdlog::level::err, msg); \
        }                                          \
    } while (0)

#define ENG_LOG_INFO(msg)                           \
    do {                                            \
        if (ENG_LOG_LEVEL > 2) {                    \
            ENG_LOG_AT(::spdlog::level::info, msg); \
        }                                           \
    } while (0)

#define ENG_LOG_DEBUG(msg)                           \
    do {                                             \
        if (ENG_LOG_LEVEL > 4) {                     \
            ENG_LOG_AT(::spdlog::level::debug, msg); \
        }                                            \
    } while (0)

#define ENG_LOG_TRACE(msg)                           \
    do {                                             \
        if (ENG_LOG_LEVEL > 5) {                     \
            ENG_LOG_AT(::spdlog::level::trace, msg); \
        }                                            \
    } while (0)