#include <cartridge.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(CartridgeTest, TestLoad32K)
{
    std::array<uint8_t, 0x8000> romdata;
    romdata.fill(0xde);

    Cartridge cart;
    cart.Load((const uint8_t *)romdata.data(), 0x8000);

    EXPECT_TRUE(cart.is_loaded());
}

TEST(CartridgeTest, TestLoad4K)
{
    std::array<uint8_t, 0x1000> romdata;
    romdata.fill(0xad);

    Cartridge cart;
    cart.Load((const uint8_t *)romdata.data(), 0x1000);

    EXPECT_TRUE(cart.is_loaded());
}

TEST(CartridgeTest, TestLoad64K)
{
    std::array<uint8_t, 0x10000> romdata;
    romdata.fill(0xbe);

    Cartridge cart;
    cart.Load((const uint8_t *)romdata.data(), 0x10000);

    EXPECT_TRUE(cart.is_loaded());
}

TEST(CartridgeTest, TestLoadTooLarge)
{
    std::array<uint8_t, 0x10001> romdata;
    romdata.fill(0xbe);

    Cartridge cart;
    cart.Load((const uint8_t *)romdata.data(), 0x10001);

    EXPECT_FALSE(cart.is_loaded());
}

TEST(CartridgeTest, TestRead)
{
    std::array<uint8_t, 0x1000> romdata;
    romdata.fill(0xef);

    Cartridge cart;
    cart.Load((const uint8_t *)romdata.data(), 0x1000);

    EXPECT_TRUE(cart.is_loaded());
    EXPECT_EQ(0xef, cart.Read(0));
}


TEST(CartridgeTest, TestWrite)
{
    std::array<uint8_t, 0x1000> romdata;
    romdata.fill(0xde);

    Cartridge cart;
    cart.Load((const uint8_t *)romdata.data(), 0x1000);

    EXPECT_TRUE(cart.is_loaded());
    cart.Write(0, 0xbe);
    // rom should remain unchanged.
    EXPECT_EQ(0xde, cart.Read(0));
}
