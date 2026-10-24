// Plugin de PSP para PPSSPP: muestra 8 flechas arriba al centro de la pantalla
// que se iluminan segun la direccion del stick analogico izquierdo.
//
// Lee el stick con sceCtrlPeekBufferPositive (no consume ni cambia los controles
// del juego) y dibuja directo sobre el framebuffer que se esta mostrando.

#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>

PSP_MODULE_INFO("StickFlechas", 0, 1, 0);

// ---- Ajustes (cambialos y vuelve a compilar) ----
#define SCREEN_W      480
#define SCREEN_H      272
#define CENTER_X      240   // centro del circulo de flechas
#define CENTER_Y      34
#define RING_RADIUS   20    // distancia de las flechas al centro
#define ARROW_SIZE    7     // tamano de una flecha apagada
#define ARROW_SIZE_ON 10    // tamano de la flecha encendida
#define DEADZONE      45    // 0..127, cuanto hay que mover el stick (~0.35)

// Direcciones en pantalla: derecha, abajo-der, abajo, abajo-izq, izq, arriba-izq, arriba, arriba-der
static const int dirX[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static const int dirY[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

static inline float absf(float v) { return v < 0 ? -v : v; }

static void put_pixel(void *fb, int bufw, int fmt, int x, int y, int r, int g, int b) {
	if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H)
		return;
	int idx = y * bufw + x;
	switch (fmt) {
	case PSP_DISPLAY_PIXEL_FORMAT_565:
		((unsigned short *)fb)[idx] = (unsigned short)(((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3));
		break;
	case PSP_DISPLAY_PIXEL_FORMAT_5551:
		((unsigned short *)fb)[idx] = (unsigned short)(0x8000 | ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3));
		break;
	case PSP_DISPLAY_PIXEL_FORMAT_4444:
		((unsigned short *)fb)[idx] = (unsigned short)(0xF000 | ((b >> 4) << 8) | ((g >> 4) << 4) | (r >> 4));
		break;
	default: // 8888
		((unsigned int *)fb)[idx] = 0xFF000000u | ((unsigned)b << 16) | ((unsigned)g << 8) | (unsigned)r;
		break;
	}
}

// Triangulo relleno que apunta hacia (dx, dy), centrado en (cx, cy).
static void draw_arrow(void *fb, int bufw, int fmt, int cx, int cy, int dx, int dy, int s, int r, int g, int b) {
	float len = (dx != 0 && dy != 0) ? 0.7071f : 1.0f;
	float ux = dx * len, uy = dy * len;  // eje de la flecha
	for (int py = -s; py <= s; py++) {
		for (int px = -s; px <= s; px++) {
			float u = px * ux + py * uy;     // a lo largo de la flecha (punta en +s)
			float v = -px * uy + py * ux;    // a lo ancho
			if (u >= -s && u <= s && absf(v) <= (s - u) * 0.5f)
				put_pixel(fb, bufw, fmt, cx + px, cy + py, r, g, b);
		}
	}
}

int main(int argc, char **argv) {
	// Esperar un poco a que el juego arranque.
	sceKernelDelayThread(3 * 1000 * 1000);

	while (1) {
		sceDisplayWaitVblankStart();

		SceCtrlData pad;
		if (sceCtrlPeekBufferPositive(&pad, 1) < 1)
			continue;

		void *fb = 0;
		int bufw = 0, fmt = 0;
		sceDisplayGetFrameBuf(&fb, &bufw, &fmt, PSP_DISPLAY_SETBUF_IMMEDIATE);
		if (!fb || bufw <= 0)
			continue;
		// Direccion sin cache para que se vea al instante.
		fb = (void *)((unsigned int)fb | 0x40000000);

		int ax = (int)pad.Lx - 128;
		int ay = (int)pad.Ly - 128;  // en la PSP, Ly crece hacia abajo
		int heldX = ax > DEADZONE ? 1 : (ax < -DEADZONE ? -1 : 0);
		int heldY = ay > DEADZONE ? 1 : (ay < -DEADZONE ? -1 : 0);

		for (int i = 0; i < 8; i++) {
			int active = (heldX || heldY) && dirX[i] == heldX && dirY[i] == heldY;
			float len = (dirX[i] != 0 && dirY[i] != 0) ? 0.7071f : 1.0f;
			int x = CENTER_X + (int)(dirX[i] * len * RING_RADIUS);
			int y = CENTER_Y + (int)(dirY[i] * len * RING_RADIUS);
			if (active)
				draw_arrow(fb, bufw, fmt, x, y, dirX[i], dirY[i], ARROW_SIZE_ON, 64, 224, 255);
			else
				draw_arrow(fb, bufw, fmt, x, y, dirX[i], dirY[i], ARROW_SIZE, 90, 90, 90);
		}
	}
	return 0;
}
