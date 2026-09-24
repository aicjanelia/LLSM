#include "reader.h"
#include "writer.h"
#include <cassert>
#include <fstream>
#include <iostream>

int main()
{
    const auto directory = fs::temp_directory_path() / fs::unique_path("llsm-writer-test-%%%%-%%%%");
    fs::create_directory(directory);
    const auto path = directory / "result.tif";
    auto image = kImageType::New();
    image->SetRegions(kImageType::SizeType{{4, 3, 2}});
    image->Allocate();
    image->FillBuffer(0.5);
    kImageType::SpacingType spacing;
    spacing.Fill(1.0);

    // A failed fresh run leaves no final TIFF and cleans its staging files.
    {
        TiffStackBlockWriter writer(path.string(), 4, 3, 2, 16, spacing);
        writer.AppendCore(image, 0, 1);
        assert(!fs::exists(path));
        bool rejected = false;
        try { writer.Finish(); }
        catch (const std::runtime_error &) { rejected = true; }
        assert(rejected);
    }
    assert(fs::is_empty(directory));

    // Failed overwrite preserves the previous result even after some slices
    // have been written; successful overwrite publishes all expected slices.
    { std::ofstream previous(path.string()); previous << "previous result"; }
    try
    {
        TiffStackBlockWriter writer(path.string(), 4, 3, 2, 16, spacing);
        writer.AppendCore(image, 0, 1);
        throw std::runtime_error("simulated decon failure in a later block");
    }
    catch (const std::runtime_error &) {}
    { std::ifstream previous(path.string()); std::string line; std::getline(previous, line); assert(line == "previous result"); }

    for (unsigned int bits : {8, 16, 32})
    {
        {
            TiffStackBlockWriter writer(path.string(), 4, 3, 2, bits, spacing);
            writer.AppendCore(image, 0, 1);
            writer.AppendCore(image, 1, 1);
            writer.Finish();
            writer.Finish(); // Publishing a completed stack is idempotent.
        }
        TiffStackBlockReader reader(path.string());
        assert(reader.GetDepth() == 2);
        assert(reader.GetBitsPerSample() == bits);
        auto last = reader.ReadBlock(1, 1, spacing);
        assert(std::abs(last->GetBufferPointer()[0] - 0.5) < 0.005);
    }
    fs::remove(path);
    assert(fs::is_empty(directory));
    fs::remove(directory);
    std::cout << "Block writer publication tests passed" << std::endl;
}
