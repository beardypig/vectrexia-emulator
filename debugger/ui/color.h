#pragma once

#include <imgui.h>

namespace debugger::ui::color {
    const auto red = ImColor(1.0f, 0.0f, 0.0f, 1.0f);
    const auto green = ImColor(0.0f, 1.0f, 0.0f, 1.0f);
    const auto blue = ImColor(0.0f, 0.0f, 1.0f, 1.0f);
    const auto yellow = ImColor(1.0f, 1.0f, 0.0f, 1.0f);
    const auto cyan = ImColor(0.0f, 1.0f, 1.0f, 1.0f);
    const auto magenta = ImColor(1.0f, 0.0f, 1.0f, 1.0f);
    const auto orange = ImColor(1.0f, 0.5f, 0.0f, 1.0f);
    const auto purple = ImColor(0.5f, 0.0f, 0.5f, 1.0f);
    const auto brown = ImColor(0.6f, 0.4f, 0.2f, 1.0f);
    const auto black = ImColor(0.0f, 0.0f, 0.0f, 1.0f);
    const auto white = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
    const auto gray = ImColor(0.5f, 0.5f, 0.5f, 1.0f);
    const auto lightGray = ImColor(0.75f, 0.75f, 0.75f, 1.0f);
    const auto darkGray = ImColor(0.25f, 0.25f, 0.25f, 1.0f);
    const auto lightBlue = ImColor(0.68f, 0.85f, 0.90f, 1.0f);
    const auto lightGreen = ImColor(0.56f, 0.93f, 0.56f, 1.0f);
    const auto pink = ImColor(1.0f, 0.75f, 0.8f, 1.0f);
    const auto olive = ImColor(0.5f, 0.5f, 0.0f, 1.0f);
    const auto teal = ImColor(0.0f, 0.5f, 0.5f, 1.0f);
    const auto navy = ImColor(0.0f, 0.0f, 0.5f, 1.0f);
    const auto gold = ImColor(1.0f, 0.84f, 0.0f, 1.0f);
    const auto silver = ImColor(0.75f, 0.75f, 0.75f, 1.0f);
    const auto maroon = ImColor(0.5f, 0.0f, 0.0f, 1.0f);
    const auto lime = ImColor(0.75f, 1.0f, 0.0f, 1.0f);
    const auto skyBlue = ImColor(0.53f, 0.81f, 0.92f, 1.0f);
    const auto violet = ImColor(0.93f, 0.51f, 0.93f, 1.0f);

	static ImColor applyTransparency(const ImColor& color, float transparency) {
		auto colorc = color;
		colorc.Value.w = transparency;
        return colorc;
    }
}