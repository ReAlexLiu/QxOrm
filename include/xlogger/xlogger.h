//
// Created by tux on 2020/4/29.
//

#ifndef EDGE_XLOGGER_H
#define EDGE_XLOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

#ifdef _WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1):__FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#endif

class xlogger
{
private:
    xlogger();

    ~xlogger() = default;

public:
    xlogger(const xlogger&) = delete;

    xlogger& operator=(const xlogger&) = delete;

    static xlogger& getInstance();

    void destroy();

    //void getCurrentPath(char* path, char* name = nullptr);

    template<typename... Args>
    void _xlogger_impl(spdlog::level::level_enum lvl, const char* filename, const char* funcname, int line, spdlog::string_view_t fmt, const Args &... args)
    {
        if (create())
        {
            spdlog::source_loc loc(filename, line, funcname);
            spdlog::get("both")->log(loc, lvl, fmt, args...);
        }
    }

private:
    bool create();

private:
    bool mRunnable;
};

#define xverbose(...) xlogger::getInstance()._xlogger_impl(spdlog::level::trace, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define xdebug(...) xlogger::getInstance()._xlogger_impl(spdlog::level::debug, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define xinfo(...) xlogger::getInstance()._xlogger_impl(spdlog::level::info, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define xwarn(...) xlogger::getInstance()._xlogger_impl(spdlog::level::warn, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define xerror(...) xlogger::getInstance()._xlogger_impl(spdlog::level::err, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define xfatal(...) xlogger::getInstance()._xlogger_impl(spdlog::level::critical, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)



#define xverbose_if(exp, ...) if (exp) { xlogger::getInstance()._xlogger_impl(spdlog::level::trace, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__);}
#define xdebug_if(exp, ...) if (exp) { xlogger::getInstance()._xlogger_impl(spdlog::level::debug, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__);}
#define xinfo_if(exp, ...) if (exp) { xlogger::getInstance()._xlogger_impl(spdlog::level::info, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__);}
#define xwarn_if(exp, ...) if (exp) { xlogger::getInstance()._xlogger_impl(spdlog::level::warn, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__);}
#define xerror_if(exp, ...) if (exp) { xlogger::getInstance()._xlogger_impl(spdlog::level::err, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__);}
#define xfatal_if(exp, ...) if (exp) { xlogger::getInstance()._xlogger_impl(spdlog::level::critical, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__);}

#define xassert(exp, ...) if (!exp) { xlogger::getInstance()._xlogger_impl(spdlog::level::warn, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__); qAssert(false); }

#endif //EDGE_XLOGGER_H
