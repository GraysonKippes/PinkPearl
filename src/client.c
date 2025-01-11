#include "client.h"

#include <stdbool.h>
#include "game/game.h"
#include "glfw/GLFWManager.h"
#include "render/RenderManager.h"
#include "util/Time.h"

static bool clientRunning = false;

void runClient(void) {
	clientRunning = true;
	
	static const double tickPerSecond = 20.0;	// ticks / s
	static const double msPerTick = 1000.0 / tickPerSecond; // ms

	uint64_t previousTime = getTimeMS();	// ms
	float tickDelta = 0.0F; // Unitless interpolation value

	while (clientRunning && !shouldAppWindowClose()) {

		const uint64_t currentTime = getTimeMS();	// ms
		const uint64_t deltaTime = currentTime - previousTime;	// ms
		previousTime = currentTime;
		tickDelta += (float)(deltaTime / msPerTick);

		while (tickDelta >= 1.0F) {
			tick_game();
			tickDelta -= 1.0F;
		}

		if (clientRunning && !shouldAppWindowClose()) {
			const GameState gameState = getGameState();
			renderFrame(tickDelta, areaGetCameraPosition(&currentArea), areaGetProjectionBounds(currentArea), !gameState.paused && !gameState.scrolling);
		} else {
			break;
		}
	}
	
	clientRunning = true;
}
