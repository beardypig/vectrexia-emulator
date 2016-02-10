#include <stdlib.h>
#include <unistd.h>
#include <memory>
#include <vectrexia.h>
#include "gif.h"

static const int FRAME_WIDTH = 330;
static const int FRAME_HEIGHT = 410;
std::unique_ptr<Vectrex> vectrex = std::make_unique<Vectrex>();
uint8_t framebuffer[FRAME_WIDTH * FRAME_HEIGHT * 4];

int main(int argc, char *argv[])
{
    int c, ch, index, r = 0;
    long skipframes = 0, outframes = 1000;
    FILE *romfile;
    char giffilename[2000];
    uint8_t rombuffer[65536];
    GifWriter gw = {};

    opterr = 0;
    while ((c = getopt(argc, argv, "s:n:")) != -1)
        switch (c)
        {
            case 's':
                skipframes = strtol(optarg, NULL, 10);
                break;
            case 'n':
                outframes = strtol(optarg, NULL, 10);
                break;
            default:
                abort();
        }

    auto nargs = (argc - optind);
    if (nargs < 1 || nargs > 2)
    {
        fprintf(stderr, "vectgif: usage: vectgif <rom> [gif]\n");
        return 1;
    }

    // load the ROM file
    printf("[ROM]: loading ROM \"%s\"\n", argv[optind]);
    romfile = fopen(argv[optind], "rb");

    while( ( ch = fgetc(romfile) ) != EOF && r < 65536)
    {
        rombuffer[r++] = (uint8_t) ch;
    }

    printf("[ROM]: size = %d\n", r);

    if (!vectrex->LoadCartridge(rombuffer, (size_t) r))
    {
        printf("Failed to load the ROM file\n");
        fclose(romfile);
        return 1;
    }

    fclose(romfile);

    if (nargs == 1)
    {
        sprintf(giffilename, "%s.gif", argv[optind]);
    }
    else
    {
        sprintf(giffilename, "%s", argv[optind + 1]);
    }


    GifBegin(&gw, giffilename, FRAME_WIDTH, FRAME_HEIGHT, 2, 8, false);


    vectrex->Reset();

    vectrex->SetPlayerOne(0x80, 0x80, 1, 1, 1, 1);
    vectrex->SetPlayerTwo(0x80, 0x80, 1, 1, 1, 1);

    for (int frame = 0; frame < skipframes+outframes; frame++)
    {
        vectrex->Run(30000);
        auto fb = vectrex->getFramebuffer();

        // get the palette frame buffer
        for (int i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++)
        {
            auto color = fb[i].rgba8();
            framebuffer[i*4] = (uint8_t) ((color >> 16) & 0xff);
            framebuffer[i*4+1] = (uint8_t) ((color >> 8) & 0xff);
            framebuffer[i*4+1] = (uint8_t) (color & 0xff);
            framebuffer[i*4+2] = 0xff;
        }

        if (frame > skipframes)
            GifWriteFrame(&gw, framebuffer, FRAME_WIDTH, FRAME_HEIGHT, 2);

        if (frame % 100 == 0)
        {
            printf("[VECTREX] frame = %'d\n", frame);
        }
    }

    GifEnd(&gw);

    return 0;
}
