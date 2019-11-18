#include "sbndaq-redis-plugin/Utilities.h"
 


std::string fRedisHostname;                                                                                                                     int fRedisPort; 
std::string fRedisPassword;    

void RedisConnect{
  fRedisHostname(p.get<std::string>("RedisHostname","icarus-db02")),
  fRedisPort(p.get<int>("RedisPort",6379)),
  fRedisPassword(p.get<std::string>("RedisPassword","icarus"))
  
    context =  sbndaq::Connect2Redis(fRedisHostname,fRedisPort,fRedisPassword);

  }
