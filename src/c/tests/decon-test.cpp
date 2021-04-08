#include "defines.h"
#include "reader.h"
#include "writer.h"
#include "deskew.h"
#include "resampler.h"
#include "decon.h"

int main()
{
    // read image
    std::string image_path("examples/cell2_ch1_CAM1_stack0001_488nm_0004529msec_0009990533msecAbs_000x_000y_000z_0000t.tif");
    itk::SmartPointer<kImageType> image = ReadImageFile(image_path, true);

    // read PSF
    std::string psf_path("examples/488_PSF_piezoScan.tif");
    itk::SmartPointer<kImageType> psf = ReadImageFile(psf_path, true);

    if (image == nullptr)
    {
        std::cerr << "Unable to read " << image_path.c_str() << std::endl;
        return EXIT_FAILURE;
    }

    if (psf == nullptr)
    {
        std::cerr << "Unable to read " << psf_path.c_str() << std::endl;
        return EXIT_FAILURE;
    }

    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;

    // parameters
    double xy_res = 0.104;
    double stage_move_distance = 0.4;
    double angle = 31.8;
    float fill_value = 100.0 / std::numeric_limits<unsigned short>::max();

    // deskew
    itk::SmartPointer<kImageType> deskew_img = Deskew(image, angle, stage_move_distance, xy_res, fill_value, true);
    image = NULL; // this will clean up the memory
    WriteImageFile<kImageType,ImageTypeOut>(deskew_img,"deskew-in-decon-test.tif", true);

    // parameters
    double psf_z_res = 0.4;

    kImageType::SpacingType psf_spacing = deskew_img->GetSpacing();
    psf_spacing[2] = psf_z_res;
    psf->SetSpacing(psf_spacing);

    itk::SmartPointer<kImageType> psf_resampled = Resampler(psf, deskew_img->GetSpacing(), true);
    
    itk::SmartPointer<kImageType> decon_img = RichardsonLucy(deskew_img, psf_resampled, 100, true);

    // write image
    decon_img->SetSpacing(deskew_img->GetSpacing());
    WriteImageFile<kImageType, ImageTypeOut>(decon_img,"decon-test.tif", true);

    return EXIT_SUCCESS;
}
