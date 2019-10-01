#ifndef _sbndqm_Waveform_h_
#define _sbndqm_Waveform_h_

#include <string.h>
#include <iostream>
#include <string>
#include <vector>
class redisContext;

namespace sbndqm {

// Public-facing functions

// Send a single waveform to a database
template<typename DataType>
int SendWaveform(
    redisContext *redis,
    const std::string &key,
    const std::vector<DataType> &waveform,
    float tick_period=1.,
    bool pipeline=false);

// Send a number of waveforms (e.g. an OpDetWaveform) to a databse
template<typename DataType, typename OffsetType>
int SendSplitWaveform(
    redisContext *redis,
    const std::string &key,
    const std::vector<std::vector<DataType>> &waveforms,
    const std::vector<OffsetType> &offsets,
    float tick_period=1.,
    bool pipeline=false);

// Internal cunstions for sending stuff to redis
int SendRedisCommand(redisContext *redis, bool pipeline, const char *fmt, ...);
int SendRedisCommandArgv(redisContext *c, bool pipeline, int argc, const char **argv, const size_t *argvlen);

template<typename T>
struct TypeParseTraits;
#define REGISTER_PARSE_TYPE(type) template <> struct TypeParseTraits<type> \
    { static const char *name; }; \
    const char *TypeParseTraits<type>::name = #type;
REGISTER_PARSE_TYPE(double);
REGISTER_PARSE_TYPE(float);
REGISTER_PARSE_TYPE(uint8_t);
REGISTER_PARSE_TYPE(uint16_t);
REGISTER_PARSE_TYPE(uint32_t);
REGISTER_PARSE_TYPE(uint64_t);
REGISTER_PARSE_TYPE(int8_t);
REGISTER_PARSE_TYPE(int16_t);
REGISTER_PARSE_TYPE(int32_t);
REGISTER_PARSE_TYPE(int64_t);

template<typename DataType, typename OffsetType>
int SendSplitWaveform(
    redisContext *redis, 
    const std::string &key, 
    const std::vector<std::vector<DataType>> &waveforms, 
    const std::vector<OffsetType> &offsets, 
    float tick_period,
    bool pipeline) {
  
  // first -- delete the old key
  (void) SendRedisCommand(redis, pipeline, "DEL %s", key.c_str());

  // set the type-names
  (void) SendRedisCommand(redis, pipeline, "HMSET %s DataType %s", key.c_str(), TypeParseTraits<DataType>::name); 
  (void) SendRedisCommand(redis, pipeline, "HMSET %s OffsetType %s", key.c_str(), TypeParseTraits<OffsetType>::name);
  (void) SendRedisCommand(redis, pipeline, "HMSET %s SizeType %s", key.c_str(), TypeParseTraits<uint32_t>::name);

  // set the period size
  (void) SendRedisCommand(redis, pipeline, "HMSET %s TickPeriod %f", key.c_str(), tick_period);

  // mash the waveforms together into a single command
  // copy the waveforms into a single vector
  std::vector<DataType> all_waveforms;
  for (const std::vector<DataType> &wf: waveforms) {
    all_waveforms.insert(all_waveforms.end(), wf.begin(), wf.end());
  }
  (void) SendRedisCommand(redis, pipeline, "HMSET %s Data %b", key.c_str(), &all_waveforms[0], all_waveforms.size() * sizeof(DataType));

  // Save the offsets
  (void) SendRedisCommand(redis, pipeline, "HMSET %s Offsets %b", key.c_str(), &offsets[0], offsets.size() * sizeof(OffsetType));

  // Save the sizes of the waveforms
  std::vector<uint32_t> waveform_sizes;
  for (const std::vector<DataType> &wf: waveforms) {
    waveform_sizes.push_back(wf.size());
  }
  (void) SendRedisCommand(redis, pipeline, "HMSET %s Sizes %b", key.c_str(), &waveform_sizes[0], waveform_sizes.size() * sizeof(uint32_t));

  // if we pipelined, there were 8 commands, otherwise nothing to return
  return (pipeline) ? 8 : 0;
} 

template <typename DataType>
int SendWaveform(
    redisContext *redis, 
    const std::string &key, 
    const std::vector<DataType> &waveform,
    float tick_period,
    bool pipeline) {
  
  // first -- delete the old key
  (void) SendRedisCommand(redis, pipeline, "DEL %s", key.c_str());

  // set the type-names
  (void) SendRedisCommand(redis, pipeline, "HMSET %s DataType %s", key.c_str(), TypeParseTraits<DataType>::name); 

  // save the waveforms
  (void) SendRedisCommand(redis, pipeline, "HMSET %s Data %b", key.c_str(), &waveform[0], waveform.size() * sizeof(DataType));

  // set the period size
  (void) SendRedisCommand(redis, pipeline, "HMSET %s TickPeriod %f", key.c_str(), tick_period);

  // if we pipelined, there were 4 commands, otherwise nothing to return
  return (pipeline) ? 4 : 0;
} 
} // end namespace sbndqm

#endif
