#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageIOBase.h"

#include <cstring>

int main(int argc, char* argv[])
{
    std::string file_path("/nrs/aic/instruments/llsm/pipeline-test/no-deskew/2020-03-11/4-TL10min/642-4ms-20p_488-10ms-20p_25um_CamA_ch1_CAM1_stack0000_488nm_0000000msec_0023277664msecAbs_000x_000y_000z_0000t.tif");

    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(file_path.c_str(), itk::CommonEnums::IOFileMode::ReadMode);

    imageIO->SetFileName(file_path.c_str());
    imageIO->ReadImageInformation();

    const itk::IOPixelEnum pixelType = imageIO->GetPixelType();

    std::cout << "Pixel Type is " << itk::ImageIOBase::GetPixelTypeAsString(pixelType) << std::endl;

    return EXIT_SUCCESS;
}