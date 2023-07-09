// This decoder decodes binary file for nevis trigger board data 
//Written by Daisy Kalra following data format https://sbn-docdb.fnal.gov/cgi-bin/sso/RetrieveFile?docid=17726&filename=SBND_TPC_Data_Format_April_24_2020.pdf&version=1
//To run this code, g++ -std=gnu++11 -o bin decoder.cpp
// ./bin --> this will generate decoded output on screen. 


#include <iostream>
#include <fstream>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <map>
#include <tuple>
#include <vector>
#include <typeinfo>
#include <ctime>
#include <sys/time.h>
#include <cstdint>

#include <chrono>

using namespace std;

int main()
{
  std::ifstream binFile;

  //Recent file
  binFile.open("/data/sbndrawbin_run-00001_2023.06.26-09.20.51_NevisTB.dat", std::ios::binary);

  std::ofstream outfile;
  outfile.open("EventNTB.txt");


  if( !binFile.is_open() ){
    std::cerr << "ERROR: Could not open file " << std::endl;
    return 0;
  }


  int wordcnt=1; // to keep track of words
  int frame1, frame2, frame; // Frame is 24-bit word
  // uint32_t
  int  event1, event2, event; // Trigger or event number is 24-bit word
  int pmt;
  int   busy, cntlrtrig, ext, active, gate1, gate2, veto, calib; // 1-bit for all these markers
  //uint16_t 
  int sample2Mhz, sample16Mhz; // sample number is 12 bits.
  int  remainder16Mhz, remainder_1_64Mhz, remainder_2_64Mhz, remainder64Mhz; // 3 bits for 16MHz phase remainder and 2 bits for 64MHz phase remainder
  int sample64Mhz;
  int  Trigger_sample_number_64MHz;
  int  Trigger_sample_number_16MHz;
 
  while( binFile.peek() != EOF ){
    uint32_t word32b;
    binFile.read( reinterpret_cast<char*>(&word32b), sizeof(word32b) );

    std::cout.setf ( std::ios::hex, std::ios::basefield );  // set hex as the basefield                                                                          
    std::cout.setf ( std::ios::showbase ); // activate showbase    i.e with prefix 0x                                                                            

    //std::cout << "32 bit word: " << word32b << std::endl;

    uint16_t first16b = word32b & 0xffff; //right 16 bit word                                                    
    uint16_t last16b = (word32b>>16) & 0xffff; // left 16 bit word                                               


    //    std::cout << "Right 16 bits: " << first16b << std::endl;
    // std::cout << "Left  16 bits: " << last16b << std::endl;
   

    if (wordcnt==1) {
     
      busy = first16b & 0x1;
      remainder16Mhz =  (first16b>>1) & 0x3;
      sample2Mhz     =  (first16b>>4) & 0xfff;  

      //      std::cout << "2MHz sample: " << std::dec << sample2Mhz << std::endl;
      //std::cout << "Busy marker: "  << busy << std::endl;
      //std::cout << "Sample number remainder on 16MHz clock: " <<  std::dec << remainder16Mhz << std::endl; 

      Trigger_sample_number_16MHz = sample2Mhz*8 + remainder16Mhz*4;
      //std::cout << "Sample number in  16MHz precision:  " << std::dec << Trigger_sample_number_16MHz << std::endl;
     
      frame1 = last16b & 0xffff; // lowest 16 bits of frame number saving to frame1

      wordcnt=2;

    }//end of wordcnt 1    

    else if (wordcnt==2) {
      frame2= first16b &  0xff;  // upper 8 bits of frame number saving to frame2
      frame=frame2+frame1; // frame2 comes first as this has upper 8 bits and then add to frame 1 which has lower 16 bits
      event1 = (first16b>>8) & 0xff; // lower 8 bits of event or trigger number

      //      std::cout << "frame1: " << frame1 << " frame2 : " << frame2 << std::endl;
      //      std::cout << "Frame: " << std::dec << (frame & 0xffffff) << std::endl;

      event2 = last16b & 0xffff;  // upper 16 bits of event or trigger number

      //      event = event2+event1; // event2 comes first as this has upper 16 bits 
      event= event1+event2;
      //      std::cout << "Event1: " << event1 << "Event2: " << event2 << std::endl;
      std::cout << "Event: " << std::dec << event << std::endl;
      std::cout << "Frame: " << std::dec << (frame & 0xffffff) << std::endl;
      std::cout << "2MHz sample: " << std::dec << sample2Mhz << std::endl;     
      outfile << event << "\n";
      wordcnt=3;



    }

    else if (wordcnt==3) {

      pmt         = first16b & 0xff;
      cntlrtrig   = (first16b>>8) & 0x1;
      ext         = (first16b>>9) & 0x1;
      active      = (first16b>>10) & 0x1;
      gate2       = (first16b>>11) & 0x1;
      gate1       = (first16b>>12) & 0x1;
      veto        = (first16b>>13) & 0x1;
      calib       = (first16b>>14) & 0x1;

      //lower bit in  remainder_1_64Mhz
      remainder_1_64Mhz  = (first16b>>15) & 0x1;
      
      remainder_2_64Mhz  = last16b & 0x1; // upper bit of remainder
      remainder64Mhz = remainder_2_64Mhz+remainder_1_64Mhz;
      
      // std::cout << "{Placeholder} PMT data words: " << std::dec << pmt << std::endl;
      std::cout << "Controller trigger: "<< std::dec << cntlrtrig << std::endl;
      std::cout << "Calibration trigger: " << std::dec << calib  << std::endl;
      std::cout << "External trigger: " << std::dec << ext << std::endl;
      std::cout << "Gate1 (NuMI): " << std::dec << gate1 << std::endl;
      std::cout << "Gate2 (BNB): " << std::dec << gate2 << std::endl;
      std::cout << "Veto: " << std::dec << veto << std::endl;

      //  std::cout << "Sample number(64MHz clock): " << std::dec << remainder64Mhz << std::endl;
      

      Trigger_sample_number_64MHz = sample2Mhz*32 + remainder16Mhz*4 + remainder64Mhz;


      std::cout << "Sample number (64MHz precision): " << Trigger_sample_number_64MHz << std::endl;
      wordcnt=4;
    }

    else if(wordcnt==4 and word32b==0xffffffff){
      //std::cout<< "******************* End of Trigger/Event **********************" << std::endl;
      wordcnt=1;

    }


 
  }

  binFile.close();
  outfile.close();
}//end of main function
