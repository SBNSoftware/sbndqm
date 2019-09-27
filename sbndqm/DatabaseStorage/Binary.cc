#include <string>

#include "sbndaq-redis-plugin/hiredis/hiredis.h"
#include "sbndaq-redis-plugin/hiredis/async.h"

#include "Binary.h"

int sbndqm::SendBinary(redisContext *redis, const std::string &key, char *data, unsigned length) {
  void *reply = redisCommand(redis, "SET %s %b", key.c_str(), data, length);
  (void) reply;
  return 0;
}


