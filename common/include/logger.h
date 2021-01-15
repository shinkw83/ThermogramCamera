#ifndef         __LOGGER_H__
#define         __LOGGER_H__


#include <log4cplus/logger.h>
#include <log4cplus/layout.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/loggingmacros.h>

using namespace log4cplus;

#define		MAX_FILE_SIZE			(1024*1024*1024 / 10)

class LogMgrC{
	SharedAppenderPtr myAppender;
	Logger logger;

	public:
	LogMgrC(const char *path, const char *log_name=0, int filecount = 10, const LogLevel log_level=DEBUG_LOG_LEVEL)
		: myAppender(new RollingFileAppender(path, MAX_FILE_SIZE, 100, true))
		  , logger(log_name?Logger::getInstance(log_name):Logger::getRoot())
	{
		std::auto_ptr<Layout> myLayout
			= std::auto_ptr<Layout>(new log4cplus::PatternLayout("%D{%m/%d %H:%M:%S:%q } %-5p %m%n"));

		myAppender->setLayout(myLayout);
		logger.addAppender(myAppender);
		logger.setLogLevel(log_level);
	}

	void setLogLevel(const LogLevel log_level)
	{
		logger.setLogLevel(log_level);
	}

	~LogMgrC()
	{
		logger.removeAppender(myAppender);
	}

	Logger &get() { return logger; }
};


class DebugLogger{

	SharedAppenderPtr myAppender;
	Logger logger;

	public:
	DebugLogger(const char *path, const char *log_name=0, int filecount = 10)//, const LogLevel log_level=DEBUG_LOG_LEVEL)
		: myAppender(new RollingFileAppender(path, 1024*1024*100, 50, true))
		  , logger(log_name?Logger::getInstance(log_name):Logger::getRoot())
	{
		std::auto_ptr<Layout> myLayout
			= std::auto_ptr<Layout>(new log4cplus::PatternLayout("%D{%m/%d %H:%M:%S:%q } %m%n"));

		myAppender->setLayout(myLayout);
		logger.addAppender(myAppender);
		logger.setLogLevel(DEBUG_LOG_LEVEL);
	}

	void setLogLevel(const LogLevel log_level)
	{
		logger.setLogLevel(log_level);
	}


	~DebugLogger()
	{
		logger.removeAppender(myAppender);
	}

	Logger &get() { return logger; }
};


#endif
