#pragma once
#include <atomic>
#include "../../configuration.h"

namespace debugger::ui::widget {
	class Widget {
	public:
		Widget(Configuration& config) : config(config) {}
		virtual ~Widget() = default;
		virtual void render(long delta) = 0;
		virtual void process(long delta) {}
		virtual void show() {
			showWindow = true;
		}
		virtual void hide() {
			showWindow = false;
		}

	protected:
		Configuration& config;
		std::atomic<bool> showWindow = false;
	};
}