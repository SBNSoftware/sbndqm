#include "sbndqm/TransferInput/ArtdaqInput.hh"
#include "sbndqm/TransferInput/TransferWrapper.hh"

namespace art
{
	/**
	 * \brief Trait definition (must precede source typedef).
	 */
	template <>
	struct Source_generator<ArtdaqInput<sbndqm_artdaq::TransferWrapper>>
	{
		static constexpr bool value = true; ///< Dummy variable
	};

	/**
	 * \brief TransferInput is an art::Source using the artdaq::TransferWrapper class as the data source
	 */
	typedef art::Source<ArtdaqInput<sbndqm_artdaq::TransferWrapper>> TransferInput;
} // namespace art

DEFINE_ART_INPUT_SOURCE(art::TransferInput)
