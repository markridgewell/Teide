
#pragma once

#include "Teide/Visitable.h"
#include "Teide/Vulkan.h"
#include "Teide/VulkanGraphicsDevice.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

inline bool g_windowless = false;

Teide::VulkanGraphicsDevicePtr CreateTestGraphicsDevice();

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
        ((*os << ',', testing::internal::UniversalPrint(tail, os)), ...);
        *os << '}';
    });
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
#    define EXPECT_UNREACHABLE(statement) EXPECT_DEATH(statement, UnreachableMessage)
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
