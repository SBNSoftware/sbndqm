#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TImage.h"

#include "Image.h"
#include "Binary.h"

int sbndqm::SendHistogram(redisContext *redis, const std::string &key, TH1 *hist, size_t buffer_x, size_t buffer_y, size_t size_x, size_t size_y) {
  TCanvas *c = new TCanvas;
  hist->Draw();
  TImage *img = TImage::Create();
  img->FromPad(c, buffer_x, buffer_y, size_x, size_y);
  char *image_data = NULL;
  int image_len = -1;

  img->GetImageBuffer(&image_data, &image_len); 
  int ret = SaveBinary(redis, key, image_data, image_len);
  delete c; 
  delete img;
  return ret;
}

int sbndqm::SendHistogram(redisContext *redis, const std::string &key, TH2 *hist, size_t buffer_x, size_t buffer_y, size_t size_x, size_t size_y) {
  TCanvas *c = new TCanvas;
  hist->Draw();
  TImage *img = TImage::Create();
  img->FromPad(c, buffer_x, buffer_y, size_x, size_y);
  char *image_data = NULL;
  int image_len = -1;

  img->GetImageBuffer(&image_data, &image_len); 
  int ret = SaveBinary(redis, key, image_data, image_len);
  delete c; 
  delete img;
  return ret;
}

