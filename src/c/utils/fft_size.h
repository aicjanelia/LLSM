#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace llsm
{

constexpr std::size_t kFFTDimensions = 3;
using FFTSize = std::array<std::uint64_t, kFFTDimensions>;

// The ITK/FFTW path used by decon fails with std::bad_array_new_length when
// the padded voxel count crosses the signed 32-bit boundary.
constexpr std::uint64_t kMaximumFFTVoxelCount =
    static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());

inline std::uint64_t CheckedVoxelCount(const FFTSize &size)
{
    std::uint64_t count = 1;
    for (const std::uint64_t dimension : size)
    {
        if (dimension == 0)
        {
            throw std::invalid_argument("FFT dimensions must be greater than zero");
        }
        if (count > std::numeric_limits<std::uint64_t>::max() / dimension)
        {
            throw std::overflow_error("FFT voxel count exceeds uint64_t");
        }
        count *= dimension;
    }
    return count;
}

inline bool HasNoPrimeFactorGreaterThan(std::uint64_t value,
                                        std::uint64_t greatest_prime_factor)
{
    if (greatest_prime_factor <= 1)
    {
        return true;
    }

    for (std::uint64_t factor = 2; factor <= greatest_prime_factor && value > 1; ++factor)
    {
        while (value % factor == 0)
        {
            value /= factor;
        }
    }
    return value == 1;
}

inline std::uint64_t NextFFTFriendlySize(std::uint64_t size,
                                         std::uint64_t greatest_prime_factor)
{
    if (greatest_prime_factor <= 1)
    {
        return size;
    }

    while (!HasNoPrimeFactorGreaterThan(size, greatest_prime_factor))
    {
        if (size == std::numeric_limits<std::uint64_t>::max())
        {
            throw std::overflow_error("FFT dimension exceeds uint64_t");
        }
        ++size;
    }
    return size;
}

inline FFTSize MinimumConvolutionSize(const FFTSize &image_size,
                                      const FFTSize &kernel_size)
{
    FFTSize size{};
    for (std::size_t dimension = 0; dimension < kFFTDimensions; ++dimension)
    {
        if (image_size[dimension] == 0 || kernel_size[dimension] == 0)
        {
            throw std::invalid_argument("Image and kernel dimensions must be greater than zero");
        }
        // ITK pads both sides by GetKernelRadius(), i.e. floor(kernel / 2).
        // For an even kernel this is one voxel more than kernel - 1.
        const std::uint64_t padding = 2 * (kernel_size[dimension] / 2);
        if (image_size[dimension] >
            std::numeric_limits<std::uint64_t>::max() - padding)
        {
            throw std::overflow_error("Convolution dimension exceeds uint64_t");
        }
        size[dimension] = image_size[dimension] + padding;
    }
    return size;
}

inline FFTSize OptimizedConvolutionSize(const FFTSize &minimum_size,
                                        std::uint64_t greatest_prime_factor)
{
    FFTSize size{};
    for (std::size_t dimension = 0; dimension < kFFTDimensions; ++dimension)
    {
        size[dimension] = NextFFTFriendlySize(minimum_size[dimension],
                                              greatest_prime_factor);
    }
    return size;
}

struct FFTPaddingPlan
{
    FFTSize minimum_size;
    FFTSize optimized_size;
    std::uint64_t minimum_voxel_count;
    std::uint64_t optimized_voxel_count;
    bool minimum_size_supported;
    bool disable_extra_padding;
};

inline FFTPaddingPlan PlanFFTPadding(const FFTSize &image_size,
                                     const FFTSize &kernel_size,
                                     std::uint64_t greatest_prime_factor)
{
    FFTPaddingPlan plan{};
    plan.minimum_size = MinimumConvolutionSize(image_size, kernel_size);
    plan.optimized_size = OptimizedConvolutionSize(plan.minimum_size,
                                                   greatest_prime_factor);
    plan.minimum_voxel_count = CheckedVoxelCount(plan.minimum_size);
    plan.optimized_voxel_count = CheckedVoxelCount(plan.optimized_size);
    plan.minimum_size_supported = plan.minimum_voxel_count <= kMaximumFFTVoxelCount;
    plan.disable_extra_padding = plan.minimum_size_supported &&
                                 plan.optimized_voxel_count > kMaximumFFTVoxelCount;
    return plan;
}

} // namespace llsm
