/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _FAWNDS_FACTORY_H_
#define _FAWNDS_FACTORY_H_

//using silt::DataHeader;
//using silt::HashUtil;

// using namespace std;

#include "configuration.h"
#include "fawnds.h"

// using namespace silt;

namespace silt {
    // class FawnDS;

    class FawnDS_Factory {
    public:
        static silt::FawnDS* New(std::string config_file);
        static silt::FawnDS* New(const silt::Configuration* config);
    };

} // namespace silt

#endif  // #ifndef _FAWNDS_FACTORY_H_
