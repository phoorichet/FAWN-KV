namespace cpp silt

service SiltCluster {
  
  i32 join(1:string ip, 2:i32 port),
  i32 put(1:string key, 2:string value),
  string get(1:string key)

}