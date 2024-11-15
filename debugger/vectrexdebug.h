#pragma once

#include "vectrexia.h"

class DebugVectrex : public Vectrex {
public:
	const std::array<uint8_t, 1024>& GetRAM() const { return ram_; }
	const std::array<uint8_t, 0x10000>& getMemory() {
		std::fill(snapshot.begin(), snapshot.end(), 0);
		std::copy(ram_.begin(), ram_.end(), snapshot.begin() + 0xc800);
		std::copy(ram_.begin(), ram_.end(), snapshot.begin() + 0xcc00);
		std::copy(sysrom_.begin(), sysrom_.end(), snapshot.begin() + 0xe000);

		if (cartridge_) {
			auto cartdata = cartridge_->dump();
			// pb1 controls which bank of the cart rom to access
			auto pb1 = (uint8_t)(via_->getPortBState() >> 6 & 1);
			std::copy(cartdata.begin() + pb1 * 0x8000, cartdata.begin() + pb1 * 0x8000 + 0x8000, snapshot.begin());
		}

		// Use peek to avoid the side effects of reading from the VIA
        for (auto addr = 0xD000; addr <= 0xD7FF; ++addr) {
			snapshot[addr] = via_->Peek((uint8_t)(addr & 0xf));
        }
		return snapshot;
	}
	const VectrexController getPlayer1() const { return p1_joystick; }
	const VectrexController getPlayer2() const { return p2_joystick; }
protected:
	std::array<uint8_t, 0x10000> snapshot;
};