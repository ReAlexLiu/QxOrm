//
// Created by tux on 2020/4/29.
//

#include <xlogger/xlogger.h>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef WIN32
#include <spdlog/sinks/msvc_sink.h>
#endif // WIN32

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <iostream>

#ifdef DYNAMIC_LINK_LIBRARY
#include <dlfcn.h>
#endif

#if defined(WIN32) && defined(DYNAMIC_LINK_LIBRARY)
HMODULE GetSelfModuleHandle()
{
    MEMORY_BASIC_INFORMATION mbi;
    return ((::VirtualQuery(GetSelfModuleHandle, &mbi, sizeof(mbi)) != 0) ? (HMODULE)mbi.AllocationBase : NULL);
}
#endif

xlogger::xlogger()
    : mRunnable(false)
{}

xlogger& xlogger::getInstance()
{
    static xlogger _instance;
    return _instance;
}

void getCurrentPath(char* path, char* name)
{
    char currentPath[1024] = { 0 };
#ifdef WIN32
#ifdef DYNAMIC_LINK_LIBRARY
    GetModuleFileNameA(GetSelfModuleHandle(), currentPath, 1024);
#else
    GetModuleFileNameA(nullptr, currentPath, 1024);
#endif // DYNAMIC_LINK_LIBRARY
    char* p = strrchr(currentPath, '\\');
#else
#ifdef DYNAMIC_LINK_LIBRARY
    Dl_info dl_info;
    dladdr((void*)getCurrentPath, &dl_info);
    strcpy(currentPath, dl_info.dli_fname);
#else
    readlink("/proc/self/exe", currentPath, 1024);
#endif // DYNAMIC_LINK_LIBRARY
    char* p = strrchr(currentPath, '/');
#endif
    if (path)
    {
        memcpy(path, currentPath, p - currentPath + 1);
    }

    if (name)
    {
        char* n = strrchr(currentPath, '.');
        memcpy(name, p + 1, n ? n - p - 1 : strlen(p));
    }
}

bool xlogger::create()
{
    if (mRunnable)
    {
        return true;
    }

    char currentPath[260] = { 0 };
    char currentName[260] = { 0 };
    getCurrentPath(currentPath, currentName);
    char configFile[260] = { 0 };
    strcpy(configFile, currentPath);
#ifndef WIN32
    strcat(configFile, "conf/logs.conf");
#else
    strcat(configFile, "logs.conf");
#endif // !WIN32


    boost::property_tree::ptree lvptProperties;
    try
    {
        boost::property_tree::ini_parser::read_ini(configFile, lvptProperties);
    }
    catch (...)
    {

    }

    bool console = lvptProperties.get<bool>("xlogger.console", true);
    bool file = lvptProperties.get<bool>("xlogger.file", false);
    std::size_t max_size = lvptProperties.get<std::size_t>("xlogger.max_size", 1048576 * 6);
    std::size_t max_files = lvptProperties.get<std::size_t>("xlogger.max_files", 3);
#ifndef _DEBUG
    spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(lvptProperties.get<int>("xlogger.level", SPDLOG_LEVEL_ERROR));
#else
    spdlog::level::level_enum level = static_cast<spdlog::level::level_enum>(lvptProperties.get<int>("xlogger.level", SPDLOG_LEVEL_TRACE));
#endif
    std::chrono::seconds interval = std::chrono::seconds(lvptProperties.get<int>("xlogger.interval", 3));


    std::vector<spdlog::sink_ptr> sinkList;

    if (console)
    {
        auto stdoutColorSink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
        stdoutColorSink->set_level(level);
        stdoutColorSink->set_pattern("%^[%L][%m-%d %H:%M:%S.%e][%!,%@][%t] %v%$");
        sinkList.push_back(stdoutColorSink);
#ifdef WIN32
        auto msvcColorSink = std::make_shared<spdlog::sinks::windebug_sink_st>();
        msvcColorSink->set_level(level);
        msvcColorSink->set_pattern("%^[%L][%m-%d %H:%M:%S.%e][%!,%@][%t] %v%$");
        sinkList.push_back(msvcColorSink);
#endif // WIN32


    }

    if (file)
    {
        char logsFile[260] = { 0 };
        strcpy(logsFile, currentPath);
        strcat(logsFile, "logs/");
        strcat(logsFile, currentName);
        strcat(logsFile, ".log");

        auto rotatingFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(logsFile, max_size, max_files);
        rotatingFileSink->set_level(level);
        rotatingFileSink->set_pattern("[%L][%Y-%m-%d %H:%M:%S.%e][%!,%@][%t] %v");
        sinkList.push_back(rotatingFileSink);
    }

    auto logger = std::make_shared<spdlog::logger>("both", begin(sinkList), end(sinkList));
    spdlog::register_logger(logger);

    logger->flush_on(spdlog::level::err);

    // std::chrono::seconds(seconds)
    spdlog::flush_every(interval);
    spdlog::get("both")->set_level(level);
    mRunnable = true;

    return true;
}

void xlogger::destroy()
{
    spdlog::drop_all();
}
