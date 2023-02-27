//Macro to create png files out of the histograms found in the RunRawDecoderTFile which is created after the OnlinePlotter is ran.
//
//

void drawHisto(const std::string& rootFile) {
  string filename = rootFile+"_RunRawDecoderTFile.root";//Creating file to open from input name of original root file from run sans .root 
  string plotIWant = "Run2_MeanADC";
  string nameAbrev = rootFile.substr(rootFile.length()-5,rootFile.length()-4);	
  bool justMean = true;//if true only produce plot for the plotIWant defined above.
  if (!justMean) gROOT->SetBatch(kTRUE);
  // Open the ROOT file and get the directory
  TFile* file = new TFile(filename.c_str());
  TDirectory* dir = (TDirectory*) file->Get("monitor");

  // Loop over all keys in the directory
  TIter next(dir->GetListOfKeys());
  TKey* key;
  while ((key = (TKey*) next())) {
    TObject* obj = key->ReadObj();
    if (obj->IsA()->InheritsFrom("TH1")) {//Stuff to do for all plots
      TH1* histo = (TH1*) obj;
      histo->GetXaxis()->SetTitle("Channels");
      histo->GetYaxis()->SetTitle("Modules");     
      histo->GetXaxis()->SetRangeUser(1,130000);
      histo->GetXaxis()->SetLabelSize(0.03);
      histo->GetYaxis()->SetLabelSize(0.03);
      histo->GetXaxis()->SetTitleSize(0.04);
      histo->GetYaxis()->SetTitleSize(0.04);
      histo->GetXaxis()->SetTickLength(0.02);
      histo->GetYaxis()->SetTickLength(0.02);
      //histo->SetRightMargin(0.2); 
      histo->GetXaxis()->CenterTitle();
      histo->GetYaxis()->CenterTitle();
      if (!justMean || std::string(histo->GetName())==plotIWant){
        // Create a canvas and draw the histogram
        TCanvas* canvas = new TCanvas("canvas", "My Canvas", 800, 600);
	if (obj->IsA()->InheritsFrom("TH2")){//stuff to do if the plot is 2D
 	   histo->GetYaxis()->SetRangeUser(1, 7);
	   histo->Draw("colz");
           histo->GetZaxis()->SetTitle("");
           //TColorBar *color_bar = (TColorBar*) histo->GetListOfFunctions()->FindObject("palette");
           // Set the right margin of the color bar to 0.2
           //color_bar->SetPad(0.9, 0.1, 1.0, 0.9);
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
