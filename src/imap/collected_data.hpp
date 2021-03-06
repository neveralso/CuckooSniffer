#ifndef CUCKOOSNIFFER_IMAP_COLLECTED_DATA_HPP
#define CUCKOOSNIFFER_IMAP_COLLECTED_DATA_HPP

#include <string>
#include "base/collected_data.hpp"

namespace cs {
namespace imap {

class CollectedData : public cs::base::CollectedData {

public:

    CollectedData();

    void append(const std::string &);

    const std::string &get_data() const;

private:

    std::string data_;

};

}
}


#endif //CUCKOOSNIFFER_IMAP_COLLECTED_DATA_HPP
