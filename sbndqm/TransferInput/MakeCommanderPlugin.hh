#ifndef artdaq_ExternalComms_MakeCommanderPlugin_hh
#define artdaq_ExternalComms_MakeCommanderPlugin_hh

// JCF, Sep-6-2016

// MakeCommanderPlugin expects the following arguments:

// A FHiCL parameter set which contains within it a table defining a
// transfer plugin

// The name of that table

// The send/receive role of the plugin

#include "CommanderInterface.hh"

#include "fhiclcpp/fwd.h"

#include <memory>
#include <string>

namespace sbndqm_artdaq
{
	/**
	 * \brief Load a CommanderInterface plugin
	 * \param pset ParameterSet used to configure the CommanderInterface
	 * \param commandable sbndqm_artdaq::Commandable object to send transition commands to
	 * \return Pointer to the new CommanderInterface instance
	 */
	std::unique_ptr<CommanderInterface>
	MakeCommanderPlugin(const fhicl::ParameterSet& pset,
						sbndqm_artdaq::Commandable& commandable);
}

#endif
