#include "itkImage.h"
#include <iostream>
int
main()
{
using ImageType = itk::Image<unsigned short, 3>;
ImageType::Pointer image = ImageType::New();
std::cout << "ITK Hello World !" << std::endl;
return EXIT_SUCCESS;
}