#include <array>
#include <string>
#include <experimental/optional>

#include "sbndaq-online/hiredis/hiredis.h"
#include "sbndaq-online/hiredis/async.h"

#include "fhiclcpp/ParameterSet.h"

namespace sbndqm {
  void GenerateMetricConfig(const fhicl::ParameterSet &config);

  struct MetricConfig {
    std::experimental::optional<std::array<double, 2>> display_range;
    std::experimental::optional<std::array<double, 2>> warning_range;
    std::experimental::optional<std::array<double, 2>> alarm_range;
    std::experimental::optional<std::string> units;
    std::experimental::optional<std::string> title;
  };

  unsigned AddGroup(redisContext *redis, const std::string &group_name);
  unsigned GroupMetricConfig(redisContext *redis, const std::string &group_name, 
    const std::vector<std::string> &metric_names, const std::vector<MetricConfig> &metric_configs,
    const std::vector<std::string> &stream_names);
  unsigned AddInstances2Group(redisContext *redis, const std::string &group_name, const std::vector<std::string> &instance_names);

  bool ProcessRedisReply(void *reply);
  bool ProcessAllRedisReplies(redisContext *redis, unsigned n_commands);

}
