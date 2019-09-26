#ifndef _sbndqm_Image_h_
#define _sbndqm_Image_h_

#include <string>

class redisContext;
class TH1;
class TH2;

namespace sbndqm {
  int SendHistogram(redisContext *redis, const std::string &key, TH1 *hist, size_t buffer_x, size_t buffer_y, size_t size_x, size_t size_y);
  int SendHistogram(redisContext *redis, const std::string &key, TH2 *hist, size_t buffer_x, size_t buffer_y, size_t size_x, size_t size_y);
}

#endif
