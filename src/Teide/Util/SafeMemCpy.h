
#include <cstddef>
#include <cstring>
#include <ranges>

namespace Teide
{

template <class Dest, class Source>
    requires(
        std::ranges::contiguous_range<Dest>
        && std::ranges::sized_range<Dest>
        &&
        // !std::ranges::constant_range<Dest> && // C++23
        std::ranges::contiguous_range<Source>
        && std::ranges::sized_range<Source>
        && !(
            sizeof(std::ranges::range_value_t<Dest>)
            % sizeof(std::ranges::range_value_t<Source>)
            != 0
            && sizeof(std::ranges::range_value_t<Source>)
            % sizeof(std::ranges::range_value_t<Dest>)
            != 0))
void SafeMemCpy(Dest&& dest, const Source& source)
{
    namespace sr = std::ranges;
    std::cout << sr::size(dest) << " * " << sizeof(sr::range_value_t<Dest>) << "\n";
    std::cout << sr::size(source) << " * " << sizeof(sr::range_value_t<Source>) << "\n";
    const auto destSize = static_cast<std::size_t>(sr::size(dest)) * sizeof(sr::range_value_t<Dest>);
    const auto sourceSize = static_cast<std::size_t>(sr::size(source)) * sizeof(sr::range_value_t<Source>);
    const auto size = std::min(destSize, sourceSize);
    std::memcpy(sr::data(dest), sr::data(source), size);
}

} // namespace Teide
