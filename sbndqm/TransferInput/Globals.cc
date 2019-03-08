#include "Globals.hh"

int sbndqm_artdaq::Globals::my_rank_ = -1;
std::unique_ptr<sbndqm_artdaq::PortManager> sbndqm_artdaq::Globals::portMan_ = std::make_unique<sbndqm_artdaq::PortManager>();
std::string sbndqm_artdaq::Globals::app_name_ = "";
int sbndqm_artdaq::Globals::partition_number_ = -1;

std::mutex sbndqm_artdaq::Globals::mftrace_mutex_;
std::string sbndqm_artdaq::Globals::mftrace_iteration_ = "Booted";
std::string sbndqm_artdaq::Globals::mftrace_module_ = "DAQ";
