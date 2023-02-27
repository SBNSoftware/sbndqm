//File: OnlinePlotter.cpp
//Brief: An "algorithm" class for creating online monitoring plots in a way that works 
//       either with or without LArSoft.  Should start out by plotting diagnostics from 
//       perl plotting code, then branch out as we identify more pathologies.  
//Author: Andrew Olivier aolivier@ur.rochester.edu

#ifndef CRT_ONLINEPLOTTER_CPP
#define CRT_ONLINEPLOTTER_CPP

//crt-core includes
#include "DATA/CRTTrigger.h"

//ROOT includes
#include "TH2D.h"
#include "TProfile.h"
#include "TProfile2D.h"

//c++ includes
#include <unordered_map> 
#include <string>
#include <iostream> //TODO: Remove me

namespace CRT
{
  template <class TFS> 
  //TFS is an object that defines a function like:
  //
  //template <class HIST, class ARGS...> HIST* make(ARGS... args)
  //
  //Looks like ART's TFileService.  
  class OnlinePlotter
  {
    public:
      
      OnlinePlotter(TFS& tfs, const double tickLength = 16): fFileService(tfs), fWholeJobPlots(tfs->mkdir("PerJob")), 
                                                             fRunStartTime(std::numeric_limits<uint64_t>::max()), 
                                                             fRunStopTime(0), fStartTotalTime(std::numeric_limits<uint64_t>::max()), 
                                                             fRunCounter(0),
                                                              fModuleToUSB ({{0, 13}, 
                                                                             {1, 13},
                                                                             {2, 13},
                                                                             {3, 13},
                                                                             {4, 13},
                                                                             {5, 13},
                                                                             {6, 13},
                                                                             {7, 13},
                                                                             {8, 14},
                                                                             {9, 14},
                                                                             {10, 14},
                                                                             {11, 14},
                                                                             {12, 14}, 
                                                                             {13, 14},
                                                                             {14, 14},
                                                                             {15, 14},
                                                                             {16, 3},
                                                                             {17, 3},
                                                                             {18, 3},
                                                                             {19, 3},
                                                                             {20, 22},
                                                                             {21, 22},
                                                                             {22, 22},
                                                                             {23, 22},
                                                                             {24, 22},
                                                                             {25, 22},
                                                                             {26, 22},
                                                                             {27, 22},
                                                                             {28, 3},
                                                                             {29, 3},
                                                                             {30, 3},
                                                                             {31, 3}}), fClockTicksToNs(tickLength)
      {
         //TODO: Get unordered_mapping from module to USB from some parameter passed to constructor
         //TODO: Get tick length from parameter passed to constructor
      }

      virtual ~OnlinePlotter()
      {
        const auto deltaT = (fRunStopTime - fStartTotalTime)*fClockTicksToNs*1.e-9;
        if(deltaT > 0)
        {
          fWholeJobPlots.fMeanRate->Scale(1./deltaT);
          fWholeJobPlots.fMeanRatePerBoard->Scale(1./deltaT);
        }
      }

      void ReactEndRun(const std::string& /*fileName*/)
      {
        //Scale all rate histograms here with total elapsed time in run
        const auto deltaT = (fRunStopTime-fRunStartTime)*1e-9*fClockTicksToNs; //Convert ticks from timestamp into seconds
                                                                   //TODO: Use tick length from constructor
        const auto totalDeltaTInSeconds = (fRunStopTime - fStartTotalTime)*fClockTicksToNs*1.e-9; //TODO: replace with tick length from constructor
        if(deltaT > 0) 
        {
          //TODO: Remove cout for LArSoft compatibility
          std::cout << "Elapsed time for this run is " << deltaT << " seconds.  Total elapsed time is " << totalDeltaTInSeconds << " seconds.\n";
          std::cout << "Beginning of all time was " << (uint64_t)(fStartTotalTime*fClockTicksToNs*1.e-9) << ", beginning of run was " 
                    << (uint64_t)(fRunStartTime*fClockTicksToNs*1.e-9) << ", and end of run was " << (uint64_t)(fRunStopTime*fClockTicksToNs*1.e-9) << "\n";
          const auto timeInv = 1./deltaT;
          fCurrentRunPlots->fMeanRate->Scale(timeInv); 
          fCurrentRunPlots->fMeanRatePerBoard->Scale(timeInv);
        }

        //Fill histograms that profile over board or USB number
        const auto nChannels = fCurrentRunPlots->fMeanRate->GetXaxis()->GetNbins();
        const auto nModules = fCurrentRunPlots->fMeanRate->GetYaxis()->GetNbins();
        std::unordered_map<unsigned int, size_t> usbToHits; //USB bins of number of hits
        for(auto channel = 0; channel < nChannels; ++channel)
        {
          for(auto module = 0; module < nModules; ++module)
          {
            const auto count = fCurrentRunPlots->fMeanRate->GetBinContent(fCurrentRunPlots->fMeanRate->GetBin(channel, module));

            const auto foundUSB = fModuleToUSB.find(module);
            if(foundUSB != fModuleToUSB.end()) 
            {
              const auto usb = foundUSB->second;
              auto found = fUSBToForeverPlots.find(usb);
              if(found == fUSBToForeverPlots.end())
              {
                found = fUSBToForeverPlots.emplace(usb, fFileService->mkdir("USB"+std::to_string(usb))).first;
              }

              auto& forever = found->second;
              usbToHits[usb] += count;
              forever.fMeanADC->Fill(totalDeltaTInSeconds, 
                                     fCurrentRunPlots->fMeanADC->GetBinContent(fCurrentRunPlots->fMeanADC->GetBin(channel, module)));
            } //If found a USB for this module
          } //For each module in mean rate/ADC plots
        } //For each channel in mean rate/ADC plots
        if(deltaT > 0)
        {
          for(const auto bin: usbToHits)
          {
            const auto found = fUSBToForeverPlots.find(bin.first);
            if(found != fUSBToForeverPlots.end()) found->second.fMeanRate->Fill(totalDeltaTInSeconds, bin.second);
          }
        }
      }

      void ReactBeginRun(const std::string& /*fileName*/)
      {
        //const uint64_t totalDeltaTInSeconds = (fRunStopTime - fStartTotalTime)*fClockTicksToNs*1.e-9; //TODO: replace with tick length from constructor
        fCurrentRunPlots.reset(new PerRunPlots(fFileService->mkdir("Run"+std::to_string(++fRunCounter)))); 
        //TODO: The above directory name is not guaranteed to be unique, and art::TFileDirectory's only mechanism for 
        //      reacting to that situation seems to be catching a cet::exception from whenver the internal cd() method is 
        //      called.  I need to either find a way to deal with this problem or stop making plots per-file.  
        //      One way to deal with this is by going back to "run number" directory labels.    

        fRunStartTime = std::numeric_limits<uint64_t>::max();
        fRunStopTime = 0;
      }
      long long timestampDiff=0.0;// runstarttime as a long long
      double doubletimeDiff =0.0;//time difference as a double between the start time as a long long eg. timestampDiff and the timestamp per trigger
      //uint64_t oldTime =0;//USed for print debugging	
      void AnalyzeEvent(const std::vector<CRT::Trigger>& triggers) //Make plots from CRT::Triggers
      {
        for(const auto& trigger: triggers)//Loop over each of the events eg.triggers
        {
          const long long timestamp = trigger.Timestamp();
          double timestampLessRST = 0.0; //timestamp less than run start time in seconds AC:changed from uint64_t
	  if(timestamp > 1e16)
          {
            //Update time bounds based on this timestamp
            if((uint64_t)(timestamp) < fRunStartTime) 
            {
              //std::cout << "fRunStartTime=" << fRunStartTime << " is >= " << timestamp << ", so setting fRunStartTime.\n";
              fRunStartTime = timestamp;
	      timestampDiff = timestamp;
            }
            if((uint64_t)(timestamp) > fRunStopTime) 
            {
              //std::cout << "fRunStopTime=" << fRunStopTime << " is <= " << timestamp << ", so setting fRunStopTime.\n";
              fRunStopTime = timestamp;
            }
            if((uint64_t)(timestamp) < fStartTotalTime) 
            {
              //std::cout << "fStartTotalTime=" << fStartTotalTime << " is >= " << timestamp << ", so setting fStartTotalTime.\n";
              fStartTotalTime = timestamp;
            }
	    //fClockTicksToNs = 16 this is the tick correction factor.	
            timestampLessRST = ((uint64_t)(timestamp)-fRunStartTime)*fClockTicksToNs*1.e-9;//Calculate timestamp - run start time.           
	    //uint64_t timeDiff = (uint64_t)(timestampLessRST)-oldTime;//Helps to just print out whenever we get a new timestamp
	    //if (timeDiff>1){
	    //oldTime = (uint64_t)(timestampLessRST);
	    //doubletimeDiff = Difference in time between timestamp and timetampDiff(which is the runStartTime as a long long) multiplied by tick
	    //conversion and turned from nanosecond into millisecond.
	    doubletimeDiff = (timestamp-timestampDiff)*1.e-6*16.;
	    std::cout <<"TimestampLessRST: " <<(uint64_t)(timestampLessRST)		      
		      <<" Timestamp times tick: "<<(uint64_t)(timestamp*fClockTicksToNs*1.e-9)
	    	      <<" RunStartTime Tick divided: "<< (uint64_t)(fRunStartTime*fClockTicksToNs*1.e-9)<<'\n'
		      <<" RunStartTime Not Tick divided: "<<(uint64_t)(fRunStartTime)<<'\n'
	              <<" Timestamp unshifted (ticks) : " << (timestamp)  <<'\n'        
	              //<<" timestampDiff: " << (timestampDiff)  <<'\n'        
	              <<" Timestamp difference (ns) : " << (timestamp-timestampDiff)*16.  <<'\n'
	              <<" Timestamp difference (ms) : " << doubletimeDiff  <<'\n';
	    //}
	    
	    const auto module = trigger.Channel();
            const auto& hits = trigger.Hits();             	    	
 	    fCurrentRunPlots->fTimestampPerModule->Fill(doubletimeDiff,module);
	    fCurrentRunPlots->fTimestamp->Fill(doubletimeDiff);
            int mapping_array[64] = {};//
	    int count = 1;
	    for(int i = 64; i>=1; i--){
		mapping_array[i] = count;
		count++;
	    }	
	    for(const auto& hit: hits)//Loop over each hit in each trigger.
            {
              const auto channel = mapping_array[hit.Channel()];//Changed from just being = to hit.Channel()	      	
	      //AC timestamp plot
	      //fCurrentRunPlots->fTimestamp->Fill(doubletimeDiff);
              //std::cout<<"TS-RunStartTime: "<<timestamp-fRunStartTime<<'\n'; 
              fCurrentRunPlots->fMeanRate->Fill(channel, module); 
              fCurrentRunPlots->fMeanADC->Fill(channel, module, hit.ADC());
              fCurrentRunPlots->fMeanADCPerBoard->Fill(module, hit.ADC());

              fWholeJobPlots.fMeanRate->Fill(channel, module);
              fWholeJobPlots.fMeanADC->Fill(channel, module, hit.ADC());
              fWholeJobPlots.fMeanADCPerBoard->Fill(module, hit.ADC());
            }
            fCurrentRunPlots->fMeanRatePerBoard->Fill(module);
            fWholeJobPlots.fMeanRatePerBoard->Fill(module);
          } //If UNIX timestamp is not 0
        }//Loop over triggers
        //std::cout<< "Start time: "<<fRunStartTime*1.e-9<<'\n';
        //std::cout<< "Total runtime: "<<(fRunStopTime-fRunStartTime)*1.e-9<<'\n';
      }
    private:
      //Keep track of the current run number
      size_t fRunNum;

      TFS fFileService; //Handle to tool for writing out files
      using DIRECTORY=decltype(fFileService->mkdir("test"));

      //Plots I want for each USB board for each Run
      struct PerRunPlots
      {
        PerRunPlots(DIRECTORY&& dir): fDir(dir)
        {
          fMeanRate = dir.template make<TH2D>("MeanRate", "Mean Rates;Channel;Module;Rate", 64, 0, 64, 32, 0, 32);
          fMeanRate->SetStats(false);          
          fTimestampPerModule = dir.template make<TH2D>("TimestampsPerModule", "TimeStamp;Timestamp-RunStartTime(s);Module", 50000, 0, 500000, 7, 0, 7);
          fTimestampPerModule->SetStats(false);          
          fTimestamp = dir.template make<TH1D> ("Timestamps","Timestamp",50000,0,500000) ;
          fMeanADC = dir.template make<TProfile2D>("MeanADC", "Mean ADC Values;Channel;Module;Rate [Hz]", 64, 0, 64, 32, 0, 32, 0., 4096);
          fMeanADC->SetStats(false);

          fMeanRatePerBoard = dir.template make<TH1D>("MeanRateBoard", "Mean Rate per Board;Board;Rate [Hz]", 
                                                       32, 0, 32);
          fMeanADCPerBoard = dir.template make<TProfile>("MeanADCBoard", "Mean ADC Value per Board;Board;ADC",
                                                         32, 0, 32);
        }

        ~PerRunPlots()
        {
          fMeanRate->SetMinimum(0);
          //fMeanRate->SetMaximum(60); //TODO: Maybe tune this one day.  When using the board reader, it really depends 
                                       //      on the trigger rate.  
        }

        DIRECTORY fDir; //Directory for the current run where these plots are kept
        TH2D* fMeanRate; //Plot of mean hit rate per channel 
        TProfile2D* fMeanADC; //Plot of mean ADC per channel
        TH1D* fMeanRatePerBoard; //Plot of mean rate for each board
        TProfile* fMeanADCPerBoard; //Plot of mean ADC per board
	TH1D* fTimestamp; //Timestamps 
	TH2D* fTimestampPerModule; //Timestamps per Module
      };

      std::unique_ptr<PerRunPlots> fCurrentRunPlots; //Per run plots for the current run
      PerRunPlots fWholeJobPlots; //Fill some PerRunPlots with the whole job
      uint64_t fRunStartTime; //Earliest timestamp in run
      uint64_t fRunStopTime; //Latest timestamp in run
      uint64_t fStartTotalTime; //Earliest timestamp stamp in entire job
      size_t fRunCounter; //Number of times ReactBeginRun() has been called so far.  Used 
                          //to generate unique TDirectory names.  

      //Plots I want for each USB board integrated over all(?) Runs
      struct ForeverPlots
      {
        ForeverPlots(DIRECTORY&& dir): fDir(dir)
        {
          fMeanRate = dir.template make<TH1D>("MeanRateHistory", "Mean Rate History;Time [s];Rate [Hz]",
                                              1000, 0, 3600);
          fMeanADC  = dir.template make<TProfile>("MeanADCHistory", "Mean ADC History;Time [s];ADC", 
                                                  1000, 0, 3600);
        }

        DIRECTORY fDir; //Directory for this USB's overall plots
        TH1D* fMeanRate; //Mean hit rate in time
        TProfile* fMeanADC; //Mean hit ADC in time
      };

      std::unordered_map<unsigned int, ForeverPlots> fUSBToForeverPlots; //Mapping from USB number to ForeverPlots.  

      //Configuration parameters
      std::unordered_map<unsigned int, unsigned int> fModuleToUSB; //Mapping from module number to USB
      const double fClockTicksToNs; //Length of a clock tick in nanoseconds
  };
}

#endif //CRT_ONLINEPLOTTER_CPP
