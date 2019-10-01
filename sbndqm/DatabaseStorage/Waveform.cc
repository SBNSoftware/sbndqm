#include <stdlib.h>

#include "sbndaq-redis-plugin/hiredis/hiredis.h"
#include "sbndaq-redis-plugin/hiredis/async.h"

#include "Waveform.h"

int sbndqm::SendRedisCommand(redisContext *redis, bool pipeline, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  int ret = 0;
  // TODO: error handling;
  if (pipeline) {
    redisvAppendCommand(redis, fmt, argp); 
  }
  else {
    void *reply = redisvCommand(redis, fmt, argp); 
    (void) reply;
  }
  va_end(argp);
  return ret;
}

int sbndqm::SendRedisCommandArgv(redisContext *c, bool pipeline, int argc, const char **argv, const size_t *argvlen) {
  if (pipeline) {
    redisAppendCommandArgv(c, argc, argv, argvlen);
  }
  else {
    void *reply= redisCommandArgv(c, argc, argv, argvlen);
    std::cout << ((redisReply *)reply)->str << std::endl;
    (void)reply;
  }
  return 0;

}


