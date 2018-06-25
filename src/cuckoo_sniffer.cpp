#include "cuckoo_sniffer.hpp"

#include <functional>

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/record_ordering.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/filesystem.hpp>

#include <boost/system/error_code.hpp>

#include "threads/data_queue.hpp"


namespace cs {

    using namespace std;

    std::map<std::string, std::string> CONFIG;

    boost::log::sources::severity_logger <boost::log::trivial::severity_level> lg;


    void init_log_in_thread() {
        BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());
    }

    void init_log() {

        boost::system::error_code ec;
        boost::filesystem::create_directories("/var/log/imcs", ec);
        cout << ec.message() << endl;

        boost::shared_ptr <boost::log::core> core = boost::log::core::get();

        boost::shared_ptr <boost::log::sinks::text_file_backend> backend =
                boost::make_shared<boost::log::sinks::text_file_backend>(
                        boost::log::keywords::file_name = "/var/log/imcs/imcs_sniffer_%Y%m%d_%H%M%S_%5N.log",
                        boost::log::keywords::rotation_size = 5 * 1024 * 1024,
                        boost::log::keywords::time_based_rotation =
                                boost::log::sinks::file::rotation_at_time_point(0, 0, 0)
                );
        backend->auto_flush(true);
        // Wrap it into the frontend and register in the core.
        // The backend requires synchronization in the frontend.
        typedef boost::log::sinks::synchronous_sink <boost::log::sinks::text_file_backend> sink_t;
        boost::shared_ptr <sink_t> sink(new sink_t(backend));
//    sink->set_formatter(
//            boost::log::expressions::format("%1% %2% %|6t|%3% : %4%")
//            % boost::log::expressions::format_date_time<boost::posix_time::ptime>
//                    ("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
//            % boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
//            % boost::log::trivial::severity
//            % boost::log::expressions::smessage
//    );
        sink->set_formatter(
                boost::log::expressions::stream
                        << boost::log::expressions::format_date_time<boost::posix_time::ptime>
                                ("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                        //            << " " << boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
                        << " " << std::setw(7) << std::left << boost::log::trivial::severity
                        << " " << boost::log::expressions::smessage
        );

        boost::log::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
//    boost::log::core::get()->add_global_attribute("ThreadID",  boost::log::attributes::current_thread_id());
        core->add_sink(sink);

        // cout stream
        boost::shared_ptr< boost::log::sinks::text_ostream_backend > cout_backend =
                boost::make_shared< boost::log::sinks::text_ostream_backend >(

                        );
        cout_backend->add_stream(
                boost::shared_ptr< std::ostream >(&std::cout));
        cout_backend->auto_flush(true);
        auto cout_sink = boost::shared_ptr<
                boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend >
        >(
                new boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend >(cout_backend)) ;
        cout_sink->set_formatter(
                boost::log::expressions::stream
                        << " " << std::setw(7) << std::left << boost::log::trivial::severity
                        << " " << boost::log::expressions::smessage
        );
        core->add_sink(cout_sink);

        boost::log::core::get()->set_filter
                (
                        boost::log::trivial::severity >= boost::log::trivial::trace
                );

        boost::log::add_common_attributes();

        init_log_in_thread();

        LOG_DEBUG << "Logger start.";
    }


}