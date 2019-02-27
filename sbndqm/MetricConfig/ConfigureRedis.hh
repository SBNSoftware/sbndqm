#include <array>
#include <string>
#include <experimental/optional>

#include "sbndaq-redis-plugin/hiredis/hiredis.h"
#include "sbndaq-redis-plugin/hiredis/async.h"

#include "fhiclcpp/ParameterSet.h"

namespace sbndqm {
  void InitializeMetricConfig(const fhicl::ParameterSet &config);

  struct MetricConfig {
    std::experimental::optional<std::array<double, 2>> display_range;
    std::experimental::optional<std::array<double, 2>> warning_range;
    std::experimental::optional<std::array<double, 2>> alarm_range;
    std::experimental::optional<std::string> units;
    std::experimental::optional<std::string> title;
  };

  unsigned AddGroup(redisContext *redis, const std::string &group_name);
  unsigned GroupMetricConfig(redisContext *redis, const std::string &group_name, const std::vector<std::string> &metric_names, const std::vector<MetricConfig> &metric_configs);

  bool ProcessRedisReply(void *reply);
  bool ProcessAllRedisReplies(redisContext *redis, unsigned n_commands);

}
