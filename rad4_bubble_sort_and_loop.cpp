//code for Windows to run well
#define _USE_MATH_DEFINES
#include <cmath>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <math.h>
#include <time.h>
#include <chrono>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

//#define __MAC__

#include <winsock2.h>
#ifdef _WIN32
#include <windows.h>
#include <glut.h>
#include <gl\GL.h>
#include <gl\GLU.h>
#endif

#ifdef __MAC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#endif



// ----------------------------------------------------------
struct RadLight
{
	int tileX;
	int tileY;
};

// ----------------------------------------------------------
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int TILE_SIZE = 25;
const int TILES_WIDE = SCREEN_WIDTH / TILE_SIZE;
const int TILES_HIGH = SCREEN_HEIGHT / TILE_SIZE;
const int TOTAL_TILES = TILES_WIDE * TILES_HIGH;

int drawSize = 4;

GLubyte *frameBuffer = NULL;
const int RED_OFS = 0;      /* offset to red byte */
const int GREEN_OFS = 1;    /* offset to green byte */
const int BLUE_OFS = 2;     /* offset to blue byte */

GLfloat * tiles = NULL;

// mouse
int LMBDown = 0;
int RMBDown = 0;
int mouseTileX = 0;
int mouseTileY = 0;

bool editMode = false;

// lights
const int NUM_VPLS = 64;
const int TOTAL_LIGHTS = 1 + NUM_VPLS;
RadLight lights[TOTAL_LIGHTS];

const int ONE_BILLION = 1000000000;


// ----------------------------------------------------------
void initGraphics(void)
{
	// set up famebuffer for 2D rendering
	frameBuffer = new GLubyte[SCREEN_WIDTH * SCREEN_HEIGHT * 3];
	memset(frameBuffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * 3);
	assert(frameBuffer != NULL);

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, SCREEN_WIDTH, 0.0, SCREEN_HEIGHT, 0.0, 1.0);

	glClearColor(1.0, 0.0, 0.0, 1.0);
}

// ----------------------------------------------------------
void cleanupGraphics(void)
{
	delete(frameBuffer);  frameBuffer = NULL;
}

// ----------------------------------------------------------
void setTile(int tileX, int tileY, GLfloat val)
{
	assert(tileX >= 0);
	assert(tileX < TILES_WIDE);
	assert(tileY >= 0);
	assert(tileY < TILES_HIGH);

	tiles[tileY * TILES_WIDE + tileX] = val;
}

// ----------------------------------------------------------
GLfloat getTile(int tileX, int tileY)
{
	// prevent ray from escaping grid.
	if (tileX < 0) return -1.0;
	if (tileY < 0) return -1.0;
	if (tileX >= TILES_WIDE) return -1.0;
	if (tileY >= TILES_HIGH) return -1.0;

	return tiles[tileY * TILES_WIDE + tileX];
}

// ----------------------------------------------------------
void initScene(void)
{
	tiles = new GLfloat[TILES_WIDE * TILES_HIGH];

	for (int i = 0; i < TILES_WIDE; i++){
		for (int j = 0; j < TILES_HIGH; j++){
			setTile(i, j, 0.0);
		}
	}
	setTile(10, 10, -1.0);
	setTile(10, 9, -1.0);

	setTile(15, 6, -1.0);

	setTile(30, 20, -1.0);
}

// ----------------------------------------------------------
void cleanupScene(void)
{
	delete(tiles);
	tiles = NULL;
}

// ----------------------------------------------------------
void setPixel(int x, int y, GLfloat r, GLfloat g, GLfloat b)
{
	assert(x >= 0 && x < SCREEN_WIDTH);
	assert(y >= 0 && y < SCREEN_HEIGHT);

	int pos = 3 * SCREEN_WIDTH*y + 3 * x;

	char red = (char)(r * 255);
	char green = (char)(g * 255);
	char blue = (char)(b * 255);

	frameBuffer[pos + RED_OFS] = red;
	frameBuffer[pos + GREEN_OFS] = green;
	frameBuffer[pos + BLUE_OFS] = blue;
}

// ----------------------------------------------------------
bool raycast(int x0, int y0, int x1, int y1, int &xhit, int &yhit)
{
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = abs(y1 - y0), sy = y0<y1 ? 1 : -1;
	int err = (dx>dy ? dx : -dy) / 2, e2;

	int x = x0;
	int y = y0;

	int stepped = 0;

	for (;;){
		if (getTile(x, y) < 0.0) {
			xhit = x - stepped*sx;
			yhit = y - stepped*sy;
			return false;

		}

		if (x == x1 && y == y1) break;
		e2 = err;
		if (e2 > -dx) { err -= dy; x += sx; }
		if (e2 < dy) { err += dx; y += sy; }
		stepped = 1;
	}

	return true;
}

// ----------------------------------------------------------
void spawnLights(void)
{
	lights[0].tileX = mouseTileX;
	lights[0].tileY = mouseTileY;

	//perform  bubble sort of lights
	int i, j, flag = 1;
	int temp_x;
	int temp_y;
	int size = TOTAL_LIGHTS;
	for (i = 0; i <= size && flag; i++){
		flag = 0;
		for (j = 0; j < (size - 1); j++){
			float jplus_val = lights[j + 1].tileY*TILES_WIDE + lights[j + 1].tileX;
			float j_val = lights[j].tileY*TILES_WIDE + lights[j].tileX;

			if (jplus_val < j_val){
				temp_x = lights[j].tileX;
				temp_y = lights[j].tileY;
				lights[j].tileX = lights[j + 1].tileX;
				lights[j].tileY = lights[j + 1].tileY;
				lights[j + 1].tileX = temp_x;
				lights[j + 1].tileY = temp_y;
				flag = 1;
			}
		}
	}



	// *** CAST FIRST BOUNCE ***
	float deltaDegrees = 360.0f / (float)NUM_VPLS;
	float deltaRadians = deltaDegrees * M_PI / 180.0f;
	float twopi = 2.0f * M_PI;

	int vpl = 1;

	float radians = 0.0f;
	while (radians < twopi)
	{
		float xdir = cos(radians);
		float ydir = sin(radians);

		// make it big enough to span the screen
		int destX = mouseTileX + floor(xdir * 5000);
		int destY = mouseTileY + floor(ydir * 5000);

		int hitX;
		int hitY;
		bool rayfree = raycast(mouseTileX, mouseTileY, destX, destY, hitX, hitY);
		assert(!rayfree); // must always hit something

		lights[vpl].tileX = hitX;
		lights[vpl].tileY = hitY;

		radians += deltaRadians;
		vpl++;

	}
}

// ----------------------------------------------------------
void renderScene(void)
{
	// clear screen
	memset(frameBuffer, 0x22, SCREEN_WIDTH * SCREEN_HEIGHT * 3);

	spawnLights();

	// cast light to each pixel
	for (int i = 0; i < TILES_WIDE; i++)
	{
		for (int j = 0; j < TILES_HIGH; j++)
		{
			// only cast from open tiles
			if (getTile(i, j) >= 0.0)
			{
				if (editMode)
				{
					setTile(i, j, 1.0f);
				}
				else
				{
					int lightsSeen = 0;
					int hitX;
					int hitY;

					for (int n = 0; n < TOTAL_LIGHTS; n++)
					{
						if (raycast(i, j, lights[n].tileX, lights[n].tileY, hitX, hitY))
						{
							lightsSeen++;
						}
					}

					setTile(i, j, 1.0f * lightsSeen / (1.0f * TOTAL_LIGHTS));
				}
			}
		}
	}

	//three difference scenarios- tile size 1, 2 or divisible by 5
	//tile size divisibile by 5
	if ((TILE_SIZE % 5) == 0){
		// *** render world pixels based on their intensities ***
		for (int i = 0; i < TILES_WIDE; i += 1)
		{
			for (int j = 0; j <= TILES_HIGH - 5; j += 5)
			{
				// convert blocker tiles to black
				float intensity[5];
				intensity[0] = getTile(i, j);
				intensity[1] = getTile(i, j + 1);
				intensity[2] = getTile(i, j + 2);
				intensity[3] = getTile(i, j + 3);
				intensity[4] = getTile(i, j + 4);
				//float intensity_2 = getTile((i + 1), (j + 1));
				//float intensit
				for (int k = 0; k < 5; k++){
					if (intensity[k] < 0.0){
						intensity[k] = 0.0;
					}
				}

				for (int x = 0; x < TILE_SIZE; x++) {
					for (int y = 0; y < TILE_SIZE; y++) {
						setPixel(i * TILE_SIZE + x, j * TILE_SIZE + y, intensity[0], intensity[0], intensity[0]);
						setPixel(i *TILE_SIZE + x, (j + 1) * TILE_SIZE + y, intensity[1], intensity[1], intensity[1]);
						setPixel(i  *TILE_SIZE + x, (j + 2) * TILE_SIZE + y, intensity[2], intensity[2], intensity[2]);
						setPixel(i *TILE_SIZE + x, (j + 3) * TILE_SIZE + y, intensity[3], intensity[3], intensity[3]);
						setPixel(i *TILE_SIZE + x, (j + 4) * TILE_SIZE + y, intensity[4], intensity[4], intensity[4]);
					}
				}
			}
		}


	}

	//tile size divisible by 2
	else if ((TILE_SIZE % 2) == 0){
		// *** render world pixels based on their intensities ***
		for (int i = 0; i < TILES_WIDE; i += 1)
		{
			for (int j = 0; j < TILES_HIGH - 1; j += 2)
			{
				// convert blocker tiles to black
				float intensity = getTile(i, j);
				float intensity_2 = getTile((i), (j + 1));
				if (intensity < 0.0) {
					intensity = 0.0;
				}
				if (intensity_2 < 0.0){
					intensity_2 = 0.0;
				}
				for (int x = 0; x < TILE_SIZE; x++) {
					for (int y = 0; y < TILE_SIZE; y++) {
						setPixel(i * TILE_SIZE + x, j * TILE_SIZE + y, intensity, intensity, intensity);
						setPixel(i *TILE_SIZE + x, (j + 1) * TILE_SIZE + y, intensity_2, intensity_2, intensity_2);
					}
				}
			}
		}
	}


	//tile_size is 1
	else {
		// *** render world pixels based on their intensities ***
		for (int i = 0; i < TILES_WIDE; i++)
		{
			for (int j = 0; j < TILES_HIGH; j++)
			{
				// convert blocker tiles to black
				float intensity = getTile(i, j);
				if (intensity < 0.0) intensity = 0.0;

				for (int x = 0; x < TILE_SIZE; x++)
					for (int y = 0; y < TILE_SIZE; y++)
						setPixel(i * TILE_SIZE + x, j * TILE_SIZE + y, intensity, intensity, intensity);
			}
		}
	}
}

// ----------------------------------------------------------
void displayCallback() {
	glClear(GL_COLOR_BUFFER_BIT);

	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::nanoseconds ns;
	typedef std::chrono::duration<float> fsec;
	auto t0 = Time::now();

	renderScene();

	auto t1 = Time::now();
	fsec duration = t1 - t0;
	ns nanoseconds = std::chrono::duration_cast<ns>(duration);
	std::cout << "\nRendered " << TOTAL_TILES << "in " << nanoseconds.count() << " nanoseconds";

	// present frame buffer to screen
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glRasterPos3f(0.0, 0.0, 0.0);
	glDrawPixels(SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, frameBuffer);

	glFlush();
	glutSwapBuffers();
}

// ----------------------------------------------------------
void mouseCallback(int button, int state, int x, int y)
{
	mouseTileX = x / TILE_SIZE;
	mouseTileY = (SCREEN_HEIGHT - y) / TILE_SIZE;
	printf("mouse move %d %d\n", mouseTileX, mouseTileY);

	LMBDown = (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
	RMBDown = (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);

	if (editMode)
	{
		int xEnd = mouseTileX + drawSize;
		int yEnd = mouseTileY + drawSize;

		if (xEnd > TILES_WIDE) xEnd = TILES_WIDE;
		if (yEnd > TILES_HIGH) yEnd = TILES_HIGH;

		if (LMBDown)
		{
			for (int i = mouseTileX; i < xEnd; i++)
				for (int j = mouseTileY; j < yEnd; j++)
					setTile(i, j, -1.0);
		}
		else if (RMBDown)
		{
			for (int i = mouseTileX; i < xEnd; i++)
				for (int j = mouseTileY; j < yEnd; j++)
					setTile(i, j, 0.0);
		}
	}
	glutPostRedisplay();
}

// ----------------------------------------------------------
void motionCallback(int x, int y)
{
	mouseCallback(0, 0, x, y);
}

// ----------------------------------------------------------
// custom keyFunc with preset keys
void keyCallback(unsigned char ch, int x, int y) {
	switch (ch) {
	case '1':
	{
		printf("Draw size changed to 1.\n");
		drawSize = 1;
		break;
	}
	case '2':
	{
		printf("Draw size changed to 2.\n");
		drawSize = 2;
		break;
	}
	case '3':
	{
		printf("Draw size changed to 3.\n");
		drawSize = 3;
		break;
	}
	case '4':
	{
		printf("Draw size changed to 4.\n");
		drawSize = 4;
		break;
	}
	case 'd':
	case 'D':
	{
		editMode = !editMode;

		if (editMode)
			printf("Edit mode ON.\n");
		else
			printf("Edit mode OFF.\n");

		break;
	}
	}

	glutPostRedisplay();
}

// ----------------------------------------------------------
void idleCallback(void)
{
	//no need for this call since any input redraws.
	//we dont care about FPS, just render time for a frame.
	//glutPostRedisplay();
}

// ----------------------------------------------------------
int main(int argc, char **argv)
{
	initScene();
	glutInit(&argc, argv);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	int win = glutCreateWindow("2D Radiosity Demo");
	glutSetWindow(win);

	glutKeyboardFunc(keyCallback);
	glutMouseFunc(mouseCallback);
	glutMotionFunc(motionCallback);
	glutDisplayFunc(displayCallback);
	glutIdleFunc(idleCallback);

	initGraphics();


	glutMainLoop();

	cleanupScene();
	cleanupGraphics();
}