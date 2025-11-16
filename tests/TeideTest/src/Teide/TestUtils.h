
#pragma once

#include "Teide/Visitable.h"
#include "Teide/Vulkan.h"
#include "Teide/VulkanDevice.h"

#include <gtest/gtest.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

inline bool g_windowless = false;

Teide::VulkanDevicePtr CreateTestDevice();

std::optional<std::uint32_t> GetTransferQueueIndex(vk::PhysicalDevice physicalDevice);
Teide::PhysicalDevice FindPhysicalDevice(vk::Instance instance);

std::vector<std::byte> HexToBytes(std::string_view hexString);

namespace Teide
{
void PrintTo(const Visitable auto& obj, std::ostream* os)
{
    obj.Visit([os]<typename Head, typename... Tail>(const Head& head, const Tail&... tail) {
        *os << '{';
        testing::internal::UniversalPrint(head, os);
        ((*os << ", ", testing::internal::UniversalPrint(tail, os)), ...);
        *os << '}';
    });
}

template <class T>
concept Enum = std::is_enum_v<T>;

void PrintTo(const Enum auto& v, std::ostream* os)
{
    *os << +std::to_underlying(v);
}

template <class T>
void PrintTo(const std::optional<T>& v, std::ostream* os)
{
    if (v.has_value())
    {
        *os << '{';
        testing::internal::UniversalPrint(v.value(), os);
        *os << '}';
    }
    else
    {
        *os << "nullopt";
    }
}

template <class T>
void PrintTo(const std::vector<T>& v, std::ostream* os)
{
    *os << '{';
    bool first = true;
    for (const T& elem : v)
    {
        if (!first)
        {
            *os << ", ";
        }
        first = false;
        testing::internal::UniversalPrint(elem, os);
    }
    *os << '}';
}

inline void PrintWithLabel(std::string_view label, const auto& v, std::ostream* os)
{
    *os << label << '=';
    testing::internal::UniversalPrint(v, os);
}

template <class T, Geo::Extent N, class Tag>
void PrintTo(const Geo::Vector<T, N, Tag>& v, std::ostream* os)
{
    *os << '{' << v << '}';
}

inline void PrintTo(const SamplerState& v, std::ostream* os)
{
    *os << '{';
    PrintWithLabel("magFilter", v.magFilter, os);
    *os << ", ";
    PrintWithLabel("minFilter", v.minFilter, os);
    *os << ", ";
    PrintWithLabel("mipmapMode", v.mipmapMode, os);
    *os << ", ";
    PrintWithLabel("addressModeU", v.addressModeU, os);
    *os << ", ";
    PrintWithLabel("addressModeV", v.addressModeV, os);
    *os << ", ";
    PrintWithLabel("addressModeW", v.addressModeW, os);
    *os << ", ";
    PrintWithLabel("maxAnisotropy", v.maxAnisotropy, os);
    *os << ", ";
    PrintWithLabel("compareOp", v.compareOp, os);
    *os << '}';
}

inline void PrintTo(const TextureData& v, std::ostream* os)
{
    *os << '{';
    PrintWithLabel("size", v.size, os);
    *os << ", ";
    PrintWithLabel("format", v.format, os);
    *os << ", ";
    PrintWithLabel("mipLevelCount", v.mipLevelCount, os);
    *os << ", ";
    PrintWithLabel("sampleCount", v.sampleCount, os);
    *os << ", ";
    PrintWithLabel("samplerState", v.samplerState, os);
    *os << ", ";
    PrintWithLabel("pixels", v.pixels, os);
    *os << '}';
}

inline void PrintTo(const RenderTargetInfo& value, ::std::ostream* os)
{
    *os << "size={" << value.size << "}, ";
    *os << "frameBufferLayout={";
    PrintTo(value.framebufferLayout, os);
    *os << "}";
}
} // namespace Teide

namespace std
{
inline void PrintTo(std::byte b, std::ostream* os)
{
    fmt::format_to(std::ostream_iterator<char>(*os), "0x{:02x}", static_cast<std::uint8_t>(b));
}

template <class T, std::size_t N>
void PrintTo(const std::span<T, N>& span, std::ostream* os)
{
    *os << "{";
    bool firstElem = true;
    for (const auto& elem : span)
    {
        if (!firstElem)
        {
            *os << ",";
        }
        firstElem = false;
        testing::internal::UniversalPrint(elem, os);
    }
    *os << "}";
}
} // namespace std

#ifdef NDEBUG
#    define EXPECT_UNREACHABLE(statement)                                                                              \
        if (false)                                                                                                     \
        {                                                                                                              \
            statement;                                                                                                 \
        }
#else
#    define EXPECT_UNREACHABLE(statement) EXPECT_THROW(statement, UnreachableError)
#endif

#define CONSTEXPR_EXPECT_EQ(a, b)                                                                                      \
    static_assert((a) == (b));                                                                                         \
    EXPECT_EQ(a, b);

#define CONSTEXPR_EXPECT_NE(a, b)                                                                                      \
    static_assert((a) != (b));                                                                                         \
    EXPECT_NE(a, b);

// Implements the polymorphic NotNull() matcher, which matches any raw or smart
// pointer that is not NULL.
class ValidVkHandleMatcher
{
public:
    template <typename Pointer>
    bool MatchAndExplain(const Pointer& p, testing::MatchResultListener* /* listener */) const
    {
        return static_cast<bool>(p);
    }

    static void DescribeTo(::std::ostream* os) { *os << "is a valid Vulkan handle"; }
    static void DescribeNegationTo(::std::ostream* os) { *os << "is not a valid Vulkan handle"; }
};

// Creates a polymorphic matcher that matches any non-null vulkan handle.
inline testing::PolymorphicMatcher<ValidVkHandleMatcher> IsValidVkHandle()
{
    return testing::MakePolymorphicMatcher(ValidVkHandleMatcher());
}
