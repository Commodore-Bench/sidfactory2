#pragma once

#define EMULATION_CYCLES_PER_SECOND_PAL		985248		// Seemingly a magic number, but is the number of cycles a commodore 64 in PAL mode executes per second
#define EMULATION_CYCLES_PER_SECOND_NTSC	1022727		// Seemingly a magic number, but is the number of cycles a commodore 64 in NTSC mode executes per second
#define EMULATION_FRAMES_PER_SECOND_PAL		50.125f		// Actually 50.125
#define EMULATION_FRAMES_PER_SECOND_NTSC	59.826f		// Actually 59.826
#define EMULATION_CYCLES_PER_SCANLINE_PAL	63
#define EMULATION_CYCLES_PER_SCANLINE_NTSC	65
#define EMULATION_SCANLINES_PER_FRAME_PAL	312
#define EMULATION_SCANLINES_PER_FRAME_NTSC	263
#define EMULATION_CYCLES_PER_FRAME_PAL		(EMULATION_CYCLES_PER_SCANLINE_PAL * EMULATION_SCANLINES_PER_FRAME_PAL)
#define EMULATION_CYCLES_PER_FRAME_NTSC		(EMULATION_CYCLES_PER_SCANLINE_NTSC * EMULATION_SCANLINES_PER_FRAME_NTSC)

