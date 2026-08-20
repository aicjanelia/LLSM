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

        assert((plan.minimum_size == llsm::FFTSize{{536, 1560, 2551}}));
        assert((plan.optimized_size == llsm::FFTSize{{539, 1560, 2560}}));
        assert(plan.minimum_voxel_count == UINT64_C(2133044160));
        assert(plan.optimized_voxel_count == UINT64_C(2152550400));
        assert(plan.minimum_size_supported);
        assert(plan.disable_extra_padding);

        const auto unrounded_plan = llsm::PlanFFTPadding(image_size, kernel_size, 0);
        assert(unrounded_plan.optimized_size == unrounded_plan.minimum_size);
        assert(unrounded_plan.optimized_voxel_count == UINT64_C(2133044160));
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

    std::cout << "FFT size tests passed" << std::endl;
    return 0;
}
