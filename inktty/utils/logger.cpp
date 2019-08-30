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

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>

#include <inktty/utils/ansi_terminal_writer.hpp>
#include <inktty/utils/logger.hpp>

namespace inktty {
/******************************************************************************
 * Static helpers                                                             *
 ******************************************************************************/

#ifdef NDEBUG
static constexpr LogSeverity DEFAULT_LOG_LEVEL = LogSeverity::INFO;
#else
static constexpr LogSeverity DEFAULT_LOG_LEVEL = LogSeverity::DEBUG;
#endif


/******************************************************************************
 * Class LogStreamBackend::Impl                                               *
 ******************************************************************************/

class LogStreamBackend::Impl {
private:
	std::ostream &m_os;
	Terminal m_terminal;

public:
	Impl(std::ostream &os, bool use_color) : m_os(os), m_terminal(use_color) {}

	std::ostream *log(LogSeverity lvl, std::time_t time, const char *module)
	{
		{
			char time_str[41];
			std::strftime(time_str, 40, "%Y-%m-%d %H:%M:%S",
			              std::localtime(&time));
			m_os << m_terminal.italic() << time_str << m_terminal.reset();
		}

		if (module) {
			m_os << " [" << module << "] ";
		} else {
			m_os << " ";
		}

		if (lvl <= LogSeverity::DEBUG) {
			m_os << m_terminal.color(Terminal::CYAN, true)  << "debug";
		} else if (lvl <= LogSeverity::INFO) {
			m_os << m_terminal.color(Terminal::BLUE, true) << "info";
		} else if (lvl <= LogSeverity::WARNING) {
			m_os << m_terminal.color(Terminal::MAGENTA, true) << "warning";
		} else if (lvl <= LogSeverity::ERROR) {
			m_os << m_terminal.color(Terminal::RED, true) << "error";
		} else {
			m_os << m_terminal.color(Terminal::RED, true) << "fatal error";
		}

		m_os << m_terminal.reset() << ": ";

		return &m_os;
	}
};

/******************************************************************************
 * Class LogStreamBackend                                                     *
 ******************************************************************************/

LogStreamBackend::LogStreamBackend(std::ostream &os, bool use_color)
    : m_impl(new Impl(os, use_color))
{
}

std::ostream *LogStreamBackend::log(LogSeverity lvl, std::time_t time,
                                    const char *module)
{
	return m_impl->log(lvl, time, module);
}

LogStreamBackend::~LogStreamBackend()
{
	// Do nothing here, just required for the unique_ptr destructor
}

/******************************************************************************
 * Class Logger::Impl                                                         *
 ******************************************************************************/

class Logger::Impl {
private:
	std::vector<std::tuple<std::shared_ptr<LogBackend>, LogSeverity>>
	    m_backends;
	std::mutex m_logger_mtx;
	std::map<LogSeverity, size_t> m_counts;

	size_t backend_idx(int idx) const
	{
		idx = (idx < 0) ? int(m_backends.size()) + idx : idx;
		if (idx < 0 || size_t(idx) >= m_backends.size()) {
			throw std::invalid_argument("Given index out of range");
		}
		return idx;
	}

public:
	size_t backend_count() const { return m_backends.size(); }
	int add_backend(std::shared_ptr<LogBackend> backend, LogSeverity lvl)
	{
		std::lock_guard<std::mutex> lock(m_logger_mtx);
		m_backends.emplace_back(std::move(backend), lvl);
		return m_backends.size() - 1;
	}

	void min_level(LogSeverity lvl, int idx)
	{
		std::get<1>(m_backends[backend_idx(idx)]) = lvl;
	}

	LogSeverity min_level(int idx)
	{
		return std::get<1>(m_backends[backend_idx(idx)]);
	}

	StreamProxy log(LogSeverity lvl, std::time_t time, const char *module)
	{
		// Create the resulting StreamProxy instance and pass the logger mutex
		// lock. The StreamProxy will act as a lock guard for the mutex.
		StreamProxy proxy(&m_logger_mtx);

		// Update the statistics
		auto it = m_counts.find(lvl);
		if (it != m_counts.end()) {
			it->second++;
		} else {
			m_counts.emplace(lvl, 1);
		}

		// Actually issue the elements
		for (auto &backend : m_backends) {
			if (lvl >= std::get<1>(backend)) {
				proxy.add_stream(std::get<0>(backend)->log(lvl, time, module));
			}
		}

		return proxy;
	}

	void log(LogSeverity lvl, std::time_t time, const char *module,
	         const std::string &message)
	{
		log(lvl, time, module) << message;
	}

	StreamProxy log(LogSeverity lvl, const char *module)
	{
		return log(lvl, time(nullptr), module);
	}

	void log(LogSeverity lvl, const char *module, const std::string &message)
	{
		log(lvl, time(nullptr), module, message);
	}

	size_t count(LogSeverity lvl) const
	{
		size_t res = 0;
		auto it = m_counts.lower_bound(lvl);
		for (; it != m_counts.end(); it++) {
			res += it->second;
		}
		return res;
	}
};

/******************************************************************************
 * Class Logger                                                               *
 ******************************************************************************/

Logger::Logger() : m_impl(new Impl()) {}

Logger::Logger(std::shared_ptr<LogBackend> backend, LogSeverity lvl) : Logger()
{
	add_backend(std::move(backend), lvl);
}

size_t Logger::backend_count() const { return m_impl->backend_count(); }

size_t Logger::count(LogSeverity lvl) const { return m_impl->count(lvl); }

int Logger::add_backend(std::shared_ptr<LogBackend> backend, LogSeverity lvl)
{
	return m_impl->add_backend(backend, lvl);
}

void Logger::min_level(LogSeverity lvl, int idx)
{
	return m_impl->min_level(lvl, idx);
}

LogSeverity Logger::min_level(int idx) { return m_impl->min_level(idx); }

void Logger::log(LogSeverity lvl, std::time_t time, const char *module,
                 const std::string &message)
{
	m_impl->log(lvl, time, module, message);
}

Logger::StreamProxy Logger::log(LogSeverity lvl, std::time_t time,
                                const char *module)
{
	return m_impl->log(lvl, time, module);
}

void Logger::debug(const char *module, const std::string &message)
{
	m_impl->log(LogSeverity::DEBUG, module, message);
}

Logger::StreamProxy Logger::debug(const char *module)
{
	return m_impl->log(LogSeverity::DEBUG, module);
}

void Logger::info(const char *module, const std::string &message)
{
	m_impl->log(LogSeverity::INFO, module, message);
}

Logger::StreamProxy Logger::info(const char *module)
{
	return m_impl->log(LogSeverity::INFO, module);
}

void Logger::warn(const char *module, const std::string &message)
{
	m_impl->log(LogSeverity::WARNING, module, message);
}

Logger::StreamProxy Logger::warn(const char *module)
{
	return m_impl->log(LogSeverity::WARNING, module);
}

void Logger::error(const char *module, const std::string &message)
{
	m_impl->log(LogSeverity::ERROR, module, message);
}

Logger::StreamProxy Logger::error(const char *module)
{
	return m_impl->log(LogSeverity::ERROR, module);
}

void Logger::fatal_error(const char *module, const std::string &message)
{
	m_impl->log(LogSeverity::FATAL_ERROR, module, message);
}

Logger::StreamProxy Logger::fatal_error(const char *module)
{
	return m_impl->log(LogSeverity::FATAL_ERROR, module);
}

/******************************************************************************
 * Functions                                                                  *
 ******************************************************************************/

Logger &global_logger()
{
	static std::mutex global_logger_mtx;
	static Logger logger;

	// Add the default backends to the logger if this has not been done yet
	std::lock_guard<std::mutex> lock(global_logger_mtx);
	if (logger.backend_count() == 0) {
		logger.add_backend(std::make_shared<LogStreamBackend>(
		                       std::cerr, isatty(STDERR_FILENO)),
		                   DEFAULT_LOG_LEVEL);
	}
	return logger;
}
}
