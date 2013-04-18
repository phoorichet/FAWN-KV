/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _TFAWNKV_H_
#define _TFAWNKV_H_

#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TSocket.h>
#include <tbb/atomic.h>

#include "fe.h"

//#include "FawnKV.h"
//#include "FawnKVApp.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace tbb;
using namespace std;

using boost::shared_ptr;

using namespace fawn;

#define FE_SERVER_PORT_BASE 7000


class FawnKVClt {
 private:
    FrontEnd *frontend;
    int64_t continuation;
    string myIP;
    uint16_t myPort;

 public:
    FawnKVClt(const std::string& managerIP, const std::string& clientIP, const int32_t clientPort = 0);
    ~FawnKVClt();

    string get(const std::string& key);
    void put(const std::string& key, const std::string& value);
    int64_t put_w(const std::string& key, const std::string& value);
    void remove(const std::string& key);
};


#endif
