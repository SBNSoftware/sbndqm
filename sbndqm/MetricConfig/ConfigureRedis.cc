#include "ConfigureRedis.hh"

#include <string>
#include <vector>
#include <fstream>

#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "sbndaq-redis-plugin/Utilities.h"

#include "json/json.h"
#include "json/writer.h"

void sbndqm::InitializeMetricConfig(const fhicl::ParameterSet &config) {
  // Load connection config
  std::string server_name = config.get<std::string>("hostname", "localhost");
  int server_port = config.get<int>("port", 6379);
  std::string password = config.get<std::string>("redis_password", "");
  // check password file if necessary
  if (password.size() == 0) {
    std::string passfile_name = config.get<std::string>("redis_passfile", "");
    if (passfile_name.size() > 0) {
      std::ifstream passfile(passfile_name);
      if (passfile.good()) {
        passfile >> password;
      }
    }
    else {
      mf::LogError("Redis Metric Config") << "Failed to open password file.";
      return;
    }
  }

  // make connection
  void *reply = NULL;
  redisContext *context = sbndaq::Connect2Redis(server_name, server_port, password, &reply);
  
  // check connection
  if (context == NULL || context->err) {
    if (context) {
       mf::LogError("Redis Metric Config") <<  "Redis connection error reply: " << context->errstr;
       redisFree(context);
    }
    else {
      mf::LogError("Redis Metric Config") << "Cannot allocate redis context.";
    }
    return;
  }

  // check auth if neccessary 
  if (password.size() > 0) {
    bool success = sbndqm::ProcessRedisReply(reply);
    if (!success) {
      redisFree(context);
      return;
    }
  }

  // start sending config commands to redis
  // keep track of the number of pipelined commands
  unsigned n_commands = 0;

  // set all the groups
  std::vector<std::string> groups = config.get<std::vector<std::string>>("groups", {});
  // make sure names are valid
  for (unsigned i = 0; i < groups.size(); i++) {
    groups[i] = sbndaq::ValidateRedisName(groups[i]);
  }

  for (const std::string &group_name: groups) {
    n_commands += AddGroup(context, group_name);
  }

  // and set the metric config for each group
  fhicl::ParameterSet metric_config = config.get<fhicl::ParameterSet>("metrics", {});

  // get the metric names and their configs
  std::vector<std::string> metrics = metric_config.get_pset_names();
  std::vector<sbndqm::MetricConfig> metric_configs;
  for (const std::string &metric_name: metrics) {
    fhicl::ParameterSet this_config = metric_config.get<fhicl::ParameterSet>(metric_name);
    // build the metric config object
    sbndqm::MetricConfig redis_metric_config;

    if (this_config.has_key("display_range")) {
      redis_metric_config.display_range = this_config.get<std::array<double, 2>>("display_range");
    }
    else {
      redis_metric_config.display_range = {};
    }

    if (this_config.has_key("warning_range")) {
      redis_metric_config.warning_range = this_config.get<std::array<double, 2>>("warning_range");
    }
    else {
      redis_metric_config.warning_range = {};
    }

    if (this_config.has_key("alarm_range")) {
      redis_metric_config.alarm_range = this_config.get<std::array<double, 2>>("alarm_range");
    }
    else {
      redis_metric_config.alarm_range = {};
    }

    if (this_config.has_key("units")) {
      redis_metric_config.units = this_config.get<std::string>("units");
    }
    else {
      redis_metric_config.units = {};
    }

    if (this_config.has_key("title")) {
      redis_metric_config.title = this_config.get<std::string>("title");
    }
    else {
      redis_metric_config.title = {};
    }
    metric_configs.push_back(redis_metric_config);
  }

  // send the config to each group
  for (const std::string &group_name: groups) {
    n_commands += GroupMetricConfig(context, group_name, metrics, metric_configs);
  }

  // send all the commands down the pipeline
  bool success = ProcessAllRedisReplies(context, n_commands);
  if (!success) {
    mf::LogError("Redis Metric Config") << "Redis metric configuration failed.";
  }

  redisFree(context);

  return;
}

unsigned sbndqm::AddGroup(redisContext *redis, const std::string &group_name) {
  redisAppendCommand(redis, "SADD GROUPS %s", group_name.c_str());
  return 1;
}

unsigned sbndqm::GroupMetricConfig(redisContext *redis, const std::string &group_name, 
    const std::vector<std::string> &metric_names, 
    const std::vector<sbndqm::MetricConfig> &metric_configs) {

  // encode the metric config as a JSON dictionary
  Json::Value root; 
  
  for (unsigned i = 0; i < metric_names.size(); i++) {
    const sbndqm::MetricConfig &config = metric_configs[i];
    const std::string &metric = metric_names[i];

    Json::Value json_config;
    if (config.display_range) {
      json_config["display_range"] = Json::arrayValue;
      json_config["display_range"][0] = config.display_range.value()[0];
      json_config["display_range"][1] = config.display_range.value()[1];
    }
    if (config.warning_range) {
      json_config["warning_range"] = Json::arrayValue;
      json_config["warning_range"][0] = config.warning_range.value()[0];
      json_config["warning_range"][1] = config.warning_range.value()[1];
    }
    if (config.alarm_range) {
      json_config["alarm_range"] = Json::arrayValue;
      json_config["alarm_range"][0] = config.alarm_range.value()[0];
      json_config["alarm_range"][1] = config.alarm_range.value()[1];
    }
    if (config.units) {
      json_config["units"] = config.units.value();
    }
    if (config.title) {
      json_config["title"] = config.title.value();
    }
    json_config["name"] = metric;
    root[metric] = json_config;
  }

  // print the JSON config to string
  Json::FastWriter fastWriter;
  std::string json_config_str = fastWriter.write(root);

  // send it out
  redisAppendCommand(redis, "SET GROUP_CONFIG:%s %s", group_name.c_str(), json_config_str.c_str());
  return 1;
}

bool sbndqm::ProcessAllRedisReplies(redisContext *redis, unsigned n_commands) {
  bool success = true;
  for (unsigned i = 0; i < n_commands; i++) {
    void *reply = NULL;
    redisGetReply(redis, &reply);
    bool this_success = ProcessRedisReply(reply);
    if (success) success = this_success;
  }
  return success;
}

bool sbndqm::ProcessRedisReply(void *r) {
  if (r == NULL) {
    mf::LogError("Redis Metric Config") << "Redis connection NULL reply";
    return false;
  }

  redisReply *reply = (redisReply *)r;
  switch (reply->type) {
    case REDIS_REPLY_ERROR:
      mf::LogError("Redis Metric Config") << "Redis connection error reply: " << reply->str;
      return false;
    case REDIS_REPLY_STATUS:
      mf::LogDebug("Redis Metric Config") << "Message reply status: " << reply->str;
      break;
    default:
      break;
  }
  freeReplyObject(reply);
  return true;
}


