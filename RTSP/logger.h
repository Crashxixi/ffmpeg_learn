//
// Created by 胡启奇 on 2022/10/31.
//

#ifndef RTSP_LOGGER_H
#define RTSP_LOGGER_H
#include <sys/time.h>

enum LOG_LEVEL : int {
    TRACE = 1,
    DEBUG = 2,
    INFO = 3,
    WARN = 4,
    ERROR = 5
};

static void getCurTime(int size, char szTime[]) {
    time_t tt = {0};
    struct tm *curr_time = nullptr;

    time(&tt);
    curr_time = localtime(&tt);
    snprintf(szTime, size - 1, "%04d/%02d/%02d %02d:%02d:%02d",
             curr_time->tm_year + 1900, curr_time->tm_mon + 1, curr_time->tm_mday,
             curr_time->tm_hour, curr_time->tm_min, curr_time->tm_sec);
}

static int64_t getTimeMillisecond() {
    timeval tv;
    gettimeofday(&tv, nullptr);
    return ((int64_t) tv.tv_sec * 1000 + (unsigned long long) tv.tv_usec / 1000);
}

static char *getLogLevel(LOG_LEVEL level) {
    switch (level) {
        case TRACE:
            return (char *) "TRACE";
        case DEBUG:
            return (char *) "DEBUG";
        case INFO:
            return (char *) "INFO";
        case WARN:
            return (char *) "WARN";
        case ERROR:
            return (char *) "ERROR";
        default:
            return (char *) "Default";
    }
}

static void WriteLog(LOG_LEVEL level, const char *func_name, int line, const char *fmt, ...) {
    va_list args;
    char *logLevel = nullptr;
    char szTime[20] = {0};
    char szLine[4096] = {0};
    char szContent[4096] = {0};

    va_start(args, fmt);
    vsnprintf(szContent, sizeof(szContent) - 1, fmt, args);
    va_end(args);
    getCurTime(sizeof(szTime), szTime);
    logLevel = getLogLevel(level);
    int64_t curTime = getTimeMillisecond();
    snprintf(szLine, sizeof(szLine) - 1, "[%s %s-%d %s:%d] %s\n",
             logLevel, szTime, int(curTime % 1000), func_name, line, szContent);
    fputs(szLine, stdout);
    fflush(stdout);
}

#define LogError(fmt, ...) WriteLog(ERROR, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LogWarn(fmt, ...) WriteLog(WARN, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LogInfo(fmt, ...) WriteLog(INFO, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LogDebug(fmt, ...) WriteLog(DEBUG, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LogTrace(fmt, ...) WriteLog(TRACE, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#endif//RTSP_LOGGER_H
