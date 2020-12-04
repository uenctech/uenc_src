#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdio.h>
#include <atomic>
#include <string>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>


struct noncopyable
{
protected:
	noncopyable() = default;
	virtual ~ noncopyable() = default;

	noncopyable(const noncopyable &) = delete;
	 noncopyable & operator=(const noncopyable &) = delete;
};

#ifdef NDEBUG
#define hlog(level, ...)                                                                \
    do {                                                                                \
        if (level <= ebpc::Logger::getLogger().getLogLevel()) {                               \
            ebpc::Logger::getLogger().logv(level, __FILE__, __LINE__, __func__, __VA_ARGS__); \
        }                                                                               \
    } while (0)
#else
#define hlog(level, ...)                                                                \
    do {                                                                                \
        if (level <= ebpc::Logger::getLogger().getLogLevel()) {                               \
            snprintf(0, 0, __VA_ARGS__);                                                \
            ebpc::Logger::getLogger().logv(level, __FILE__, __LINE__, __func__, __VA_ARGS__); \
        }                                                                               \
    } while (0)

#endif

#define trace(...) hlog(ebpc::Logger::LTRACE, __VA_ARGS__)
#define debug(...) hlog(ebpc::Logger::LDEBUG, __VA_ARGS__)
#define info(...) hlog(ebpc::Logger::LINFO, __VA_ARGS__)
#define warn(...) hlog(ebpc::Logger::LWARN, __VA_ARGS__)
#define error(...) hlog(ebpc::Logger::LERROR, __VA_ARGS__)
#define fatal(...) hlog(ebpc::Logger::LFATAL, __VA_ARGS__)
#define fatalif(b, ...)                        \
    do {                                       \
        if ((b)) {                             \
            hlog(ebpc::Logger::LFATAL, __VA_ARGS__); \
        }                                      \
    } while (0)
#define check(b, ...)                          \
    do {                                       \
        if ((b)) {                             \
            hlog(ebpc::Logger::LFATAL, __VA_ARGS__); \
        }                                      \
    } while (0)
#define exitif(b, ...)                         \
    do {                                       \
        if ((b)) {                             \
            hlog(ebpc::Logger::LERROR, __VA_ARGS__); \
            _exit(1);                          \
        }                                      \
    } while (0)

#define setloglevel(l) ebpc::Logger::getLogger().setLogLevel(l)
#define setlogfile(n) ebpc::Logger::getLogger().setFileName(n)

namespace ebpc
{

struct Logger:private noncopyable
{
	enum LogLevel
	{ LFATAL = 0, LERROR, LUERR, LWARN, LINFO, LDEBUG, LTRACE, LALL
	};
	 Logger();
	~Logger();
	void logv(int level, const char *file, int line, const char *func, const char *fmt ...);

	void setFileName(const std::string & filename);
	void setLogLevel(const std::string & level);
	void setLogLevel(LogLevel level)
	{
		level_ = std::min(LALL, std::max(LFATAL, level));
	} LogLevel getLogLevel()
	{
		return level_;
	}
	const char *getLogLevelStr()
	{
		return levelStrs_[level_];
	}
	int getFd()
	{
		return fd_;
	}

	void adjustLogLevel(int adjust)
	{
		setLogLevel(LogLevel(level_ + adjust));
	}
	void setRotateInterval(long rotateInterval)
	{
		rotateInterval_ = rotateInterval;
	}
	static Logger & getLogger();

private:
	void maybeRotate();
	static const char *levelStrs_[LALL + 1];
	int fd_;
	LogLevel level_;
	long lastRotate_;
	std::atomic < int64_t > realRotate_;
	long rotateInterval_;
	std::string filename_;
};

}

#endif//_LOGGING_H_