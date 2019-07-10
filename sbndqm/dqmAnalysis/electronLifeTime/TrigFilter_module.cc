#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "sbndqm/dqmAnalysis/TPC/OnlineFilters/CRTData.hh"
#include "sbndqm/dqmAnalysis/TPC/OnlineFilters/CRTData.cxx"
namespace filt{

  class TrigFilter : public art::EDFilter {
  public:
    explicit TrigFilter(fhicl::ParameterSet const& pset);
    virtual ~TrigFilter() { }
    virtual bool filter(art::Event& e);
    void    reconfigure(fhicl::ParameterSet const& pset);

  private:

    std::string fCRTStripModuleLabel;
    std::vector<int>    fmodlistup;
    std::vector<int>    fmodlistdown;
    bool fstripmatch;
    float    fTimeCoinc;

  };

  TrigFilter::TrigFilter(fhicl::ParameterSet const& pset)
    : EDFilter(pset) //added by ace 12/05/2019                                                                                                                            
  {

    this->reconfigure(pset);
  }

  void TrigFilter::reconfigure(fhicl::ParameterSet const& pset)
  {

    fCRTStripModuleLabel = pset.get< std::string >("CRTStripModuleLabel","crt");
    fmodlistup = pset.get<std::vector<int>>("ModuleUpStream");
    fmodlistdown = pset.get<std::vector<int>>("ModuleDownStream");
    fstripmatch = pset.get<bool>("RequireStripMatch",true);
    fTimeCoinc = pset.get<float>("StripTimeCoincidence",0.2);
  }

  bool TrigFilter::filter(art::Event& e)
  {

    bool KeepMe = false;
    //       int event = e.id().event();                                                                                                                                   


    int nstr=0;
    art::Handle< std::vector<sbndqm::crt::CRTData> > crtStripListHandle;
    std::vector<art::Ptr<sbndqm::crt::CRTData> > striplist;
    if (e.getByLabel(fCRTStripModuleLabel, crtStripListHandle))  {
      //    if (e.getByLabel("crt", crtStripListHandle))  {                                                                                                                
      art::fill_ptr_vector(striplist, crtStripListHandle);
      nstr = striplist.size();
    }
    //    std::cout << "number of crt strips " << nstr << std::endl;                                                                                                       

    for (int i = 0; i<nstr-2; i+=2){
      if ((striplist[i]->ADC()+striplist[i+1]->ADC())>500.) {
        uint32_t chan1 = striplist[i]->Channel();
        int strip1 = (chan1 >> 1) & 15;
        int module1 = (chan1>> 5);
        //                                                                                                                                                                 
        uint32_t ttime1 = striplist[i]->T0();
        //  T0 in units of ticks, but clock frequency (16 ticks = 1 us) is wrong.                                                                                          
        // ints were stored as uints, need to patch this up                                                                                                                
        float ctime1 = ttime1/16.;
        if (ttime1 > 2147483648) {
          ctime1 = ((ttime1-4294967296)/16.);
        }
        //                                                                                                                                                                 
        for (int j = i+2; j<nstr; j+=2){
          if ((striplist[j]->ADC()+striplist[j+1]->ADC())>500.) {
	    bool match = false;
	    uint32_t chan2 = striplist[j]->Channel();
	    int strip2 = (chan2 >> 1) & 15;
	    int module2 = (chan2>> 5);
	    //                                                                                                                                                               
	    uint32_t ttime2 = striplist[j]->T0();
	    //  T0 in units of ticks, but clock frequency (16 ticks = 1 us) is wrong.                                                                                        
	    // ints were stored as uints, need to patch this up                                                                                                              
	    float ctime2 = ttime2/16.;
	    if (ttime2 > 2147483648) {
	      ctime2 = ((ttime2-4294967296)/16.);
	    }
	    //                                                                                                                                                               
	    float diff = abs(ctime1-ctime2);
	    //      if (diff<=fTimeCoinc && ctime1>-1300 && ctime1<1300) {                                                                                                   
	    if (diff<=fTimeCoinc ) {
	      // check if these are opposite modules                                                                                                                         
	      //      bool islip = false;                                                                                                                                    
	      for (int k=0;k<16;++k) {
		if ((fmodlistup[k]==module1 && fmodlistdown[k]==module2) ||
		    (fmodlistup[k]==module2 && fmodlistdown[k]==module1))
		  match=true;
	      }
	      if (match && fstripmatch) {
		// std::cout << diff << " " <<module1 << " " << strip1 << " " <<                                                                                               
		//   module2 << " " << strip2 << " " << ctime1 << " " << ctime2 << std::endl;                                                                                  
		if (strip1!=strip2) match=false;
		// if (fabs(strip1-strip2)>1.5) { match=false;                                                                                                               
		//        if (islip) { if ((strip1==1 && strip2==15) || (strip1==15 && strip2==1) ) match=true;}                                                             
		// }                                                                                                                                                         
		if ((module1==40 || module1==41 || module1==56 || module1==57) && strip1<12) match=false;
		if ((module1==46 || module1==47 || module1==62 || module1==63) && strip1>3) match=false;
	      }
	      if (match) {
		float xpos;
		if (module1<module2) xpos=((int(module1/2)-20)*16+strip1)*11.2-358.4;
		else xpos=((int(module2/2)-20)*16+strip2)*11.2-358.4-5.6;
		float dtime;
		if (xpos>0) dtime = ctime1+((200.0-xpos)/0.16);
		else dtime = ctime1+((200.0+xpos)/0.16);
		if (dtime<-200.0 || dtime>1500.0 ) match=false;
		// std::cout << "Event " << event << " " << module2 << " " << xpos << " " <<  ctime1 << " " <<                                                               
                // dtime << " " << match << std::endl;                                                                                                                     
	      }
	    }
	    if (match) {
	      //   std::cout << "Found One! Event " << event <<                                                                                                                
	      //     "   m/s1 m/s2 " << module1 << " " << strip1 << " " <<                                                                                                     
	      //     module2 << " " << strip2 << " " << ctime1 << " " <<ctime2 << std::endl;                                                                                   
	      KeepMe=true;
	    }
          }}
      }}
    return KeepMe;

  }

  // A macro required for a JobControl module.                                                                                                                             
  DEFINE_ART_MODULE(TrigFilter)
} // namespace filt 
