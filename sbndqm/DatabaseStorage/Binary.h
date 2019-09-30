#ifndef _sbndqm_Binary_hh
#define _sbndqm_Binary_hh
#include <string>

class redisContext;

namespace sbndqm {
  int SaveBinary(redisContext *redis, const std::string &key, char *data, unsigned length);
}


#endif
