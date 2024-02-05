#include "sbndaq-redis-plugin/Utilities.h"
 


std::string fRedisHostname;                                                                                                                     int fRedisPort; 
std::string fRedisPassword;    

void RedisConnect{
  fRedisHostname(p.get<std::string>("RedisHostname","sbnd-db01")),
  fRedisPort(p.get<int>("RedisPort",6379)),
  fRedisPassword(p.get<std::string>("RedisPassword","sbnd"))
  
    context =  sbndaq::Connect2Redis(fRedisHostname,fRedisPort,fRedisPassword);

  }
