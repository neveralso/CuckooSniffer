#include "samba/sniffer_data.hpp"

namespace cs::samba {

SnifferData::SnifferData(
        std::string &&data) :
        cs::base::SnifferData(DataType::SAMBA) {
    data_ = std::move(data);
}

const std::string &SnifferData::get_data() const {
    return data_;
}

}