//
//  testSilt.cpp
//  SiltDB
//
//  Created by Lock on 4/2/13.
//  Copyright (c) 2013 Lock. All rights reserved.
//

#include "testSilt.h"
#include <vector>
#include "fawnds_factory.h"
#include "fawnds.h"

using namespace std;
using namespace silt;

static std::string conf_file = "testConfigs/testCuckoo.xml";


void do_test(void){
  // Create a hash store
//  	Configuration* config = new Configuration(conf_file);
//  FawnDS *h =  FawnDS_Factory(new Configuration(conf_file));
//  cout << "test" << endl;
//  if (h->Create() == OK){
//    
//  }
  
  // Clean up
//  delete h;
}

int main(int argc, char ** argvs){
  do_test();
  
  return 0;
}