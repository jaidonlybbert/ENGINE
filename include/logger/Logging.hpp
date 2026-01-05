#pragma once
#include<string_view>
#include<iostream>
static constexpr int ENG_LOG_LEVEL{3};
static constexpr std::string_view ENG_LOG_ERROR_TAG{"ENG_LOG_ERROR"};
static constexpr std::string_view ENG_LOG_INFO_TAG{"ENG_LOG_INFO"};
static constexpr std::string_view ENG_LOG_DEBUG_TAG{"ENG_LOG_DEBUG"};
static constexpr std::string_view ENG_LOG_TRACE_TAG{"ENG_LOG_TRACE"};

#define ENG_LOG(tag, msg) std::cout <<  tag << ": " << msg
#define ENG_LOG_ERROR(msg) if (ENG_LOG_LEVEL > 0) { ENG_LOG(ENG_LOG_ERROR_TAG, msg); } else { (void)0; }
#define ENG_LOG_INFO(msg) if (ENG_LOG_LEVEL > 2) { ENG_LOG(ENG_LOG_INFO_TAG, msg); } else { (void)0; }
#define ENG_LOG_DEBUG(msg) if (ENG_LOG_LEVEL > 4) { ENG_LOG(ENG_LOG_DEBUG_TAG, msg); } else { (void)0; }
#define ENG_LOG_TRACE(msg) if (ENG_LOG_LEVEL > 5) { ENG_LOG(ENG_LOG_TRACE_TAG, msg); } else { (void)0; }
