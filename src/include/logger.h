#pragma once

#include "lockqueue.h"

#include <string>

enum LogLevel
{
    INFO,   //普通信息
    ERROR,  //错误信息
};
//Mprpc框架的日志系统
class Logger
{
public:
    //获取日志的单例
    static Logger& GetInstance();

    //写日志
    void Log(LogLevel level,std::string msg);

private:
    LockQueue<std::string> m_lckQue;    //日志缓冲队列

    Logger();
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
};

//定义宏
#define LOG_INFO(logmsgformat,...) \
    do \
    { \
        Logger &logger = Logger::GetInstance(); \
        char c[1024] = {0}; \
        snprintf(c,1024,logmsgformat,##__VA_ARGS__); \
        logger.Log(INFO,c); \
    } while (0);

#define LOG_ERROR(logmsgformat,...) \
    do \
    { \
        Logger &logger = Logger::GetInstance(); \
        char c[1024] = {0}; \
        snprintf(c,1024,logmsgformat,##__VA_ARGS__); \
        logger.Log(ERROR,c); \
    } while (0);
