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



std::map<int, const char*> pdu_type_to_str = {
        {Tins::PDU::RAW,                  "RAW"},
        {Tins::PDU::ETHERNET_II,          "ETHERNET_II"},
        {Tins::PDU::IEEE802_3,            "IEEE802_3"},
        {Tins::PDU::IEEE802_3,            "IEEE802_3"},
        {Tins::PDU::RADIOTAP,             "RADIOTAP"},
        {Tins::PDU::DOT11,                "DOT11"},
        {Tins::PDU::DOT11_ACK,            "DOT11_ACK"},
        {Tins::PDU::DOT11_ASSOC_REQ,      "DOT11_ASSOC_REQ"},
        {Tins::PDU::DOT11_ASSOC_RESP,     "DOT11_ASSOC_RESP"},
        {Tins::PDU::DOT11_AUTH,           "DOT11_AUTH"},
        {Tins::PDU::DOT11_BEACON,         "DOT11_BEACON"},
        {Tins::PDU::DOT11_BLOCK_ACK,      "DOT11_BLOCK_ACK"},
        {Tins::PDU::DOT11_BLOCK_ACK_REQ,  "DOT11_BLOCK_ACK_REQ"},
        {Tins::PDU::DOT11_CF_END,         "DOT11_CF_END"},
        {Tins::PDU::DOT11_DATA,           "DOT11_DATA"},
        {Tins::PDU::DOT11_CONTROL,        "DOT11_CONTROL"},
        {Tins::PDU::DOT11_DEAUTH,         "DOT11_DEAUTH"},
        {Tins::PDU::DOT11_DIASSOC,        "DOT11_DIASSOC"},
        {Tins::PDU::DOT11_END_CF_ACK,     "DOT11_END_CF_ACK"},
        {Tins::PDU::DOT11_MANAGEMENT,     "DOT11_MANAGEMENT"},
        {Tins::PDU::DOT11_PROBE_REQ,      "DOT11_PROBE_REQ"},
        {Tins::PDU::DOT11_PROBE_RESP,     "DOT11_PROBE_RESP"},
        {Tins::PDU::DOT11_PS_POLL,        "DOT11_PS_POLL"},
        {Tins::PDU::DOT11_REASSOC_REQ,    "DOT11_REASSOC_REQ"},
        {Tins::PDU::DOT11_REASSOC_RESP,   "DOT11_REASSOC_RESP"},
        {Tins::PDU::DOT11_RTS,            "DOT11_RTS"},
        {Tins::PDU::DOT11_QOS_DATA,       "DOT11_QOS_DATA"},
        {Tins::PDU::LLC,                  "LLC"},
        {Tins::PDU::SNAP,                 "SNAP"},
        {Tins::PDU::IP,                   "IP"},
        {Tins::PDU::ARP,                  "ARP"},
        {Tins::PDU::TCP,                  "TCP"},
        {Tins::PDU::UDP,                  "UDP"},
        {Tins::PDU::ICMP,                 "ICMP"},
        {Tins::PDU::BOOTP,                "BOOTP"},
        {Tins::PDU::DHCP,                 "DHCP"},
        {Tins::PDU::EAPOL,                "EAPOL"},
        {Tins::PDU::RC4EAPOL,             "RC4EAPOL"},
        {Tins::PDU::RSNEAPOL,             "RSNEAPOL"},
        {Tins::PDU::DNS,                  "DNS"},
        {Tins::PDU::LOOPBACK,             "LOOPBACK"},
        {Tins::PDU::IPv6,                 "IPv6"},
        {Tins::PDU::ICMPv6,               "ICMPv6"},
        {Tins::PDU::SLL,                  "SLL"},
        {Tins::PDU::DHCPv6,               "DHCPv6"},
        {Tins::PDU::DOT1Q,                "DOT1Q"},
        {Tins::PDU::PPPOE,                "PPPOE"},
        {Tins::PDU::STP,                  "STP"},
        {Tins::PDU::PPI,                  "PPI"},
        {Tins::PDU::IPSEC_AH,             "IPSEC_AH"},
        {Tins::PDU::IPSEC_ESP,            "IPSEC_ESP"},
        {Tins::PDU::PKTAP,                "PKTAP"},
        {Tins::PDU::MPLS,                 "MPLS"},
        {Tins::PDU::UNKNOWN,              "UNKNOWN"},
        {Tins::PDU::USER_DEFINED_PDU,     "USER_DEFINED_PDU"},
};


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
        case 63343:        //HTTP
            tcp_sniffer = new cs::http::Sniffer(stream);
            break;
//        case 445:       //SAMBA
//            tcp_sniffer = new cs::samba::Sniffer(stream);
//            break;
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
//        config.set_filter("ip6 host 2001:da8:215:1660:bbb2:3d41:d32b:dc53");
        config.set_filter("tcp port 63343");
//        config.set_filter("tcp port 80");
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
//            std::cout
//                    << pdu_type_to_str[(int)(layer2_pdu->pdu_type())] << " "
//                    << pdu_type_to_str[(int)(layer3_pdu->pdu_type())] << " "
//                    << pdu_type_to_str[(int)(layer4_pdu->pdu_type())] << std::endl;
            switch (layer2_pdu->pdu_type()) {
                case Tins::PDU::LOOPBACK:
                case Tins::PDU::ETHERNET_II:

                    switch (layer3_pdu->pdu_type()) {
                        case Tins::PDU::IP:
                        case Tins::PDU::IPv6:

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
