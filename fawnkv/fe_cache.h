/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _FECACHE_H_
#define _FECACHE_H_

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
#include <tbb/concurrent_hash_map.h>
#include <time.h>

#include "ring.h"
#include "FawnKV.h"
#include "FawnKVApp.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace tbb;

using boost::shared_ptr;

using namespace fawn;

typedef concurrent_hash_map<string,string> CacheTable;

class Cache {
private:
    CacheTable *old;
    CacheTable *cur;
    atomic<int32_t> sizeCur;
    atomic<int32_t> sizeOld;
    atomic<int32_t> num_hits;
    atomic<int32_t> num_lookups;

public:
    Cache(int m);
    bool lookup(const DBID& dkey, std::string& value);
    void insert(const DBID& dkey, const std::string& value);
    int sizeMax;
    int  size() {
        return sizeOld + sizeCur;
    }
    double hit_ratio() {
        return 1.0 * num_hits / num_lookups;
    }
    double load_factor() {
        return 1.0 * (sizeOld + sizeCur) / sizeMax;
    }
};

#endif
