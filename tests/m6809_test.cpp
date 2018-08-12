#include <m6809.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

auto getRegisters() {
    static M6809 cpu;
    return cpu.getRegisters();
}

TEST(ComputeFlags, ZeroFlagNotSetOn1)
{
    auto regs = getRegisters();
    regs.UpdateFlagZero<uint8_t>(1);
    EXPECT_EQ(0, regs.CC);
}

TEST(ComputeFlags, ZeroFlagSetOn0)
{
    auto regs = getRegisters();
    regs.UpdateFlagZero<uint8_t>(0);
    EXPECT_EQ(FLAG_Z, regs.CC);
}

TEST(ComputeFlags, NegativeFlagNotSetOn1)
{
    auto regs = getRegisters();
    regs.UpdateFlagNegative<uint8_t>(0);
    EXPECT_EQ(0, regs.CC);
}

TEST(ComputeFlags, NegativeFlagSetOnFF)
{
    auto regs = getRegisters();
    regs.UpdateFlagNegative<uint8_t>(0xff);
    EXPECT_EQ(FLAG_N, regs.CC);
}


TEST(Misc, SignExtend)
{
    uint8_t data = 0x10;
    uint16_t sdata = (uint16_t) ((~(data & 0x80) + 1) | (data & 0xff));

    EXPECT_EQ(sdata, (int16_t)data);

}
