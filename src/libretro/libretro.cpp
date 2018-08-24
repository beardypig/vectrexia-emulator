/*
Copyright (C) 2016 beardypig

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
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <memory>

#if _MSC_VER >= 1910 && !__INTEL_COMPILER
#include "win32.h"
#endif

#include "libretro.h"
#include "vectrexia.h"


std::unique_ptr<Vectrex> vectrex = std::make_unique<Vectrex>();
vxgfx::framebuffer<FRAME_WIDTH, FRAME_HEIGHT, vxgfx::pf_rgb565_t> out_buffer{};

FILE* sound_out;

// Callbacks
static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

unsigned retro_api_version(void) { return RETRO_API_VERSION; }

// Cheats
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {}

// Load a cartridge
bool retro_load_game(const struct retro_game_info *info)
{
    // Set the controller descriptor
    struct retro_input_descriptor desc[] = {
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "1" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "2" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "3" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "4" },
            { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,  "Analog X" },
            { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,  "Analog Y" },

            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "1" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "2" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "3" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "4" },
            { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,  "Analog X" },
            { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,  "Analog Y" },
            { 1, RETRO_DEVICE_NONE, 0, 0,  nullptr },
    };

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    // Reset the Vectrex, clears the cart ROM and loads the System ROM
    vectrex->Reset();

    if (info && info->data) { // ensure there is ROM data
        return vectrex->LoadCartridge((const uint8_t*)info->data, info->size);
    }

    return true;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }

// Unload the cartridge
void retro_unload_game(void) { vectrex->UnloadCartridge(); }

unsigned retro_get_region(void) { return RETRO_REGION_PAL; }

// libretro unused api functions
void retro_set_controller_port_device(unsigned port, unsigned device) {}


void *retro_get_memory_data(unsigned id) { return nullptr; }
size_t retro_get_memory_size(unsigned id){ return 0; }

// Serialisation methods
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void *data, size_t size) { return false; }
bool retro_unserialize(const void *data, size_t size) { return false; }

// End of retrolib
void retro_deinit(void) {
    fclose(sound_out);
}

// libretro global setters
void retro_set_environment(retro_environment_t cb)
{
    environ_cb = cb;
    bool no_rom = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {}
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_init(void)
{
    /* set up some logging */
    struct retro_log_callback log;
    unsigned level = 4;

    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
        log_cb = log.log;
    else
        log_cb = nullptr;

    // the performance level is guide to frontend to give an idea of how intensive this core is to run
    environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);

    vectrex->Reset();
    sound_out = fopen("test.wav", "wb");
}


/*
 * Tell libretro about this core, it's name, version and which rom files it supports.
 */
void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(retro_system_info));
    info->library_name = vectrex->GetName();
    info->library_version = vectrex->GetVersion();
    info->need_fullpath = false;
    info->valid_extensions = "bin|vec";
}

/*
 * Tell libretro about the AV system; the fps, sound sample rate and the
 * resolution of the display.
 */
void retro_get_system_av_info(struct retro_system_av_info *info) {

    int pixel_format = RETRO_PIXEL_FORMAT_RGB565;

    memset(info, 0, sizeof(retro_system_av_info));
    info->timing.fps            = 50.0;
    info->timing.sample_rate    = 44100.0;
    info->geometry.base_width   = FRAME_WIDTH;
    info->geometry.base_height  = FRAME_HEIGHT;
    info->geometry.max_width    = FRAME_WIDTH;
    info->geometry.max_height   = FRAME_HEIGHT;
    //info->geometry.aspect_ratio = 330.0f / 410.0f;

    // the performance level is guide to frontend to give an idea of how intensive this core is to run
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);
}

// Reset the Vectrex
void retro_reset(void)
{
    vectrex->Reset();
}

// Test the user input and return the state of the joysticks and buttons
void get_joystick_state(unsigned port, uint8_t &x, uint8_t &y, uint8_t &b1, uint8_t &b2, uint8_t &b3, uint8_t &b4)
{
    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
        x = 0x00;
    else if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
        x = 0xff;
    else
        x = (uint8_t) (
                (input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X) / 256) + 128);

    if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
        y = 0xff;
    else if (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN ))
        y = 0x00;
    else
    {
        // retroarch y axis is inverted wrt to the vectrex
        auto y_value = input_state_cb(port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
        y = (uint8_t) (~((y_value/256) + 128) + 1);
    }

    b1 = (unsigned char) (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A ) ? 1 : 0);
    b2 = (unsigned char) (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B ) ? 1 : 0);
    b3 = (unsigned char) (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X ) ? 1 : 0);
    b4 = (unsigned char) (input_state_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y ) ? 1 : 0);
}

static const auto green = vxgfx::pf_argb_t(255, 255, 0, 128 );

// Run a single frames with out Vectrex emulation.
void retro_run(void)
{

    // User input
    input_poll_cb();

    uint8_t p1_x, p1_y, p2_x, p2_y;
    uint8_t p1_b1, p1_b2, p1_b3, p1_b4, p2_b1, p2_b2, p2_b3, p2_b4;

    // updates the p1_* variables
    get_joystick_state(0, p1_x, p1_y, p1_b1, p1_b2, p1_b3, p1_b4);
    get_joystick_state(1, p2_x, p2_y, p2_b1, p2_b2, p2_b3, p2_b4);

    vectrex->SetPlayerOne(p1_x, p1_y, p1_b1, p1_b2, p1_b3, p1_b4);
    vectrex->SetPlayerTwo(p2_x, p2_y, p2_b1, p2_b2, p2_b3, p2_b4);

    vectrex->psg_->channel_a_on = !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_1);
    vectrex->psg_->channel_b_on = !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_2);
    vectrex->psg_->channel_c_on = !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_3);

    // Vectrex CPU is 1.5MHz (1500000) and at 50 fps, a frame lasts 20ms, therefore in every frame 30,000 cycles happen.
    auto cycles_run = vectrex->Run(30000);

    // Get buffers
    auto fb = vectrex->getFramebuffer();
    auto db = vectrex->getDebugbuffer();

    // Print sound debugging text
    vxgfx::draw_text<vxgfx::m_direct>(*db, 2, 10, green, vxl::format("@ %.fHz", (double)(cycles_run * 50)));
    vxgfx::draw_text<vxgfx::m_direct>(*db, 2, 20, green, vxl::format("Channel A: %3.0fHz (noise: %d)", vectrex->psg_->channel_a.frequency_, vectrex->psg_->channel_a.noise_enabled));
    vxgfx::draw_text<vxgfx::m_direct>(*db, 2, 30, green, vxl::format("Channel B: %3.0fHz (noise: %d)", vectrex->psg_->channel_b.frequency_, vectrex->psg_->channel_b.noise_enabled));
    vxgfx::draw_text<vxgfx::m_direct>(*db, 2, 40, green, vxl::format("Channel C: %3.0fHz (noise: %d)", vectrex->psg_->channel_c.frequency_, vectrex->psg_->channel_c.noise_enabled));


    // Define the pf_mono_t => pf_rgb565_t transform
    auto mono_to_rgb565 = [](const vxgfx::pf_mono_t &p) {
        return vxgfx::pf_rgb565_t(static_cast<uint8_t>(0xff * p.value),
                                  static_cast<uint8_t>(0xff * p.value),
                                  static_cast<uint8_t>(0xff * p.value));
    };

    // fb => out_buffer transform
    std::transform(fb->begin(), fb->end(), out_buffer.begin(), mono_to_rgb565);

    // TODO
    // some blending of db on top of out_buffer

    // 882 audio samples per frame (44.1kHz @ 50 fps)
    uint8_t buffer[882];
    vectrex->psg_->FillBuffer(buffer, sizeof(buffer));
    fwrite(buffer, sizeof(uint8_t), sizeof(buffer), sound_out);

    for (unsigned char i : buffer) {
        auto convs = static_cast<short>((i << 8u) - 0x7ffu);
        // mono sound, same data for both channels
        audio_cb(convs, convs);
    }
    
    video_cb(reinterpret_cast<const uint16_t*>(out_buffer.data()),
        FRAME_WIDTH, FRAME_HEIGHT, sizeof(unsigned short) * FRAME_WIDTH);
}
