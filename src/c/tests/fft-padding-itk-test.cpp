#include "decon.h"
#include <itkProgressAccumulator.h>
#include <cassert>
#include <iostream>

// Exercise ITK's padding itself so a matching mistake in the planner and its
// arithmetic tests cannot silently reintroduce the allocation overflow.
class PaddingProbe : public itk::FFTConvolutionImageFilter<kImageType>
{
public:
    using Self = PaddingProbe;
    using Pointer = itk::SmartPointer<Self>;
    itkNewMacro(Self);

    kImageType::SizeType PaddedSize(kImageType::Pointer image,
                                   kImageType::Pointer kernel, int prime_factor)
    {
        SetInput(image);
        SetKernelImage(kernel);
        SetSizeGreatestPrimeFactor(prime_factor);
        GetOutput()->SetRegions(image->GetLargestPossibleRegion());
        auto progress = itk::ProgressAccumulator::New();
        progress->SetMiniPipelineFilter(this);
        InternalImagePointerType padded;
        PadInput(image, padded, progress, 1.0f);
        return padded->GetLargestPossibleRegion().GetSize();
    }
};

kImageType::Pointer MakeImage(const llsm::FFTSize &size, bool allocate = true)
{
    auto image = kImageType::New();
    kImageType::SizeType itk_size;
    for (unsigned int axis = 0; axis < 3; ++axis)
        itk_size[axis] = size[axis];
    image->SetRegions(itk_size);
    if (allocate)
    {
        image->Allocate();
        image->FillBuffer(1.0);
    }
    return image;
}

int main()
{
    const llsm::FFTSize size{{8, 8, 8}};
    const auto image = MakeImage(size);
    for (const llsm::FFTSize kernel_size : {llsm::FFTSize{{3, 3, 9}},
                                           llsm::FFTSize{{3, 3, 10}},
                                           llsm::FFTSize{{2, 4, 6}}})
    {
        for (int prime_factor : {0, 13})
        {
            const auto kernel = MakeImage(kernel_size);
            auto probe = PaddingProbe::New();
            const auto actual = probe->PaddedSize(image, kernel, prime_factor);
            const auto plan = llsm::PlanFFTPadding(size, kernel_size, prime_factor);
            for (unsigned int axis = 0; axis < 3; ++axis)
                assert(actual[axis] == plan.optimized_size[axis]);
        }
    }

    // No large allocation: verify the actual decon guard rejects the former
    // failing block and disables only the optional rounding for the new one.
    using Filter = itk::RichardsonLucyDeconvolutionImageFilter<kImageType>;
    const auto kernel = MakeImage({{25, 25, 10}}, false);
    bool rejected = false;
    try
    {
        ConfigureFFTPadding<Filter>(Filter::New(), MakeImage({{4853, 1536, 273}}, false), kernel);
    }
    catch (const std::runtime_error &)
    {
        rejected = true;
    }
    assert(rejected);
    auto filter = Filter::New();
    ConfigureFFTPadding<Filter>(filter, MakeImage({{4853, 1536, 272}}, false), kernel);
    assert(filter->GetSizeGreatestPrimeFactor() == 0);
    std::cout << "ITK padding regression tests passed" << std::endl;
}
