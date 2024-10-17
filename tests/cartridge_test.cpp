/*
Copyright (C) 2016-2024 Team Vectrexia

This file is part of Vectrexia.

Vectrexia is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Vectrexia is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Vectrexia.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <cartridge.h>
#include <catch2/catch_all.hpp>  

TEST_CASE("CartridgeTest Load32K", "[cartridge]") {
    auto romdata = std::make_unique<std::array<uint8_t, 0x8000>>();
    romdata->fill(0xde);

    Cartridge cart;
    cart.Load((const uint8_t*)romdata->data(), 0x8000);

    REQUIRE(cart.is_loaded());
}

TEST_CASE("CartridgeTest Load4K", "[cartridge]") {
    auto romdata = std::make_unique<std::array<uint8_t, 0x1000>>();

    romdata->fill(0xad);

    Cartridge cart;
    cart.Load((const uint8_t*)romdata->data(), 0x1000);

    REQUIRE(cart.is_loaded());
}

TEST_CASE("CartridgeTest Load64K", "[cartridge]") {
    auto romdata = std::make_unique<std::array<uint8_t, 0x10000>>();
    romdata->fill(0xbe);

    Cartridge cart;
    cart.Load((const uint8_t*)romdata->data(), 0x10000);

    REQUIRE(cart.is_loaded());
}

TEST_CASE("CartridgeTest LoadTooLarge", "[cartridge]") {
    auto romdata = std::make_unique<std::array<uint8_t, 0x10001>>();
    romdata->fill(0xbe);

    Cartridge cart;
    cart.Load((const uint8_t*)romdata->data(), 0x10001);

    REQUIRE_FALSE(cart.is_loaded());
}

TEST_CASE("CartridgeTest Read", "[cartridge]") {
    auto romdata = std::make_unique<std::array<uint8_t, 0x1000>>();
    romdata->fill(0xef);

    Cartridge cart;
    cart.Load((const uint8_t*)romdata->data(), 0x1000);

    REQUIRE(cart.is_loaded());
    REQUIRE(cart.Read(0) == 0xef);
}

TEST_CASE("CartridgeTest Write", "[cartridge]") {
    auto romdata = std::make_unique<std::array<uint8_t, 0x1000>>();
    romdata->fill(0xde);

    Cartridge cart;
    cart.Load((const uint8_t*)romdata->data(), 0x1000);

    REQUIRE(cart.is_loaded());
    cart.Write(0, 0xbe);
    // rom should remain unchanged.
    REQUIRE(cart.Read(0) == 0xde);
}
