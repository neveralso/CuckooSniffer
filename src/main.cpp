#include <iostream>
#include <sstream>

#include <boost/log/common.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/record_ordering.hpp>
#include <boost/log/support/date_time.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/thread/thread.hpp>

#include "tins/tcp_ip/stream_follower.h"
#include "tins/sniffer.h"
#include "tins/packet.h"

#include "sniffer_manager.hpp"

#include "smtp/sniffer.hpp"
#include "imap/sniffer.hpp"
#include "ftp/data_sniffer.hpp"
#include "ftp/command_sniffer.hpp"

void on_new_connection(Tins::TCPIP::Stream& stream) {
    cs::base::TCPSniffer* tcp_sniffer = nullptr;
    uint16_t port = stream.server_port();
    std::cout << port << std::endl;
    switch (stream.server_port()) {
        case 25:        //SMTP
            tcp_sniffer = new cs::smtp::Sniffer(stream);
            break;
        case 143:       //IMAP
            tcp_sniffer = new cs::imap::Sniffer(stream);
            break;
        case 21:        //FTP
            tcp_sniffer = new cs::ftp::CommandSniffer(stream);
            break;
        default:
            const auto& ftp_data_connection = cs::ftp::CommandSniffer::get_data_connection_pool();
            if (ftp_data_connection.find(port) != ftp_data_connection.end()) {
                tcp_sniffer = new cs::ftp::DataSniffer(stream);
            }
            else {
                stream.auto_cleanup_payloads(true);
                return;
            }
            break;
    }

    cs::SNIFFER_MANAGER.append_sniffer(tcp_sniffer -> get_id(), (cs::base::Sniffer*)tcp_sniffer);

}


void on_connection_terminated(Tins::TCPIP::Stream& stream, Tins::TCPIP::StreamFollower::TerminationReason reason) {
    std::cout << "[+] Connection terminated " << cs::base::TCPSniffer::stream_identifier(stream) << std::endl;
    std::string stream_id = cs::base::TCPSniffer::stream_identifier(stream);
    ((cs::base::TCPSniffer*)cs::SNIFFER_MANAGER.get_sniffer(stream_id)) ->on_connection_terminated(stream, reason);
    cs::SNIFFER_MANAGER.erase_sniffer(cs::base::TCPSniffer::stream_identifier(stream));
}

void init_log()
{
    namespace logging = boost::log;
    namespace src = boost::log::sources;
    namespace sinks = boost::log::sinks;
    namespace keywords = boost::log::keywords;
    namespace expr = boost::log::expressions;
    using namespace logging::trivial;

    boost::shared_ptr< logging::core > core = logging::core::get();

    boost::shared_ptr< sinks::text_file_backend > backend =
            boost::make_shared< sinks::text_file_backend >(
                    keywords::file_name = "cs_%Y%m%d_%H%M%S_%5N.log",
                    keywords::rotation_size = 5 * 1024 * 1024,
                    keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0)
    );
    backend -> auto_flush(true);

    // Wrap it into the frontend and register in the core.
    // The backend requires synchronization in the frontend.
    typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
    boost::shared_ptr< sink_t > sink(new sink_t(backend));

    BOOST_LOG_SCOPED_THREAD_TAG("ThreadID", boost::this_thread::get_id());

    sink-> set_formatter (
                    expr::format("%1% %2% %3% %4%") % expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
                                                % boost::log::expressions::attr< boost::thread::id >("ThreadID")
                                                % boost::log::trivial::severity
                                                % expr::smessage
    );

    core->add_sink(sink);


    logging::core::get()->set_filter
            (
                    logging::trivial::severity >= logging::trivial::info
            );

    logging::add_common_attributes();
    src::severity_logger< severity_level > lg;

    BOOST_LOG_SEV(lg, trace) << "A trace severity message";
    BOOST_LOG_SEV(lg, debug) << "A debug severity message";
    BOOST_LOG_SEV(lg, info) << "An informational severity message";
    BOOST_LOG_SEV(lg, warning) << "A warning severity message";
    BOOST_LOG_SEV(lg, error) << "An error severity message";
    BOOST_LOG_SEV(lg, fatal) << "A fatal severity message";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: CuckooSniffer <device name>" << std::endl;
        return 0;
    }
    try {

        init_log();

        Tins::SnifferConfiguration config;
//        config.set_filter("(tcp port 25) or (tcp port 143) or (tcp port 21)");
        config.set_promisc_mode(true);
        Tins::Sniffer sniffer(argv[1], config);

        Tins::TCPIP::StreamFollower follower;
        follower.new_stream_callback(&on_new_connection);
        follower.stream_termination_callback(on_connection_terminated);

        sniffer.sniff_loop([&](Tins::PDU& packet) {
            Tins::PDU* layer2_pdu = &packet;
            Tins::PDU* layer3_pdu = layer2_pdu->inner_pdu();
            Tins::PDU* layer4_pdu = layer3_pdu->inner_pdu();
            switch (layer2_pdu->pdu_type()) {
                case Tins::PDU::ETHERNET_II:

                    switch (layer3_pdu->pdu_type()) {
                        case Tins::PDU::IP:

                            switch (layer4_pdu->pdu_type()) {
                                case Tins::PDU::TCP:
                                    follower.process_packet(packet);
                                    break;
                                default:
                                    break;
                            }
                            break;

                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }
            return true;
        });
    }
    catch (std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
}