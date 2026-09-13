#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>

#include "Logging.hpp"
#include "spdlog/sinks/ostream_sink.h"
#include "spdlog/spdlog.h"

namespace {

// spdlog appends its own line ending after every formatted message, and that ending is
// platform-native (see spdlog/details/os.h's SPDLOG_EOL) rather than always "\n".
#if defined(_WIN32)
constexpr const char* kSpdlogEol = "\r\n";
#else
constexpr const char* kSpdlogEol = "\n";
#endif

// Redirects the spdlog default logger into an in-memory stream for the lifetime of
// the object, then restores the previous default logger. The pattern is reduced to
// the bare message so assertions only see what the call site logged.
class LogCapture {
   public:
    LogCapture() : previous_(spdlog::default_logger()) {
        auto sink = std::make_shared<spdlog::sinks::ostream_sink_st>(stream_);
        auto logger = std::make_shared<spdlog::logger>("test", sink);
        logger->set_pattern("%v");
        logger->set_level(spdlog::level::trace);
        spdlog::set_default_logger(logger);
    }
    ~LogCapture() { spdlog::set_default_logger(previous_); }

    LogCapture(const LogCapture&) = delete;
    LogCapture& operator=(const LogCapture&) = delete;

    std::string str() const { return stream_.str(); }

   private:
    std::ostringstream stream_;
    std::shared_ptr<spdlog::logger> previous_;
};

}  // namespace

TEST(LoggingTest, ErrorAndInfoEmitAtDefaultLevel) {
    ASSERT_EQ(ENG_LOG_LEVEL, 3);

    {
        LogCapture capture;
        ENG_LOG_ERROR("err");
        EXPECT_EQ(capture.str(), std::string("err") + kSpdlogEol);
    }
    {
        LogCapture capture;
        ENG_LOG_INFO("info");
        EXPECT_EQ(capture.str(), std::string("info") + kSpdlogEol);
    }
}

TEST(LoggingTest, DebugAndTraceSuppressedAtDefaultLevel) {
    {
        LogCapture capture;
        ENG_LOG_DEBUG("dbg");
        EXPECT_EQ(capture.str(), "");
    }
    {
        LogCapture capture;
        ENG_LOG_TRACE("trace");
        EXPECT_EQ(capture.str(), "");
    }
}

TEST(LoggingTest, StreamingSyntaxIsConcatenated) {
    LogCapture capture;
    const int line = 42;
    ENG_LOG_ERROR("file:" << line << " boom");
    EXPECT_EQ(capture.str(), std::string("file:42 boom") + kSpdlogEol);
}

TEST(LoggingTest, EndlIsPassedThroughVerbatim) {
    LogCapture capture;
    ENG_LOG_ERROR("msg" << std::endl);
    // std::endl always writes a literal '\n' (no platform translation on an
    // ostringstream), regardless of what spdlog's own trailing line ending is. The logged
    // message is used as-is, so a call site's own std::endl produces one blank line in
    // addition to spdlog's own line ending.
    EXPECT_EQ(capture.str(), std::string("msg\n") + kSpdlogEol);
}
