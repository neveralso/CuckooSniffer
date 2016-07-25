#include "sniffer_manager.hpp"
#include "ftp/sniffer.hpp"
#include "ftp/sniffer_data.hpp"
#include "ftp/data_processor.hpp"

namespace cs::ftp {

void Sniffer::on_client_payload(const Tins::TCPIP::Stream &stream) {
}

void Sniffer::on_server_payload(const Tins::TCPIP::Stream &stream) {
}

void Sniffer::on_connection_close(const Tins::TCPIP::Stream &stream) {
    std::cout << "Connection Close" << std::endl;

    //TODO make this process in thread
    SnifferData *ftp_data = new SnifferData(
            std::string(stream.client_payload().begin(), stream.client_payload().end())
    );

    DataProcessor processor;
    processor.process(*((SnifferData *) ftp_data));


    delete ftp_data;
    SNIFFER_MANAGER.erase_sniffer(id_);
}

void Sniffer::on_connection_terminated(
        Tins::TCPIP::Stream &,
        Tins::TCPIP::StreamFollower::TerminationReason) {

    std::cout << "[+] On Connection terminated " << id_ << std::endl;
}

Sniffer::Sniffer(Tins::TCPIP::Stream &stream) : TCPSniffer(stream) {

    stream.ignore_server_data();
    stream.auto_cleanup_client_data(false);
    stream.client_data_callback(
            [this](const Tins::TCPIP::Stream &tcp_stream) {
                this->on_client_payload(tcp_stream);
            }
    );

    stream.stream_closed_callback(
            [this](const Tins::TCPIP::Stream &tcp_stream) {
                this->on_connection_close(tcp_stream);
            }
    );

}

Sniffer::~Sniffer() {

}

}
