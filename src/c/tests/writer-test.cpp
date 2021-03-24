#include "defines.h"
#include "utils.h"
#include "reader.h"
#include "writer.h"

#include <itkImage.h>
#include <itkImageBase.h>
#include <itkImageFileWriter.h>
// #include <itkMinimumMaximumImageCalculator.h>

int main()
{
  std::string input_file_name("/nrs/aic/instruments/llsm/pipeline-test/no-deskew/2020-03-11/4-TL10min/642-4ms-20p_488-10ms-20p_25um_CamA_ch1_CAM1_stack0000_488nm_0000000msec_0023277664msecAbs_000x_000y_000z_0000t.tif");

  itk::SmartPointer<kImageType> image = ReadImageFile(input_file_name);

  using PixelTypeOut = unsigned short;
  using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;

  WriteImageFile<kImageType,ImageTypeOut>(image,"test.tif");

  return EXIT_SUCCESS;
}