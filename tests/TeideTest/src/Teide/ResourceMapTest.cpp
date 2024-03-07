
#include "Teide/ResourceMap.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace Teide;

namespace
{
struct TestProperties
{
    int value = 0;

    void Visit(auto f) const { return f(value); }
    bool operator==(const TestProperties&) const = default;
};

struct TestResource
{
    TestProperties properties;
    int hiddenValue = 0;
};

struct TestHandle : public Handle<TestProperties>
{
    using Handle::Handle;
    using Handle::operator->;
};

using Map = ResourceMap<TestHandle, TestResource>;

template <class T>
T& Copy(T& obj)
{
    return obj;
}

template <class T>
T&& Move(T&& obj) // NOLINT(cppcoreguidelines-missing-std-forward)
{
    return static_cast<T&&>(obj);
}

TEST(ResourceMapTest, AddResource)
{
    auto map = Map("test");
    const TestHandle handle = map.Insert(TestResource{{42}, 102});
    EXPECT_THAT(handle->value, Eq(42));
}

TEST(ResourceMapTest, GetResource)
{
    auto map = Map("test");
    const TestHandle handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(handle);
    EXPECT_THAT(resource.properties.value, Eq(42));
    EXPECT_THAT(resource.hiddenValue, Eq(102));
}

TEST(ResourceMapTest, DestroyResourceImmediately)
{
    auto map = Map("test", 0);
    std::optional<TestHandle> handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(*handle);
    handle.reset();
    EXPECT_THAT(resource.hiddenValue, Eq(0));
}

TEST(ResourceMapTest, CopyHandleAndDestroyResourceImmediately)
{
    auto map = Map("test", 0);
    std::optional<TestHandle> handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(*handle);
    std::optional<TestHandle> handle2 = handle;
    handle.reset();
    EXPECT_THAT(resource.hiddenValue, Eq(102));
    handle2.reset();
    EXPECT_THAT(resource.hiddenValue, Eq(0));
}

TEST(ResourceMapTest, DestroyResourceAfterOneFrame)
{
    auto map = Map("test", 1);
    std::optional<TestHandle> handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(*handle);
    handle.reset();
    map.NextFrame();
    EXPECT_THAT(resource.hiddenValue, Eq(0));
}

TEST(ResourceMapTest, CopyHandleAndDestroyResourceAfterOneFrame)
{
    auto map = Map("test", 1);
    std::optional<TestHandle> handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(*handle);
    std::optional<TestHandle> handle2 = handle;
    handle.reset();
    map.NextFrame();
    EXPECT_THAT(resource.hiddenValue, Eq(102));
    handle2.reset();
    map.NextFrame();
    EXPECT_THAT(resource.hiddenValue, Eq(0));
}

TEST(ResourceMapTest, DestroyResourceAfterTwoFrames)
{
    auto map = Map("test", 2);
    std::optional<TestHandle> handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(*handle);
    handle.reset();
    map.NextFrame();
    EXPECT_THAT(resource.hiddenValue, Eq(102)); // not destroyed yet
    map.NextFrame();
    EXPECT_THAT(resource.hiddenValue, Eq(0));
}

TEST(ResourceMapTest, CopyHandleAndDestroyResourceAfterTwoFrames)
{
    auto map = Map("test", 2);
    std::optional<TestHandle> handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(*handle);
    std::optional<TestHandle> handle2 = handle;
    handle.reset();
    map.NextFrame();
    handle2.reset();
    map.NextFrame();
    EXPECT_THAT(resource.hiddenValue, Eq(102));
    map.NextFrame();
    EXPECT_THAT(resource.hiddenValue, Eq(0));
}

TEST(ResourceMapTest, DestroyResourceByAssigning)
{
    auto map = Map("test", 0);
    TestHandle handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(handle);
    handle = map.Insert({});
    static_cast<void>(handle);
    EXPECT_THAT(resource.properties.value, Eq(0));
    EXPECT_THAT(resource.hiddenValue, Eq(0));
}

TEST(ResourceMapTest, CopyHandleAndDestroyResourceByAssigning)
{
    auto map = Map("test", 0);
    TestHandle handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(handle);
    TestHandle handle2 = handle;
    handle = map.Insert({});
    static_cast<void>(handle);
    EXPECT_THAT(resource.properties.value, Eq(42));
    EXPECT_THAT(resource.hiddenValue, Eq(102));
    handle2 = map.Insert({});
    static_cast<void>(handle2);
    EXPECT_THAT(resource.properties.value, Eq(0));
    EXPECT_THAT(resource.hiddenValue, Eq(0));
}

TEST(ResourceMapTest, SelfCopyAssignment)
{
    auto map = Map("test");
    TestHandle handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(handle);
    handle = Copy(handle);
    EXPECT_THAT(resource.properties.value, Eq(42));
    EXPECT_THAT(resource.hiddenValue, Eq(102));
}

TEST(ResourceMapTest, SelfMoveAssignment)
{
    auto map = Map("test");
    TestHandle handle = map.Insert(TestResource{{42}, 102});
    const TestResource& resource = map.Get(handle);
    handle = Move(handle);
    EXPECT_THAT(resource.properties.value, Eq(42));
    EXPECT_THAT(resource.hiddenValue, Eq(102));
}

TEST(ResourceMapTest, CompareHandles)
{
    auto map = Map("test");
    const Handle handle1 = map.Insert({});
    const Handle handle2 = Copy(handle1);
    EXPECT_THAT(handle1, Eq(handle2));
    const Handle handle3 = map.Insert({});
    EXPECT_THAT(handle1, Ne(handle3));
}

} // namespace
