#pragma once

#define DECON_VERSION "AIC Decon version 0.1.0"

#include "defines.h"
#include "utils.h"
#include <itkImage.h>
#include "itkSubtractImageFilter.h"
#include "itkDivideImageFilter.h"
#include "itkExtractImageFilter.h"
#include "itkPasteImageFilter.h"
#include "itkUnaryFunctorImageFilter.h"
#include "itkMinimumMaximumImageFilter.h"
#include <itkMultiThreaderBase.h>

#define FLATFIELD_VERSION "AIC flatfield correction version 0.1.0"

// Define a functor that clamps values to 0
namespace itk
{
    namespace Functor
    {
        template<typename TInput, typename TOutput>
        class ClampToZero
        {
        public:
            ClampToZero() {}
            ~ClampToZero() {}

            bool operator!=(const ClampToZero &) const
            {
                return false;
            }

            bool operator==(const ClampToZero &other) const
            {
                return !(*this != other);
            }

            inline TOutput operator()(const TInput &x) const
            {
                return static_cast<TOutput>((x < 0) ? 0 : x);
            }
        };
    }
}

kImageType::Pointer FlatfieldCorrection(kImageType::Pointer img, kSliceType::Pointer sub, kSliceType::Pointer div, bool verbose=false)
{
    kImageType::Pointer outputStack = kImageType::New();
    outputStack->SetRegions(img->GetLargestPossibleRegion());
    outputStack->SetSpacing(img->GetSpacing());
    outputStack->SetOrigin(img->GetOrigin());
    outputStack->SetDirection(img->GetDirection());
    outputStack->Allocate();
    outputStack->FillBuffer(0); // Initialize with zeros

    // Prepare to iterate over the slices in the stack
    kImageType::RegionType stackRegion = img->GetLargestPossibleRegion();
    kImageType::SizeType stackSize = stackRegion.GetSize();
    
    for (unsigned int i = 0; i < stackSize[2]; ++i)
    {
        // Define the slice to extract
        kImageType::IndexType start = stackRegion.GetIndex();
        start[2] = i;

        kImageType::SizeType size = stackSize;
        size[2] = 0;

        kImageType::RegionType desiredRegion(start, size);

        using ExtractFilterType = itk::ExtractImageFilter<kImageType, kSliceType>;
        ExtractFilterType::Pointer extractFilter = ExtractFilterType::New();
        extractFilter->SetExtractionRegion(desiredRegion);
        extractFilter->SetInput(img);
        extractFilter->SetDirectionCollapseToSubmatrix();
        try
        {
            extractFilter->Update();
        }
        catch (itk::ExceptionObject &err)
        {
            std::cerr << "extractFilter ExceptionObject caught: " << err << std::endl;
            throw;
        }
        kSliceType::Pointer currentSlice = extractFilter->GetOutput();

        // Subtract the single slice from the current slice
        using SubtractFilterType = itk::SubtractImageFilter<kSliceType, kSliceType, kSliceType>;
        SubtractFilterType::Pointer subtractFilter = SubtractFilterType::New();
        subtractFilter->SetInput1(currentSlice);
        subtractFilter->SetInput2(sub);
        try
        {
            subtractFilter->Update();
        }
        catch (itk::ExceptionObject &err)
        {
            std::cerr << "subtractFilter ExceptionObject caught: " << err << std::endl;
            throw;
        }

        kSliceType::Pointer subtractedSlice = subtractFilter->GetOutput();

        // Clamp the subtracted values at 0
        using ClampFilterType = itk::UnaryFunctorImageFilter<kSliceType, kSliceType, itk::Functor::ClampToZero<kPixelType, kPixelType>>;
        ClampFilterType::Pointer clampFilter = ClampFilterType::New();
        clampFilter->SetInput(subtractedSlice);

        try
        {
            clampFilter->Update();
        }
        catch (itk::ExceptionObject &err)
        {
            std::cerr << "ExceptionObject caught: " << err << std::endl;
            throw;
        }

        kSliceType::Pointer clampedSlice = clampFilter->GetOutput();

        using DivideFilterType = itk::DivideImageFilter<kSliceType, kSliceType, kSliceType>;
        DivideFilterType::Pointer divideFilter = DivideFilterType::New();
        divideFilter->SetInput1(clampedSlice);
        divideFilter->SetInput2(div);

        try
        {
            divideFilter->Update();
        }
        catch (itk::ExceptionObject &err)
        {
            std::cerr << "ExceptionObject caught: " << err << std::endl;
            throw;
        }

        kSliceType::Pointer dividedSlice2D = divideFilter->GetOutput();

        kImageType::Pointer dividedSlice = Convert2DImageTo3D<kPixelType>(dividedSlice2D);

        // Paste the clamped slice into the new output stack
        using PasteFilterType = itk::PasteImageFilter<kImageType, kImageType>;
        PasteFilterType::Pointer pasteFilter = PasteFilterType::New();
        pasteFilter->SetSourceImage(dividedSlice);
        pasteFilter->SetDestinationImage(outputStack);
        pasteFilter->SetSourceRegion(dividedSlice->GetLargestPossibleRegion());
        pasteFilter->SetDestinationIndex(start);

        try
        {
            pasteFilter->Update();
        }
        catch (itk::ExceptionObject &err)
        {
            std::cerr << "ExceptionObject caught: " << err << std::endl;
            throw;
        }

        outputStack = pasteFilter->GetOutput();
    }

    return outputStack;
}