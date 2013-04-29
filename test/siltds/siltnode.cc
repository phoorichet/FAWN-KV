// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include <iostream>
#include "SiltNode.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>

#include "SiltCluster.h"
#include "fawnds_factory.h"
#include "print.h"
#include <signal.h>
#include "csapp.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::silt;
using namespace std;
using namespace silt;

silt::FawnDS *h;

char *master_ip;
int master_port;
char *myIP;
int myPort;

TThreadedServer *tserver;
SiltClusterClient *client;
boost::shared_ptr<TTransport> transport_ptr;

class SiltNodeHandler : virtual public SiltNodeIf {
 public:
  SiltNodeHandler() {
    // Your initialization goes here
  }

  int32_t connect_master(const std::string& ip, const int32_t port) {
    // Your implementation goes here
    printf("connect_master\n");


    return 0;
  }

  int32_t put(const std::string& key, const std::string& value) {
    // Your implementation goes here
    // cout << "$$$ Client got PUT " << bytes_to_hex(key) << " -> " << bytes_to_hex(value) << endl;

    FawnDS_Return ret = h->Put(ConstRefValue(key.c_str(), key.size()), ConstRefValue(value.c_str(), value.size()));
    if (ret != silt::OK) {
      cout << "error! h->PUT() return key=" << bytes_to_hex(key) 
          <<  " value=" <<  bytes_to_hex(value) 
          << " ret=" << ret 
          << endl;
      //exit(1);
    } 

    return 0;
  }

  void get(std::string& _return, const std::string& key) {
    // Your implementation goes here
    // cout << "$$$ Client got GET " << bytes_to_hex(key) << endl;
    Value ret_data;
    ret_data.resize(0);

    // FawnDS_Return ret = h->get(ConstRefValue(key, strlen(key)), ret_data);
    // if (ret != silt::OK) {
    //     printf("error! h->Get() return key=%s, ret=%d\n", 
    //            key, ret);
    //   }
    
    FawnDS_Return ret = h->Get(ConstRefValue(key.c_str(), key.size()), ret_data);
     _return = string(ret_data.data());
    if (ret != silt::OK) {
      cout << "error! h->GET() return key=" << bytes_to_hex(key) 
          <<  " value=" <<  bytes_to_hex(_return.c_str()) 
          << " ret=" << ret 
          << endl;
      //exit(1);
    } 
      
    return;
  }

};


// char *getmyip() {
//   FILE * fp = popen("/sbin/ifconfig", "r");
//   char result[20];
//   if (fp) {
//     char *p=NULL, *e; size_t n;
    
//     while ((getline(&p, &n, fp) > 0) && p) {
//        if (p = strstr(p, "inet addr:")) {
//             p+=10;
//             if (e = strchr(p, ' ')) {
//                  *e='\0';
//                  printf("%s\n", p);
//                   char *ptr = p;
//                   int c=0;
//                   while( *ptr != '\n'){
//                     c++;
//                     ptr++;
//                     // printf("c=%d\n", c);
//                   }
                  
//                   strncpy(result, p, c);
//                   printf("Self IP=%s\n", result);
//                   break;
//             }
//        }
//     }


//   }
//   pclose(fp);
//   return result;
// }

int join_cluster(){
  cout << "Connecting to " << master_ip << ":" << master_port << endl;

  boost::shared_ptr<TSocket> socket(new TSocket(master_ip, master_port));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  
  
  client = new SiltClusterClient(protocol);
  transport->open();
  int rc = 0;
  if ((rc=client->join(myIP, myPort)) != 0){
    cout << "##### client connot connect to " << master_ip << ":" << master_port << endl;
  }else{
    cout << "##### client connected to " << master_ip << ":" << master_port << endl;
  }

  transport_ptr = transport;
  
  return rc;
}


/* $begin sigaction */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
      printf("Signal error");
    return (old_action.sa_handler);
}

void
sigint_handler(int sig)
{
  
  printf("\nReceived sigint_handler: %d for pid=%d\n", sig, getpid());
  cout << "closing silt node..." << endl;
  client->leave(myIP, myPort);
  transport_ptr->close();
  tserver->stop();
  h->Close();
  return;
}

int main(int argc, char **argv) {

  if (argc < 4){
    cerr << "Usage: cluster_ip cluster_port myip myport xmlconfig" << endl;
    exit(1);
  }
  

  master_ip = argv[1];
  master_port = atoi(argv[2]);
  myIP = argv[3];
  myPort = atoi(argv[4]);
  char *xmlconfig = argv[5];

  shared_ptr<SiltNodeHandler> handler(new SiltNodeHandler());
  shared_ptr<TProcessor> processor(new SiltNodeProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(myPort));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());


  cout << "##### master" << master_ip << ":" << master_port 
      << " myself " << myIP << ":" << myPort
       << xmlconfig << endl;

  Signal(SIGINT,  sigint_handler);   /* ctrl-c */

  try{
    // Create silt database
    h = silt::FawnDS_Factory::New(xmlconfig); // test_num_records_
    if (h->Create() != silt::OK) {
          cout << "cannot create FAWNDS!" << endl;
          exit(0);
    }


    // Join the cluster
    if (join_cluster() != 0)
    {
      h->Close();
      exit(1);
    }


              tserver =  new TThreadedServer(processor,
                           serverTransport,
                           transportFactory,
                           protocolFactory);

    // TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    cout << "##### Running on port " << myPort << endl;


    tserver->serve();

    h->Close();

  }catch (const TException &tx) {
      std::cerr << "ERROR: " << tx.what() << std::endl;
      boost::shared_ptr<TSocket> socket(new TSocket(master_ip, master_port));
      boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
      boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
      
      SiltClusterClient *client;
      client = new SiltClusterClient(protocol);
      transport->open();

      client->leave(myIP, myPort);

      transport->close();
  }
  return 0;
}

