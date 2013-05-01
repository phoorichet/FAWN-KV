 #!/bin/bash
 echo $1
 ./testByYCSBWorkload testConfigs/testCombi.xml testWorkloads/exp_tiny_6.4GB testWorkloads/exp_tiny_6.4GB_update_perf_50 128.2.131.18 9090 -t $1