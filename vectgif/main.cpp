#include <cstdlib>
#include <memory>
#include <vectrexia.h>
#include "gif.h"
#include "getopt.h"

std::unique_ptr<Vectrex> vectrex = std::make_unique<Vectrex>();
std::array<uint8_t , FRAME_WIDTH * FRAME_HEIGHT * 4> gif_buffer{};

int main(int argc, char *argv[])
{
  int c = 0;
  int ch = 0;
  int r = 0;
  long skipframes = 0;
  long outframes = 1000;
  FILE *romfile;
  char giffilename[2000]{};
  uint8_t rombuffer[65536]{};
  GifWriter gw = {};

  opterr = 0;
  while ((c = getopt(argc, argv, "s:n:")) != -1) {
    switch (c) {
    case 's':skipframes = strtol(optarg, nullptr, 10);
      break;
    case 'n':outframes = strtol(optarg, nullptr, 10);
      break;
    default:abort();
    }
  }

  auto nargs = (argc - optind);
  if (nargs < 1 || nargs > 2) {
    fprintf(stderr, "vectgif: usage: vectgif <rom> [gif]\n");
    return 1;
  }

  // load the ROM file
  printf("[ROM]: loading ROM \"%s\"\n", argv[optind]);
  romfile = fopen(argv[optind], "rb");

  while( ( ch = fgetc(romfile) ) != EOF && r < 65536) {
    rombuffer[r++] = static_cast<uint8_t>(ch);
  }
  fclose(romfile);

  printf("[ROM]: size = %d\n", r);

  if (!vectrex->LoadCartridge(rombuffer, static_cast<size_t>(r))) {
    printf("Failed to load the ROM file\n");
    return 1;
  }

  if (nargs == 1) {
    sprintf(giffilename, "%s.gif", argv[optind]);
  }
  else {
    sprintf(giffilename, "%s", argv[optind + 1]);
  }

  GifBegin(&gw, giffilename, FRAME_WIDTH, FRAME_HEIGHT, 2, 8, false);

  vectrex->Reset();

  vectrex->SetPlayerOne(0x80, 0x80, 1, 1, 1, 1);
  vectrex->SetPlayerTwo(0x80, 0x80, 1, 1, 1, 1);

  for (int frame = 0; frame < outframes; frame++) {
    for (int s = 0; s < skipframes+1; s++) {
      vectrex->Run(30000);
    }

    auto framebuffer = vectrex->getFramebuffer();

    auto gb = gif_buffer.begin();
    for (auto &fb : *framebuffer) {
      *++gb = static_cast<uint8_t>(fb.value * 0xffu);
      *++gb = static_cast<uint8_t>(fb.value * 0xffu);
      *++gb = static_cast<uint8_t>(fb.value * 0xffu);
      *++gb = static_cast<uint8_t>(fb.value * 0xffu);
    }
      GifWriteFrame(&gw, gif_buffer.data(), FRAME_WIDTH, FRAME_HEIGHT, 2);
    if (frame % 100 == 0) {
      printf("[VECTREX] frame = %d\n", frame);
    }
  }

  GifEnd(&gw);

  return 0;
}
