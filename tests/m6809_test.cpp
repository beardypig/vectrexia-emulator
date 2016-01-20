#include <m6809.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(ComputeFlags, ZeroFlagNotSetOn1)
{
    EXPECT_EQ(0, M6809::ComputeZeroFlag((uint8_t)1));
}

TEST(ComputeFlags, ZeroFlagSetOn0)
{
    EXPECT_EQ(1, M6809::ComputeZeroFlag((uint8_t)0));
}

TEST(ComputeFlags, NegativeFlagNotSetOn1)
{
    EXPECT_EQ(0, M6809::ComputeZeroFlag((uint8_t)1));
}

TEST(ComputeFlags, NegativeFlagSetOnFF)
{
    EXPECT_EQ(1, M6809::ComputeNegativeFlag<8>(0xFF));
}


TEST(Misc, SignExtend)
{
    uint8_t data = 0x10;
    uint16_t sdata = (uint16_t) ((~(data & 0x80) + 1) | (data & 0xff));

    EXPECT_EQ(sdata, (int16_t)data);

}
