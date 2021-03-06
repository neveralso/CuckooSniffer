#include <iostream>
#include <sstream>

#include "cuckoo_sniffer.hpp"

#include "tins/tcp_ip/stream_follower.h"
#include "tins/sniffer.h"

#include "sniffer_manager.hpp"
#include "threads/thread.hpp"
#include "smtp/sniffer.hpp"
#include "imap/sniffer.hpp"
#include "ftp/data_sniffer.hpp"
#include "ftp/command_sniffer.hpp"
#include "http/sniffer.hpp"
#include "samba/sniffer.hpp"
#include "util/function.hpp"
#include "util/option_parser.hpp"

void on_new_connection(Tins::TCPIP::Stream& stream) {
    cs::base::TCPSniffer* tcp_sniffer = nullptr;
    uint16_t port = stream.server_port();
    LOG_TRACE << cs::util::stream_identifier(stream) << " Get tcp stream." ;
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
        case 80:        //HTTP
            tcp_sniffer = new cs::http::Sniffer(stream);
            break;
        case 445:       //SAMBA
            tcp_sniffer = new cs::samba::Sniffer(stream);
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
    std::string stream_id = cs::util::stream_identifier(stream);
    LOG_INFO << "Connection terminated " << stream_id;
    ((cs::base::TCPSniffer*)cs::SNIFFER_MANAGER.get_sniffer(stream_id)) ->on_connection_terminated(stream, reason);
    cs::SNIFFER_MANAGER.erase_sniffer(stream_id);
}


int main(int argc, const char* argv[]) {
    try {
        std::map<std::string, std::string> parsed_cfg;
        int ret = cs::util::parse_cfg(argc, argv, parsed_cfg);
        if (ret < 0) {
            std::cerr << "Parse config failed." << std::endl;
            return ret;
        }

        cs::init_log();

        LOG_INFO << "Config:";
        for(const auto& cfg: parsed_cfg) {
            LOG_INFO << cfg.first << " = " << cfg.second;
        }

        std::string interface_name = parsed_cfg["interface"];

        cs::threads::start_threads(2);

        Tins::SnifferConfiguration config;
        config.set_filter("(tcp port 445)");
        config.set_promisc_mode(true);
        Tins::Sniffer sniffer(interface_name, config);

        LOG_INFO << "Start sniffer on " << interface_name;

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
