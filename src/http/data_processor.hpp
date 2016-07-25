#ifndef CUCKOOSNIFFER_HTTP_DATA_PROCESSOR_HPP
#define CUCKOOSNIFFER_HTTP_DATA_PROCESSOR_HPP

#include <regex>

#include "base/data_processor.hpp"
#include "base/sniffer_data.hpp"

namespace cs::http {

class DataProcessor : public cs::base::DataProcessor {

public:

    virtual int process(const cs::base::SnifferData &);

    virtual ~DataProcessor();

};

}


#endif //CUCKOOSNIFFER_HTTP_DATA_PROCESSOR_HPP
