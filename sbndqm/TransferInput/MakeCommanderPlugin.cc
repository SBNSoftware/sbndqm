#include "MakeCommanderPlugin.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"

#include "cetlib/BasicPluginFactory.h"
#include "fhiclcpp/ParameterSet.h"

#include <sstream>

using namespace artdaq;

namespace sbndqm_artdaq
{
	std::unique_ptr<sbndqm_artdaq::CommanderInterface>
	MakeCommanderPlugin(const fhicl::ParameterSet& commander_pset,
						sbndqm_artdaq::Commandable& commandable)
	{
		static cet::BasicPluginFactory bpf("commander", "make");

		try
		{
			auto commander =
				bpf.makePlugin<std::unique_ptr<CommanderInterface>,
				               const fhicl::ParameterSet&,
				              sbndqm_artdaq::Commandable&>(
					commander_pset.get<std::string>("commanderPluginType"),
					commander_pset,
					commandable);

			return commander;
		}
		catch (...)
		{
			std::stringstream errmsg;
			errmsg
				<< "Unable to create commander plugin using the FHiCL parameters \""
				<< commander_pset.to_string()
				<< "\"";
			ExceptionHandler(ExceptionHandlerRethrow::yes, errmsg.str());
		}

		return nullptr;
	}
}
