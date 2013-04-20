/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _FAWNDS_FACTORY_H_
#define _FAWNDS_FACTORY_H_

//using silt::DataHeader;
//using silt::HashUtil;

//using namespace std;

#include "configuration.h"
#include "fawnds.h"

namespace silt {

    class FawnDS_Factory {
    public:
        static FawnDS* New(std::string config_file);
        static FawnDS* New(const Configuration* config);
    };

} // namespace silt

#endif  // #ifndef _FAWNDS_FACTORY_H_
