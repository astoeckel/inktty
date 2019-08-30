/*
 *  inktty -- Terminal emulator optimized for epaper displays
 *  Copyright (C) 2018, 2019  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file logger.hpp
 *
 * Implements a simple logging system.
 *
 * @author Andreas Stöckel
 */

#ifndef INKTTY_UTILS_LOGGER_HPP
#define INKTTY_UTILS_LOGGER_HPP

#include <cstdint>
#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace inktty {
/*
 * Forward declarations.
 */
class Logger;

/**
 * The LogSeverity enum holds the LogSeverity of a log message. Higher
 * severities are associated with higher values.
 */
enum LogSeverity : int32_t {
	DEBUG = 10,
	INFO = 20,
	WARNING = 30,
	ERROR = 40,
	FATAL_ERROR = 50
};

/**
 * The Backend class is the abstract base class that must be implemented by
 * any log backend.
 */
class LogBackend {
public:
	/**
	 * Pure virtual function which is called whenever a new log message should
	 * be logged.
	 */
	virtual std::ostream* log(LogSeverity lvl, std::time_t time,
	                 const char *module) = 0;
	virtual ~LogBackend() {}
};

/**
 * Implementation of the LogBackend class which loggs to the given output
 * stream.
 */
class LogStreamBackend : public LogBackend {
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	LogStreamBackend(std::ostream &os, bool use_color = false);

	std::ostream* log(LogSeverity lvl, std::time_t time, const char *module) override;
	~LogStreamBackend() override;
};

/**
 * The Logger class is the frontend class that should be used to log messages.
 * A global instance which logs to std::cerr can be accessed using the
 * global_logger() function.
 */
class Logger {
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;

public:
	/**
	 * The StreamProxy class encapsulates a pointer to an output stream as well
	 * as a mutex lock. Instances of this object can be moved around, and the
	 * instance that is destroyed last will add an endline to the output stream
	 * and release the mutex.
	 */
	class StreamProxy {
	private:
		std::vector<std::ostream*> m_os_list;
		std::mutex *m_mtx;
	public:
		StreamProxy(std::mutex *mtx): m_mtx(mtx) {
			m_mtx->lock();
		}

		~StreamProxy() {
			for (std::ostream *os: m_os_list) {
				(*os) << std::endl;
			}
			if (m_mtx) {
				m_mtx->unlock();
			}
		}

		StreamProxy(const StreamProxy&) = delete;
		StreamProxy& operator=(const StreamProxy &) = delete;

		void add_stream(std::ostream *os) {
			m_os_list.push_back(os);
		}

		StreamProxy(StreamProxy &&o) {
			m_os_list = std::move(o.m_os_list);
			m_mtx = o.m_mtx;
			o.m_mtx = nullptr;
		}

		StreamProxy& operator=(StreamProxy &&o) {
			m_os_list = std::move(o.m_os_list);
			m_mtx = o.m_mtx;
			o.m_mtx = nullptr;
			return *this;
		}

		template<typename T>
		friend StreamProxy operator <<(StreamProxy proxy, T const& value) {
			for (std::ostream *os: proxy.m_os_list) {
				(*os) << value;
			}
			return proxy;
		}
	};

	/**
	 * Default constructor, adds no backend.
	 */
	Logger();

	/**
	 * Creates a new Logger instance which logs into the given backend.
	 */
	Logger(std::shared_ptr<LogBackend> backend,
	       LogSeverity lvl = LogSeverity::INFO);

	/**
	 * Returns the number of attached backends.
	 */
	size_t backend_count() const;

	/**
	 * Adds another logger backend with the given log level and returns the
	 * index of the newly added backend.
	 */
	int add_backend(std::shared_ptr<LogBackend> backend,
	                LogSeverity lvl = LogSeverity::INFO);

	/**
	 * Sets the minimum level for the backend with the given index. Negative
	 * indices allow to access the backend list in reverse order. Per default
	 * the log level of the last added backend is updated.
	 */
	void min_level(LogSeverity lvl, int idx = -1);

	/**
	 * Returns the log level for the backend with the given index. Negative
	 * indices allow to access the backend list in reverse order.
	 */
	LogSeverity min_level(int idx = -1);

	/**
	 * Returns the number of messages that have been captured with at least the
	 * given level.
	 */
	size_t count(LogSeverity lvl = LogSeverity::DEBUG) const;

	void log(LogSeverity lvl, std::time_t time, const char *module,
	         const std::string &message);
	StreamProxy log(LogSeverity lvl, std::time_t time, const char *module = nullptr);

	void debug(const char *module, const std::string &message);
	StreamProxy debug(const char *module = nullptr);

	void info(const char *module, const std::string &message);
	StreamProxy info(const char*module = nullptr);

	void warn(const char *module,const std::string &message);
	StreamProxy warn(const char *module = nullptr);

	void error(const char *module, const std::string &message);
	StreamProxy error(const char *module = nullptr);

	void fatal_error(const char *module, const std::string &message);
	StreamProxy fatal_error(const char *module = nullptr);
};

Logger &global_logger();
}

#endif /* INKTTY_UTILS_LOGGER_HPP */
