#ifndef _sbndqm_Binary_hh
#define _sbndqm_Binary_hh

namespace sbndqm {
  int SaveBinary(redisContext *redis, const std::string &key, char *data, unsigned length);
}


#endif
