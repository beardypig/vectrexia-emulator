#include <m6809.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(ComputeFlags, ZeroFlagNotSetOn1)
{
    uint8_t flags = 0;
    M6809::ComputeZeroFlag<uint8_t>(flags, 1);
    EXPECT_EQ(0, flags);
}

TEST(ComputeFlags, ZeroFlagSetOn0)
{
    uint8_t flags = 0;
    M6809::ComputeZeroFlag<uint8_t>(flags, 0);
    EXPECT_EQ(FLAG_Z, flags);
}

TEST(ComputeFlags, NegativeFlagNotSetOn1)
{
    uint8_t flags = 0;
    M6809::ComputeNegativeFlag<uint8_t>(flags, 0);
    EXPECT_EQ(0, flags);
}

TEST(ComputeFlags, NegativeFlagSetOnFF)
{
    uint8_t flags = 0;
    M6809::ComputeNegativeFlag<uint8_t>(flags, 0xff);
    EXPECT_EQ(FLAG_N, flags);
}


TEST(Misc, SignExtend)
{
    uint8_t data = 0x10;
    uint16_t sdata = (uint16_t) ((~(data & 0x80) + 1) | (data & 0xff));

    EXPECT_EQ(sdata, (int16_t)data);

}
