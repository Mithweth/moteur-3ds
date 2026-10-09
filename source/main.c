// main.c
// Program entry point: initializes the 3DS services and every engine module,
// runs the main loop (input -> audio -> game update -> draw) until START is
// pressed or the system asks the application to quit, then tears everything
// down in reverse order. Built with DEBUG, stdout/stderr are sent over the
// network to 3dslink.
#include <citro2d.h>
#include <stdlib.h>
#include <3ds.h>
#include <time.h>
#include "game.h"
#include "lang.h"
#include "hud.h"
#include "audio.h"
#include "gamestate.h"

#ifdef DEBUG

// Redirects stdout/stderr to the 3dslink host (run with `3dslink -s`).

#include <malloc.h>
#include <unistd.h>

static u32 *soc_buffer;
static int debug_fd = -1;

static void debug_init(void) {
	soc_buffer = memalign(0x1000, 0x100000);
	if (!soc_buffer) {
		return;
	}
	Result rc = socInit(soc_buffer, 0x100000);
	if (R_SUCCEEDED(rc)) {
		debug_fd = link3dsStdio();
	}
}

static void debug_close(void) {
	fflush(stdout);
	fflush(stderr);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	if (debug_fd >= 0) {
		close(debug_fd);
	}
	socExit();
	free(soc_buffer);
}

#else
static void debug_init(void) {}
static void debug_close(void) {}
#endif

static aptHookCookie apt_cookie;

// When the game comes back from the HOME menu or from sleep mode, restart the
// timer's reference time so the time spent away is not counted.
static void apt_callback(APT_HookType hook, void *param) {
	if (hook == APTHOOK_ONRESTORE || hook == APTHOOK_ONWAKEUP) {
		timer_resume();
	}
}

int main(int argc, char **argv) {
	int ret = 0;
	gfxInitDefault();
	romfsInit();
	debug_init();
	if (!lang_init()) {
		ret = 1;
	}
	if (!gamestate_init("romfs:/states/game.state")) {
		ret = 1;
	}
	srand(time(NULL));
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	C3D_RenderTarget *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	aptHook(&apt_cookie, apt_callback, NULL);
	// audio_init never fails: without a DSP firmware the game runs silently.
	audio_init();
	// The inventory and the HUD are loaded once for the whole run; returns to
	// the title screen go through game_title_start instead.
	if (ret == 0 && !game_init()) {
		printf("Cannot initialize interface\n");
		ret = 1;
	}
	// ret != 0 means initialization failed: skip the loop and clean up.
	while (ret == 0 && aptMainLoop()) {
		hidScanInput();
		u32 keys = hidKeysDown();

		circlePosition analog;
		hidCircleRead(&analog);

		touchPosition touch;
		hidTouchRead(&touch);

		audio_update();
		if (!game_update(keys, analog, touch)) {
			break;
		}
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		game_draw(top, bottom);
		C3D_FrameEnd(0);
	}

	// game_close must run before audio_close (closing a mini-game or a
	// timeline still uses NDSP) and before C2D_Fini (it frees C2D objects).
	aptUnhook(&apt_cookie);
	game_close();
	gamestate_close();
	audio_close();
	C2D_Fini();
	C3D_Fini();
	lang_close();
	romfsExit();
	gfxExit();
	debug_close();
    
	return ret;
}
