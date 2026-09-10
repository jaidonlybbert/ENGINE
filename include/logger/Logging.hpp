#pragma once
#include <sstream>
#include <string>

#include "spdlog/spdlog.h"

// Compile-time verbosity gate. Disabled levels expand to nothing, so they cost
// nothing at runtime. Runtime filtering/sinks/formatting are handled by spdlog.
//   > 0 : errors
//   > 2 : info
//   > 4 : debug
//   > 5 : trace
static constexpr int ENG_LOG_LEVEL{3};

namespace ENG::log {

// Configure the default spdlog logger. Safe to call more than once. Call early in
// main(); if it is never called, spdlog's own defaults apply.
inline void init() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    // Leave runtime filtering to the compile-time ENG_LOG_LEVEL gate below.
    spdlog::set_level(spdlog::level::trace);
}

namespace detail {
// Collapse a streamed expression to a string, trimming one trailing newline so the
// many existing call sites that end with `<< std::endl` don't emit blank lines
// (spdlog appends its own line ending).
inline std::string to_message(const std::ostringstream& stream) {
    std::string message = stream.str();
    if (!message.empty() && message.back() == '\n') {
        message.pop_back();
    }
    return message;
}
}  // namespace detail

}  // namespace ENG::log

#define ENG_LOG_AT(spdlog_level, msg)                                                         \
    do {                                                                                      \
        std::ostringstream _eng_log_stream;                                                   \
        _eng_log_stream << msg;                                                               \
        ::spdlog::log((spdlog_level), "{}", ::ENG::log::detail::to_message(_eng_log_stream)); \
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
