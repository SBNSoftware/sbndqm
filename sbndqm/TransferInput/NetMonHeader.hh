#ifndef artdaq_DAQdata_NetMonHeader_hh
#define artdaq_DAQdata_NetMonHeader_hh

#include <cstdint>

namespace sbndqm_artdaq
{
	struct NetMonHeader;
}

/**
 * \brief Header with length information for NetMonTransport messages
 */
struct sbndqm_artdaq::NetMonHeader
{
	uint64_t data_length; ///< The length of the message
};

#endif /* artdaq_DAQdata_NetMonHeader_hh */
