#include "sniffer_manager.hpp"

namespace cs {

SnifferManager &SNIFFER_MANAGER = SnifferManager::get_instance();

SnifferManager SnifferManager::instance;

SnifferManager &SnifferManager::get_instance() {
    return instance;
}

void SnifferManager::append_sniffer(std::string sniffer_id, cs::base::Sniffer *sniffer_ptr) {
    sniffer_container[sniffer_id] = sniffer_ptr;
    std::cout << "new sniffer " << sniffer_id << ", total: " << sniffer_container.size() << std::endl;
}

cs::base::Sniffer *SnifferManager::get_sniffer(std::string sniffer_id) {
    return sniffer_container[sniffer_id];
}

void SnifferManager::erase_sniffer(std::string sniffer_id) {
    cs::base::Sniffer *sniffer = sniffer_container[sniffer_id];
    delete sniffer;
    sniffer_container.erase(sniffer_id);
    std::cout << "erase sniffer " << sniffer_id << ", total: " << sniffer_container.size() << std::endl;
}

SnifferManager::SnifferManager() {
    sniffer_container.clear();
}

}