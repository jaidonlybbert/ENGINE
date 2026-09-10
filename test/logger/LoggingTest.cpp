#include <gtest/gtest.h>

#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

#include "Logging.hpp"

namespace {

// RAII helper that redirects std::cout into a stringstream for the lifetime of
// the object, then restores the original buffer.
class CoutCapture {
   public:
    CoutCapture() : original_(std::cout.rdbuf(buffer_.rdbuf())) {}
    ~CoutCapture() { std::cout.rdbuf(original_); }

    CoutCapture(const CoutCapture&) = delete;
    CoutCapture& operator=(const CoutCapture&) = delete;

    std::string str() const { return buffer_.str(); }

   private:
    std::ostringstream buffer_;
    std::streambuf* original_;
};

}  // namespace

TEST(LoggingTest, TagsHaveExpectedValues) {
    EXPECT_EQ(ENG_LOG_ERROR_TAG, "ENG_LOG_ERROR");
    EXPECT_EQ(ENG_LOG_INFO_TAG, "ENG_LOG_INFO");
    EXPECT_EQ(ENG_LOG_DEBUG_TAG, "ENG_LOG_DEBUG");
    EXPECT_EQ(ENG_LOG_TRACE_TAG, "ENG_LOG_TRACE");
}

TEST(LoggingTest, EngLogWritesTagThenMessage) {
    CoutCapture capture;
    ENG_LOG(ENG_LOG_ERROR_TAG, "boom");
    EXPECT_EQ(capture.str(), "ENG_LOG_ERROR: boom");
}

TEST(LoggingTest, ErrorAndInfoEmitAtDefaultLevel) {
    ASSERT_EQ(ENG_LOG_LEVEL, 3);

    {
        CoutCapture capture;
        ENG_LOG_ERROR("err");
        EXPECT_EQ(capture.str(), "ENG_LOG_ERROR: err");
    }
    {
        CoutCapture capture;
        ENG_LOG_INFO("info");
        EXPECT_EQ(capture.str(), "ENG_LOG_INFO: info");
    }
}

TEST(LoggingTest, DebugAndTraceSuppressedAtDefaultLevel) {
    {
        CoutCapture capture;
        ENG_LOG_DEBUG("dbg");
        EXPECT_EQ(capture.str(), "");
    }
    {
        CoutCapture capture;
        ENG_LOG_TRACE("trace");
        EXPECT_EQ(capture.str(), "");
    }
}
