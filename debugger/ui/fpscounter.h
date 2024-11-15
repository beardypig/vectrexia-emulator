#pragma once    
#include <array>
#include <numeric>  
#include <chrono>  

class FPSCounter {
public:
    static constexpr int FRAME_COUNT = 60;  

    FPSCounter()
        : index(0), frameTimes{}, lastTick(std::chrono::high_resolution_clock::now()) {
    }

    void tick() {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTime = now - lastTick;
        lastTick = now;
        update(deltaTime.count());
    }

    float getAverageFPS() const {
        float frametime = getAverageFrametime();
        return (frametime > 0.0f) ? 1.0f / frametime : 0.0f;
    }

    float getAverageFrametime() const {
        return std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / FRAME_COUNT;
    }

private:
    // Internal method to update the FPS buffer
    void update(float frametime) {
        frameTimes[index] = frametime;
        index = (index + 1) % FRAME_COUNT;
    }

    std::array<float, FRAME_COUNT> frameTimes;
    int index; 
    std::chrono::high_resolution_clock::time_point lastTick;
};