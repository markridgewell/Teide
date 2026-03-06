
#include "Teide/Util/ListenSender.h"

#include "Teide/TestUtils.h"
#include "Teide/Util/TypeHelpers.h"
#include "gmock/gmock.h"

#include <gtest/gtest.h>

using namespace Teide;
using namespace testing;

namespace
{

template <class T>
struct MockReceiver
{
    using receiver_concept = ex::receiver_t;

    void set_value(T&& value) { impl->set_value(std::move(value)); }
    void set_stopped() { impl->set_stopped(); }

    struct RcvrImpl
    {
        MOCK_METHOD(void, set_value, (T&&));
        MOCK_METHOD(void, set_stopped, ());
    };
    using R = StrictMock<RcvrImpl>;

    std::shared_ptr<R> Impl() { return impl; }
    RcvrImpl& operator*() { return *impl; }

private:
    std::shared_ptr<R> impl = std::make_shared<R>();
};

TEST(ListenSenderTest, ConstrictedReceiverHasNoCompletionState)
{
    auto rcvr = ListenReceiver<int>();
    EXPECT_THAT(rcvr.expired(), IsTrue());
}

TEST(ListenSenderTest, ListeningCreatesCompletionState)
{
    auto rcvr = ListenReceiver<int>();
    auto sndr = Listen(rcvr);
    EXPECT_THAT(rcvr.expired(), IsFalse());
}

TEST(ListenSenderTest, DestroyingSenderDestroysCompletionState)
{
    auto rcvr = ListenReceiver<int>();
    {
        auto sndr = Listen(rcvr);
        EXPECT_THAT(rcvr.expired(), IsFalse());
    }
    EXPECT_THAT(rcvr.expired(), IsTrue());
}

TEST(ListenSenderTest, ListeningTwiceCreatesCompletionStateOnce)
{
    auto rcvr = ListenReceiver<int>();
    auto sndr1 = Listen(rcvr);
    {
        auto sndr2 = Listen(rcvr);
        EXPECT_THAT(rcvr.expired(), IsFalse());
    }
    EXPECT_THAT(rcvr.expired(), IsFalse());
}

TEST(ListenSenderTest, ConnectingThenCompletingProducesValue)
{
    auto rcvr = ListenReceiver<int>();
    auto sndr = Listen(rcvr);
    auto mockRcvr = MockReceiver<int>();
    auto mockImpl = mockRcvr.Impl();
    EXPECT_CALL(*mockImpl, set_value(42)).Times(1);

    auto op = ex::connect(sndr, mockRcvr);
    op.start();
    std::move(rcvr).set_value(42);
}

TEST(ListenSenderTest, ConnectingThenCompletingProducesValue2)
{
    auto rcvr = ListenReceiver<int>();
    auto sndr = Listen(rcvr);
    auto mockRcvr = MockReceiver<int>();
    EXPECT_CALL(*mockRcvr, set_value(42)).Times(1);

    auto op = ex::connect(sndr, std::move(mockRcvr));
    op.start();
    std::move(rcvr).set_value(42);
}

TEST(ListenSenderTest, CompletingThenConnectingProducesValue)
{
    auto rcvr = ListenReceiver<int>();
    auto sndr = Listen(rcvr);
    auto mockRcvr = MockReceiver<int>();
    auto mockImpl = mockRcvr.Impl();
    EXPECT_CALL(*mockImpl, set_value(42)).Times(1);

    std::move(rcvr).set_value(42);
    auto op = ex::connect(sndr, mockRcvr);
    op.start();
}

TEST(ListenSenderTest, ConnectingThenStoppingProducesStop)
{
    auto rcvr = ListenReceiver<int>();
    auto sndr = Listen(rcvr);
    auto mockRcvr = MockReceiver<int>();
    auto mockImpl = mockRcvr.Impl();
    EXPECT_CALL(*mockImpl, set_stopped()).Times(1);

    auto op = ex::connect(sndr, mockRcvr);
    op.start();
    std::move(rcvr).set_stopped();
}

TEST(ListenSenderTest, StoppingThenConnectingProducesStop)
{
    auto rcvr = ListenReceiver<int>();
    auto sndr = Listen(rcvr);
    auto mockRcvr = MockReceiver<int>();
    auto mockImpl = mockRcvr.Impl();
    EXPECT_CALL(*mockImpl, set_stopped()).Times(1);

    std::move(rcvr).set_stopped();
    auto op = ex::connect(sndr, mockRcvr);
    op.start();
}

TEST(ListenSenderTest, StoppingTwiceViolatesContract)
{
    auto rcvr = ListenReceiver<int>();
    auto sndr = Listen(rcvr);
    auto mockRcvr = MockReceiver<int>();
    auto mockImpl = mockRcvr.Impl();
    EXPECT_CALL(*mockImpl, set_stopped()).Times(1);

    std::move(rcvr).set_stopped();
    EXPECT_ASSERTION(std::move(rcvr).set_stopped());
    auto op = ex::connect(sndr, mockRcvr);
    op.start();
}

TEST(ListenSenderTest, SettingValueTwiceViolatesContract)
{
    auto rcvr = ListenReceiver<int>();
    auto sndr = Listen(rcvr);
    auto mockRcvr = MockReceiver<int>();
    auto mockImpl = mockRcvr.Impl();
    EXPECT_CALL(*mockImpl, set_value(42)).Times(1);

    std::move(rcvr).set_value(42);
    EXPECT_ASSERTION(std::move(rcvr).set_value(42));
    auto op = ex::connect(sndr, mockRcvr);
    op.start();
}

TEST(ListenSenderTest, SettingValueAfterCancelingViolatesContract)
{
    auto rcvr = ListenReceiver<int>();
    auto sndr = Listen(rcvr);
    auto mockRcvr = MockReceiver<int>();
    auto mockImpl = mockRcvr.Impl();
    EXPECT_CALL(*mockImpl, set_value(42)).Times(1);

    std::move(rcvr).set_value(42);
    EXPECT_ASSERTION(std::move(rcvr).set_value(42));
    auto op = ex::connect(sndr, mockRcvr);
    op.start();
}

} // namespace
