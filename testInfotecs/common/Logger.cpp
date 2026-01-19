#include "Logger.h"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_ostream.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <fstream>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

/**
 * @brief Инициализирует Boost.Log для записи логов в консоль и файл.
 *
 * Логи будут выводиться в консоль и в файл "integration_log.log".
 * Уровень логирования для консоли установлен в INFO, для файла - в TRACE.
 */
void init_logging() {
    // Настройка вывода в консоль
    logging::add_console_log(
        std::cout,
        keywords::format = expr::stream
            << expr::attr< boost::log::attributes::current_thread_id::value_type >("ThreadID") << " "
            << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") << " ["
            << logging::trivial::severity << "]: " << expr::smessage,
        keywords::filter = logging::trivial::severity >= logging::trivial::info
    );

    // Настройка вывода в файл
    logging::add_file_log(
        keywords::file_name = "integration_log.log",
        keywords::rotation_size = 10 * 1024 * 1024, // 10 MB
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(boost::posix_time::hours(24)),
        keywords::format = expr::stream
            << expr::attr< boost::log::attributes::current_thread_id::value_type >("ThreadID") << " "
            << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") << " ["
            << logging::trivial::severity << "]: " << expr::smessage,
        keywords::filter = logging::trivial::severity >= logging::trivial::trace
    );

    logging::add_common_attributes();
}
