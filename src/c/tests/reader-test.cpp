#include "defines.h"
#include "reader.h"

int main()
{
  // read image
  std::string image_path("examples/cell2_ch0_CAM1_stack0001_560nm_0004529msec_0009990533msecAbs_000x_000y_000z_0000t.tif");
  kImageType::Pointer image = ReadImageFile<kImageType>(image_path, true);

  if (image == nullptr)
  {
    std::cerr << "Unable to read " << image_path.c_str() << std::endl;
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Success" << std::endl;
  }

  return EXIT_SUCCESS;
}
