#include "fft_size.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
    {
        const llsm::FFTSize image_size{{512, 1536, 2542}};
        const llsm::FFTSize kernel_size{{25, 25, 10}};
        const auto plan = llsm::PlanFFTPadding(image_size, kernel_size, 13);

        assert((plan.minimum_size == llsm::FFTSize{{536, 1560, 2552}}));
        assert((plan.optimized_size == llsm::FFTSize{{539, 1560, 2560}}));
        assert(plan.minimum_voxel_count == UINT64_C(2133880320));
        assert(plan.optimized_voxel_count == UINT64_C(2152550400));
        assert(plan.minimum_size_supported);
        assert(plan.disable_extra_padding);

        const auto unrounded_plan = llsm::PlanFFTPadding(image_size, kernel_size, 0);
        assert(unrounded_plan.optimized_size == unrounded_plan.minimum_size);
        assert(unrounded_plan.optimized_voxel_count == UINT64_C(2133880320));
        assert(!unrounded_plan.disable_extra_padding);
    }

    {
        const llsm::FFTSize image_size{{64, 64, 64}};
        const llsm::FFTSize kernel_size{{25, 25, 25}};
        const auto plan = llsm::PlanFFTPadding(image_size, kernel_size, 13);

        assert((plan.minimum_size == llsm::FFTSize{{88, 88, 88}}));
        assert((plan.optimized_size == llsm::FFTSize{{88, 88, 88}}));
        assert(plan.minimum_size_supported);
        assert(!plan.disable_extra_padding);
    }

    {
        const llsm::FFTSize image_size{{512, 1536, 2600}};
        const llsm::FFTSize kernel_size{{25, 25, 10}};
        const auto plan = llsm::PlanFFTPadding(image_size, kernel_size, 13);

        assert(!plan.minimum_size_supported);
        assert(!plan.disable_extra_padding);
    }

    // PartFilesTest regression: the old estimate accepted this block after
    // disabling rounding, but ITK's actual padding exceeded INT32_MAX.
    {
        const llsm::FFTSize kernel{{25, 25, 10}};
        const auto unsafe = llsm::PlanFFTPadding({{4853, 1536, 273}}, kernel, 13);
        assert((unsafe.minimum_size == llsm::FFTSize{{4877, 1560, 283}}));
        assert(unsafe.minimum_voxel_count == UINT64_C(2153097960));
        assert(!unsafe.minimum_size_supported);
        assert(!unsafe.disable_extra_padding);

        const auto safe = llsm::PlanFFTPadding({{4853, 1536, 272}}, kernel, 13);
        assert(safe.minimum_voxel_count == UINT64_C(2145489840));
        assert(safe.minimum_size_supported);
        assert(safe.disable_extra_padding);
    }

    // The same padding rule applies independently to even X and Y kernels.
    assert((llsm::MinimumConvolutionSize({{8, 8, 8}}, {{2, 4, 6}}) ==
            llsm::FFTSize{{10, 12, 14}}));
    assert((llsm::MinimumConvolutionSize({{8, 8, 8}}, {{1, 1, 1}}) ==
            llsm::FFTSize{{8, 8, 8}}));

    const auto max = std::numeric_limits<std::uint64_t>::max();
    assert(llsm::MinimumConvolutionSize({{max - 2, 1, 1}}, {{2, 1, 1}})[0] == max);
    bool overflow_rejected = false;
    try
    {
        (void)llsm::MinimumConvolutionSize({{max - 1, 1, 1}}, {{2, 1, 1}});
    }
    catch (const std::overflow_error &)
    {
        overflow_rejected = true;
    }
    assert(overflow_rejected);

    std::cout << "FFT size tests passed" << std::endl;
    return 0;
}
