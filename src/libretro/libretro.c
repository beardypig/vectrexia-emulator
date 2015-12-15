#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libretro.h"
#include "vectrexia.h"
#include "sysrom.h"

/* callback globals */
static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

/* video frame buffer */
static unsigned short framebuffer[VECTREX_RES_WIDTH * VECTREX_RES_HEIGHT];

/* libretro unused api functions  */
void retro_set_controller_port_device(unsigned port, unsigned device) {}
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {}
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {}
unsigned retro_get_region(void) { return RETRO_REGION_PAL; }
unsigned retro_api_version(void) { return RETRO_API_VERSION; }
bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info) { return false; }
void *retro_get_memory_data(unsigned id) { return NULL; }
size_t retro_get_memory_size(unsigned id){ return 0; }
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void *data, size_t size) { return false; }
bool retro_unserialize(const void *data, size_t size) { return false; }
void retro_deinit(void) {}

/* libretro global setters */
void retro_set_environment(retro_environment_t cb)
{
    environ_cb = cb;
    bool no_rom = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
}

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
        log_cb = NULL;

    // the performance level is guide to frontend to give an idea of how intensive this core is to run
    environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
    vectrex_reset();
}


/*
 * Tell libretro about this core, it's name, version and which rom files it supports.
 */
void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name = VECTREXIA;
    info->library_version = VECTREXIA_VERSION;
    info->need_fullpath = false;
    info->valid_extensions = "bin|vec";
}

/*
 * Tell libretro about the AV system; the fps, sound sample rate and the
 * resolution of the display.
 */
void retro_get_system_av_info(struct retro_system_av_info *info) {

    int pixel_format = RETRO_PIXEL_FORMAT_RGB565;

    memset(info, 0, sizeof(*info));
    info->timing.fps            = VECTREX_FPS;
    info->timing.sample_rate    = VECTREX_SAMPLE_RATE;
    info->geometry.base_width   = VECTREX_RES_WIDTH;
    info->geometry.base_height  = VECTREX_RES_HEIGHT;
    info->geometry.max_width    = VECTREX_RES_WIDTH;
    info->geometry.max_height   = VECTREX_RES_HEIGHT;
    info->geometry.aspect_ratio = VECTREX_RES_WIDTH / VECTREX_RES_HEIGHT;

    // the performance level is guide to frontend to give an idea of how intensive this core is to run
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);
}

/*
 * Copy the game data in to the Vectrexia's cart memory
 */
bool retro_load_game(const struct retro_game_info *info)
{
    // Reset the Vectrex, clears the cart ROM and loads the System ROM
    retro_reset();

    // ensure the ROM is loaded and that it is not too large to load
    if (info && info->data) {
        if (info->size > 0 && info->size <= VECTREX_CART_SIZE) {
            // copy the game data in to the cart memory
            memcpy(cart, info->data, info->size);
        } else { // the ROM is the wrong size, return failure
            return false;
        }
    }

    return true;
}

/*
 * Clear the cart memory and reset the Vectrex
 */
void retro_unload_game(void) {
    retro_reset();
}

/*
 * Reset the retrolib core
 */
void retro_reset(void)
{
    // clear the cart memory
    memset(cart, 0, VECTREX_CART_SIZE);

    // copy the bios in to the sysrom
    memcpy(sysrom, system_bios, system_bios_size);
    vectrex_reset();
}

/*
 * Run a single frames with out Vectrex emulation.
 * Vectrex CPU is 1.5MHz (1500000) and at 50 fps, a frame lasts 20ms,
 * therefore every frame 30,000 cycles happen.
 */
void retro_run(void)
{
    int i = 0;
    vectrex_run(VECTREX_CLOCK/VECTREX_FPS);

    for (i = 0; i < VECTREX_SAMPLE_RATE/VECTREX_FPS; i++) {
        audio_cb(1, 1);
    }

    video_cb(framebuffer, VECTREX_RES_WIDTH, VECTREX_RES_HEIGHT, sizeof(unsigned short) * VECTREX_RES_WIDTH);
}
