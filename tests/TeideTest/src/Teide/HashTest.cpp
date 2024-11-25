
#include "Teide/Hash.h"

#include "Teide/TestUtils.h"

template <typename T>
constexpr static auto DoHash(const T& value)
{
    return Teide::Hash<T>{}(value);
}

template <typename... Ts>
constexpr static auto HashMulti(const Ts&... args)
{
    auto seed = Teide::HashInitialSeed;
    (Teide::HashAppend(seed, args), ...);
    return seed;
}

struct Ints
{
    int a, b, c, d, e;
    constexpr void Visit(const auto& f) const { f(a, b, c, d, e); }
};

TEST(HashTest, HashingIntNotEqZero)
{
    constexpr int i = 0x04030201;
    CONSTEXPR_EXPECT_NE(DoHash(i), 0);
}

TEST(HashTest, HashingArrayOfIntsNotEqZero)
{
    constexpr std::array<int, 5> ints = {1, 3, 5, 7, 9};
    CONSTEXPR_EXPECT_NE(DoHash(ints), 0);
}

TEST(HashTest, HashingStructOfIntsNotEqZero)
{
    constexpr Ints ints = {.a = 1, .b = 3, .c = 5, .d = 7, .e = 9};
    CONSTEXPR_EXPECT_NE(DoHash(ints), 0);
}

TEST(HashTest, HashingStringEqNotEqZero)
{
    constexpr std::string_view str = "abc";
    CONSTEXPR_EXPECT_NE(DoHash(str), 0);
}

TEST(HashTest, HashingWstringNotEqZero)
{
    constexpr std::wstring_view str = L"abc";
    CONSTEXPR_EXPECT_NE(DoHash(str), 0);
}

TEST(HashTest, HashingIntEqHashAppendingBytes)
{
    constexpr int i = 0x04030201;
    constexpr auto expected = HashMulti(std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4});
    CONSTEXPR_EXPECT_EQ(DoHash(i), expected);
}

TEST(HashTest, HashingArrayOfIntsEqHashAppendingInts)
{
    constexpr std::array<int, 5> ints = {1, 3, 5, 7, 9};
    constexpr auto expected = HashMulti(1, 3, 5, 7, 9);
    CONSTEXPR_EXPECT_EQ(DoHash(ints), expected);
}

TEST(HashTest, HashingStructOfIntsEqHashAppendingInts)
{
    constexpr Ints ints = {.a = 1, .b = 3, .c = 5, .d = 7, .e = 9};
    constexpr auto expected = HashMulti(1, 3, 5, 7, 9);
    CONSTEXPR_EXPECT_EQ(DoHash(ints), expected);
}

TEST(HashTest, HashingStringEqHashAppendingChars)
{
    constexpr std::string_view str = "abc";
    constexpr auto expected = HashMulti('a', 'b', 'c');
    CONSTEXPR_EXPECT_EQ(DoHash(str), expected);
}

TEST(HashTest, HashingWstringEqHashAppendingWchars)
{
    constexpr std::wstring_view str = L"abc";
    constexpr auto expected = HashMulti(L'a', L'b', L'c');
    CONSTEXPR_EXPECT_EQ(DoHash(str), expected);
}
