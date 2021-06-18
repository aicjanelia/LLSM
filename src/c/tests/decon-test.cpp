#include "defines.h"
#include "reader.h"
#include "writer.h"
#include "deskew.h"
#include "resampler.h"
#include "decon.h"
#include "utils.h"
#include "math_local.h"

#include <chrono>
#include <itkSubtractImageFilter.h>
#include <itkClampImageFilter.h>

int main()
{
    using PixelTypeOut = unsigned short;
    using ImageTypeOut = itk::Image<PixelTypeOut, kDimensions>;
    auto start = std::chrono::high_resolution_clock::now();
    // read image
    std::string image_path("examples/cell2_ch1_CAM1_stack0001_488nm_0004529msec_0009990533msecAbs_000x_000y_000z_0000t.tif");
    kImageType::Pointer image = ReadImageFile<kImageType>(image_path, true);

    // kImageType::Pointer image_sub = ConvertImage<ImageTypeOut,kImageType>(clampFilter->GetOutput());

    // read PSF
    std::string psf_path("examples/488_PSF_piezoScan.tif");
    kImageType::Pointer psf = ReadImageFile<kImageType>(psf_path, true);

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

    // parameters
    double xy_res = 0.104;
    double stage_move_distance = 0.4;
    double angle = 31.8;
    float fill_value = 0.0;//100.0 / std::numeric_limits<unsigned short>::max();

    // deskew
    kImageType::Pointer deskew_img = Deskew(image, angle, stage_move_distance, xy_res, fill_value, true);
    image = NULL; // this will clean up the memory
    WriteImageFile<kImageType,ImageTypeOut>(deskew_img,"deskew-in-decon-test.tif", true);

    auto sub_image = SubtractConstantClamped(deskew_img, 100.0 / std::numeric_limits<unsigned short>::max());

    // parameters
    double psf_z_res = 0.4;

    kImageType::SpacingType psf_spacing = deskew_img->GetSpacing();
    printf("Deskew spacing: %0.3f, %0.3f, %0.3f\n", psf_spacing[0], psf_spacing[1], psf_spacing[2]);

    psf_spacing[2] = psf_z_res;
    psf->SetSpacing(psf_spacing);

    kImageType::Pointer psf_resampled = Resampler(psf, deskew_img->GetSpacing(), true);

    WriteImageFile<kImageType,ImageTypeOut>(psf_resampled,"deskew-psf-resampled.tif", true);
    
    kImageType::Pointer decon_img = RichardsonLucy(sub_image, psf_resampled, 5, true);

    // write image
    decon_img->SetSpacing(deskew_img->GetSpacing());
    WriteImageFile<kImageType, ImageTypeOut>(decon_img,"decon-test.tif", true);

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = stop - start;

    printf("Decon test took: %2ld:%2ld:%2ld (HH:MM:SS)\n",
        std::chrono::duration_cast<std::chrono::hours>(duration).count(),
        std::chrono::duration_cast<std::chrono::minutes>(duration).count(),
        std::chrono::duration_cast<std::chrono::seconds>(duration).count());

    return EXIT_SUCCESS;
}
