#ifndef ARTDAQ_DAQDATA_GLOBALS_HH
#define ARTDAQ_DAQDATA_GLOBALS_HH

#include <sstream>
#include "PortManager.hh"

#define my_rank sbndqm_artdaq::Globals::my_rank_
#define app_name sbndqm_artdaq::Globals::app_name_
#define portMan sbndqm_artdaq::Globals::portMan_
#define seedAndRandom() sbndqm_artdaq::Globals::seedAndRandom_()
#define GetPartitionNumber() sbndqm_artdaq::Globals::getPartitionNumber_()

#define GetMFIteration() sbndqm_artdaq::Globals::GetMFIteration_()
#define GetMFModuleName() sbndqm_artdaq::Globals::GetMFModuleName_()
#define SetMFModuleName(name) sbndqm_artdaq::Globals::SetMFModuleName_(name)
#define SetMFIteration(name) sbndqm_artdaq::Globals::SetMFIteration_(name)

//https://stackoverflow.com/questions/21594140/c-how-to-ensure-different-random-number-generation-in-c-when-program-is-execut
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <mutex>


/**
 * \brief The artdaq namespace
 */
namespace sbndqm_artdaq
{
	/**
	 * \brief The sbndqm_artdaq::Globals class contains several variables which are useful across the entire artdaq system
	 */
	class Globals
	{
	public:
		static int my_rank_; ///< The rank of the current application
		static std::unique_ptr<PortManager> portMan_; ///< A handle to PortManager
		static std::string app_name_; ///< The name of the current application, to be used in logging and metrics
		static int partition_number_; ///< The partition number of the current application

		static std::mutex mftrace_mutex_;
		static std::string mftrace_module_; ///< MessageFacility's module and iteration are thread-local, but we want to use them to represent global state in artdaq.
		static std::string mftrace_iteration_; ///< MessageFacility's module and iteration are thread-local, but we want to use them to represent global state in artdaq.

		/**
		 * \brief Seed the C random number generator with the current time (if that has not been done already) and generate a random value
		 * \return A random number.
		 */
		static uint32_t seedAndRandom_()
		{
			static bool initialized_ = false;
			if (!initialized_)
			{
				int fp = open("/dev/random", O_RDONLY);
				if (fp == -1) abort();
				unsigned seed;
				unsigned pos = 0;
				while (pos < sizeof(seed))
				{
					int amt = read(fp, (char *)&seed + pos, sizeof(seed) - pos);
					if (amt <= 0) abort();
					pos += amt;
				}
				srand(seed);
				close(fp);
				initialized_ = true;
			}
			return rand();
		}

		/**
		* \brief Get the current partition number, as defined by the ARTDAQ_PARTITION_NUMBER environment variable
		* \return The current partition number (defaults to 0 if unset, will be between 0 and 127)
		*/
		static int getPartitionNumber_()
		{
			uint32_t part_u = 0;

			// 23-May-2018, KAB: added the option to return the partition number data member
			// and gave it precedence over the env var since it is typcally based on information
			// that the user specified on the command line.
			if (partition_number_ >= 0)
			{
				part_u = static_cast<uint32_t>(partition_number_);
			}
			else
			{
				auto part = getenv("ARTDAQ_PARTITION_NUMBER"); // 0-127
				if (part != nullptr)
				{
					try
					{
						auto part_s = std::string(part);
						part_u = static_cast<uint32_t>(std::stoll(part_s, 0, 0));
					}
					catch (std::invalid_argument) {}
					catch (std::out_of_range) {}
				}
				partition_number_ = part_u & 0x7F;
			}

			return (part_u & 0x7F);
		}

		static std::string GetMFIteration_()
		{
			std::unique_lock<std::mutex> lk(mftrace_mutex_);
			return mftrace_iteration_;
		}

		static std::string GetMFModuleName_()
		{
			std::unique_lock<std::mutex> lk(mftrace_mutex_);
			return mftrace_module_;
		}

		static void SetMFIteration_(std::string name)
		{
			std::unique_lock<std::mutex> lk(mftrace_mutex_);
			mftrace_iteration_ = name;
		}

		static void SetMFModuleName_(std::string name)
		{
			std::unique_lock<std::mutex> lk(mftrace_mutex_);
			mftrace_module_ = name;
		}

		static void CleanUpGlobals()
		{
			portMan_.reset(nullptr);
		}
	};
}

#include "artdaq-core/Utilities/configureMessageFacility.hh"
#include "tracemf.h"
#include "artdaq-core/Utilities/TimeUtils.hh"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/TableFragment.h"
#include "fhiclcpp/types/ConfigurationTable.h"
#endif // ARTDAQ_DAQDATA_GLOBALS_HH
