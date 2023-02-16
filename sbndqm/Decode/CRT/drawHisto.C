//Macro to create png files out of the histograms found in the RunRawDecoderTFile which is created after the OnlinePlotter is ran.
//
//

void drawHisto(const std::string& rootFile) {
  string filename = rootFile+"_RunRawDecoderTFile.root";//Creating file to open from input name of original root file from run sans .root 
  string plotIWant = "PerJob_MeanRate";
  string nameAbrev = rootFile.substr(rootFile.length()-5,rootFile.length()-4);	
  bool justMean = false;//if true only produce plot for the plotIWant defined above.
  if (!justMean) gROOT->SetBatch(kTRUE);
  // Open the ROOT file and get the directory
  TFile* file = new TFile(filename.c_str());
  TDirectory* dir = (TDirectory*) file->Get("monitor");

  // Loop over all keys in the directory
  TIter next(dir->GetListOfKeys());
  TKey* key;
  while ((key = (TKey*) next())) {
    TObject* obj = key->ReadObj();
    if (obj->IsA()->InheritsFrom("TH1")) {
      TH1* histo = (TH1*) obj;
      if (!justMean || std::string(histo->GetName())==plotIWant){
        // Create a canvas and draw the histogram
        TCanvas* canvas = new TCanvas("canvas", "My Canvas", 800, 600);
	if (obj->IsA()->InheritsFrom("TH2")){
	histo->GetYaxis()->SetRangeUser(1, 7);
	histo->Draw("colz");
	}
	else histo->Draw();
        // Save the canvas to a file in the plots folder
        canvas->SaveAs(("plots/" + nameAbrev + std::string(histo->GetName()) + ".png").c_str());
        // Clean up
        if (!justMean) delete canvas;
      }	

    }
    //delete obj;
  }
  // Clean up
  //delete dir;
  //delete file;
  gROOT->SetBatch(kFALSE);
}
