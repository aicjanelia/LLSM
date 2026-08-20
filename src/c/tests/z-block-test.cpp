#include "z_block.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
    assert(llsm::RequiredHaloDepth(10, 10) == 90);
    assert(llsm::RequiredHaloDepth(1, 10) == 0);

    const auto maximum_input_depth =
        llsm::MaximumInputDepthForFFT(512, 1536, 25, 25, 10);
    assert(maximum_input_depth == 2559);

    const auto core_depth =
        llsm::SelectCoreDepth(2542, 512, 90, maximum_input_depth);
    assert(core_depth == 512);
    assert(llsm::SelectCoreDepth(2542, 0, 90, maximum_input_depth) == 512);
    assert(llsm::SelectCoreDepth(100, 0, 90, maximum_input_depth) == 100);

    const auto blocks = llsm::MakeZBlocks(2542, core_depth, 90);
    assert(blocks.size() == 5);

    assert(blocks[0].core_begin == 0);
    assert(blocks[0].core_size == 512);
    assert(blocks[0].read_begin == 0);
    assert(blocks[0].read_size == 602);
    assert(blocks[0].core_offset == 0);

    assert(blocks[1].core_begin == 512);
    assert(blocks[1].core_size == 512);
    assert(blocks[1].read_begin == 422);
    assert(blocks[1].read_size == 692);
    assert(blocks[1].core_offset == 90);

    assert(blocks.back().core_begin == 2048);
    assert(blocks.back().core_size == 494);
    assert(blocks.back().read_begin == 1958);
    assert(blocks.back().read_size == 584);
    assert(blocks.back().core_offset == 90);

    std::uint64_t expected_core_begin = 0;
    for (const auto &block : blocks)
    {
        assert(block.core_begin == expected_core_begin);
        assert(block.core_offset + block.core_size <= block.read_size);
        expected_core_begin += block.core_size;
    }
    assert(expected_core_begin == 2542);

    const auto one_block = llsm::MakeZBlocks(100, 100, 90);
    assert(one_block.size() == 1);
    assert(one_block[0].read_begin == 0);
    assert(one_block[0].read_size == 100);

    const auto reduced_core =
        llsm::SelectCoreDepth(10000, 5000, 90, maximum_input_depth);
    assert(reduced_core == maximum_input_depth - 180);

    bool rejected_wide_plane = false;
    try
    {
        const auto no_depth =
            llsm::MaximumInputDepthForFFT(100000, 100000, 25, 25, 10);
        (void)llsm::SelectCoreDepth(100, 10, 90, no_depth);
    }
    catch (const std::runtime_error &)
    {
        rejected_wide_plane = true;
    }
    assert(rejected_wide_plane);

    std::cout << "Z block tests passed" << std::endl;
    return 0;
}
