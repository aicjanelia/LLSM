#pragma once

#include "fft_size.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace llsm
{

constexpr std::uint64_t kDefaultCoreDepth = 512;

struct ZBlock
{
    std::uint64_t core_begin;
    std::uint64_t core_size;
    std::uint64_t read_begin;
    std::uint64_t read_size;
    std::uint64_t core_offset;
};

inline std::uint64_t RequiredHaloDepth(std::uint64_t kernel_depth,
                                       std::uint64_t iterations)
{
    if (kernel_depth == 0)
    {
        throw std::invalid_argument("Kernel depth must be greater than zero");
    }
    const std::uint64_t support = kernel_depth - 1;
    if (support != 0 && iterations > std::numeric_limits<std::uint64_t>::max() / support)
    {
        throw std::overflow_error("Z halo depth exceeds uint64_t");
    }
    return support * iterations;
}

inline std::uint64_t MaximumInputDepthForFFT(std::uint64_t image_width,
                                             std::uint64_t image_height,
                                             std::uint64_t kernel_width,
                                             std::uint64_t kernel_height,
                                             std::uint64_t kernel_depth)
{
    const FFTSize one_slice_image{{image_width, image_height, 1}};
    const FFTSize kernel_size{{kernel_width, kernel_height, kernel_depth}};
    const FFTSize minimum_size = MinimumConvolutionSize(one_slice_image, kernel_size);

    if (minimum_size[0] > kMaximumFFTVoxelCount / minimum_size[1])
    {
        return 0;
    }
    const std::uint64_t padded_plane_voxels = minimum_size[0] * minimum_size[1];
    const std::uint64_t maximum_padded_depth =
        kMaximumFFTVoxelCount / padded_plane_voxels;
    if (maximum_padded_depth < kernel_depth)
    {
        return 0;
    }
    return maximum_padded_depth - kernel_depth + 1;
}

inline std::uint64_t SelectCoreDepth(std::uint64_t total_depth,
                                     std::uint64_t requested_core_depth,
                                     std::uint64_t halo_depth,
                                     std::uint64_t maximum_input_depth)
{
    if (total_depth == 0)
    {
        throw std::invalid_argument("Input depth must be greater than zero");
    }
    if (maximum_input_depth == 0)
    {
        throw std::runtime_error(
            "A single XY slice with the PSF exceeds the supported FFT size; Z splitting cannot process this image");
    }
    const std::uint64_t desired_core_depth =
        requested_core_depth == 0 ? kDefaultCoreDepth : requested_core_depth;
    if (total_depth <= maximum_input_depth && total_depth <= desired_core_depth)
    {
        return total_depth;
    }
    if (halo_depth > (maximum_input_depth - 1) / 2)
    {
        throw std::runtime_error(
            "The halo required by the PSF and iteration count leaves no room for a Z core; reduce iterations or split in XY as well");
    }

    const std::uint64_t maximum_core_depth = maximum_input_depth - 2 * halo_depth;
    return std::min(desired_core_depth, maximum_core_depth);
}

inline std::vector<ZBlock> MakeZBlocks(std::uint64_t total_depth,
                                       std::uint64_t core_depth,
                                       std::uint64_t halo_depth)
{
    if (total_depth == 0 || core_depth == 0)
    {
        throw std::invalid_argument("Input and core depths must be greater than zero");
    }

    std::vector<ZBlock> blocks;
    for (std::uint64_t core_begin = 0; core_begin < total_depth;)
    {
        const std::uint64_t core_size = std::min(core_depth, total_depth - core_begin);
        const std::uint64_t core_end = core_begin + core_size;
        const std::uint64_t read_begin = core_begin > halo_depth
                                             ? core_begin - halo_depth
                                             : 0;
        const std::uint64_t read_end = halo_depth < total_depth - core_end
                                           ? core_end + halo_depth
                                           : total_depth;
        blocks.push_back(ZBlock{
            core_begin,
            core_size,
            read_begin,
            read_end - read_begin,
            core_begin - read_begin});
        core_begin = core_end;
    }
    return blocks;
}

} // namespace llsm
