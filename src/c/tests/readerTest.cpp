#include "defines.h"
#include "reader.h"

int main()
{
  std::string input_file_name("/nrs/aic/instruments/llsm/pipeline-test/no-deskew/2020-03-11/4-TL10min/642-4ms-20p_488-10ms-20p_25um_CamA_ch1_CAM1_stack0000_488nm_0000000msec_0023277664msecAbs_000x_000y_000z_0000t.tif");

  itk::SmartPointer<kImageType> image = ReadImageFile(input_file_name);

  if (image == nullptr)
  {
    std::cerr << "Unable to read " << input_file_name.c_str() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}