#include "defines.h"
#include "reader.h"

int main()
{
  std::string input_file_name("skewed-image-c1.tif");

  itk::SmartPointer<kImageType> image = ReadImageFile(input_file_name);

  if (image == nullptr)
  {
    std::cerr << "Unable to read " << input_file_name.c_str() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
