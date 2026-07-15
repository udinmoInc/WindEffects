#pragma once

#include <chrono>
#include <cstdio>

#ifndef WE_ENABLE_AGENT_DEBUG_LOG
#define WE_ENABLE_AGENT_DEBUG_LOG 0
#endif

inline void AgentDebugLog(
    const char* hypothesisId,
    const char* location,
    const char* message,
    const char* dataJson)
{
#if WE_ENABLE_AGENT_DEBUG_LOG
    using namespace std::chrono;
    const auto now = time_point_cast<milliseconds>(steady_clock::now()).time_since_epoch().count();

    std::FILE* f = std::fopen("f:/Coding/windeffects/debug-a0cfa5.log", "a");
    if (!f) {
        return;
    }

    std::fprintf(
        f,
        "{\"sessionId\":\"a0cfa5\",\"id\":\"log_%lld\",\"timestamp\":%lld,"
        "\"runId\":\"pre-fix\",\"hypothesisId\":\"%s\",\"location\":\"%s\","
        "\"message\":\"%s\",\"data\":%s}\n",
        static_cast<long long>(now),
        static_cast<long long>(now),
        hypothesisId,
        location,
        message,
        dataJson ? dataJson : "null");

    std::fclose(f);
#else
    (void)hypothesisId;
    (void)location;
    (void)message;
    (void)dataJson;
#endif
}
