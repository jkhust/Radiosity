#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <math.h>

//#define __MAC__

//#ifdef _WIN32
#include <windows.h>
//#endif
//#ifdef __MAC__
//#include <OpenGL/gl.h>
//#include <OpenGL/glu.h>
//#include <GLUT/glut.h>
//#else
#include <GL/gl.h>
#include <GL/glut.h>
//#endif

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
const int TILE_SIZE=5;
const int TILES_WIDE = SCREEN_WIDTH/TILE_SIZE;
const int TILES_HIGH = SCREEN_HEIGHT/TILE_SIZE;
const int FULLBRIGHT_PIXELS = SCREEN_WIDTH / 5;

GLubyte *frameBuffer = NULL;
const int RED_OFS = 0;      /* offset to red byte */
const int GREEN_OFS = 1;    /* offset to green byte */
const int BLUE_OFS = 2;     /* offset to blue byte */

GLfloat * tiles = NULL;

int LMBDown=0;
int RMBDown=0;
int mouseTileX=0;
int mouseTileY=0;

bool editMode = false;

// ----------------------------------------------------------
inline float maximum(float a, float b)
{
	if(a > b)
		return a;
	else
		return b;
}

// ----------------------------------------------------------
inline float minimum(float a, float b)
{
	if(a < b)
		return a;
	else
		return b;
}

// ----------------------------------------------------------
void initGraphics(void)
{
	// set up famebuffer for 2D rendering
	frameBuffer = new GLubyte[SCREEN_WIDTH * SCREEN_HEIGHT * 3];
	memset(frameBuffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * 3);
	assert(frameBuffer != NULL);

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity( );
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity( );
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
	tiles[tileY * TILES_WIDE + tileX] = val;
}

// ----------------------------------------------------------
GLfloat getTile(int tileX, int tileY)
{
	return tiles[tileY * TILES_WIDE + tileX];
}

// ----------------------------------------------------------
void initScene(void)
{
	tiles = new GLfloat[TILES_WIDE * TILES_HIGH];

	//assuming tiles size either divisible by 2 or 1;
	//if size divisible by 2
	if(TILES_WIDE %2==0 && TILES_HIGH%2==0){
		for(int i=0;i<TILES_WIDE-2;i+=2){
			for(int j=0;j<TILES_HIGH-2;j+=2){
				setTile(i, j, 0.0);
				setTile(i+1, j+1, 0.0);
			}
		}
	}
	else {
		for(int i=0;i<TILES_WIDE;i+=1){
			for(int j=0;j<TILES_HIGH;j+=1){
				setTile(i, j, 0.0);
			}
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

	int pos = 3*SCREEN_WIDTH*y + 3*x;

	char red =  (char) (r*255);
	char green = (char) (g*255);
	char blue = (char) (b*255);

	frameBuffer[pos + RED_OFS] = red;
	frameBuffer[pos + GREEN_OFS] = green;
	frameBuffer[pos + BLUE_OFS] = blue;
}

// ----------------------------------------------------------

bool raycast(int x0, int y0, int x1, int y1, int &xhit, int &yhit, float &dist)
{
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
	int err = (dx>dy ? dx : -dy)/2, e2;

	int x=x0;
	int y=y0;

	while(1){
		if( getTile(x, y) < 0.0) {
			xhit=x;
			yhit=y;
			//dist = sqrt( (x-x0)*(x-x0) + (y-y0)*(y-y0) );
			return false;

		}

		if (x==x1 && y==y1) break;
		e2 = err;
		if (e2 >-dx) { err -= dy; x += sx; }
		if (e2 < dy) { err += dx; y += sy; }
	}

	dist = sqrt( (float) (x1-x0)*(x1-x0) + (y1-y0)*(y1-y0) );
	return true;
}

// ----------------------------------------------------------

/going to need to create threads below
void renderScene(void)
{
	// clear screen
	memset(frameBuffer, 0x22, SCREEN_WIDTH * SCREEN_HEIGHT * 3);

	// TODO: cast VPLs out from current mouse position

	// cast light to each pixel
	//planning on breaking up grid into quadrants -> thread for each quadrant
	//code below will be taken out into separate method
	for(int i=0;i<TILES_WIDE;i++)
	{
		for(int j=0;j<TILES_HIGH;j++)
		{
			// only cast from open tiles
			if(getTile(i,j) >= 0.0)
			{
				if(editMode)
				{
					setTile(i,j,1.0f);
				}
				else
				{
					int hitX=0,hitY=0;
					float dist = 0.0;
					if(raycast(i, j, mouseTileX, mouseTileY, hitX, hitY, dist))
					{
						float distFactor = maximum( (1.0f * dist * TILE_SIZE) / (float) FULLBRIGHT_PIXELS, 1.0f);
						float bottomTerm = 1.0f + (0.25f * distFactor) + (0.125f * distFactor * distFactor);
						float intensity = 1.0f / bottomTerm;
						setTile(i,j, intensity);  
					}
					else
					{
						setTile(i,j, 0.0);
					}
				}
			}
		}
	}

	// *** render world pixels based on their intensities ***
	for(int i=0;i<TILES_WIDE;i++)
	{
		for(int j=0;j<TILES_HIGH;j++)
		{
			// convert blocker tiles to black
			float intensity = maximum(getTile(i, j), 0.0);

			for(int x=0;x < TILE_SIZE;x++)
				for(int y=0;y < TILE_SIZE;y++)
					setPixel(i * TILE_SIZE + x, j * TILE_SIZE + y, intensity, intensity, intensity);
		}
	}
	//end of code being taken out
}

// ----------------------------------------------------------
void displayCallback() {
	glClear(GL_COLOR_BUFFER_BIT);

	renderScene();

	// present frame buffer to screen
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glRasterPos3f(0.0,0.0,0.0);
	glDrawPixels(SCREEN_WIDTH,SCREEN_HEIGHT,GL_RGB,GL_UNSIGNED_BYTE,frameBuffer);
	glFlush();
	glutSwapBuffers();
}

// ----------------------------------------------------------
void mouseCallback(int button, int state, int x, int y)
{
	mouseTileX = x / TILE_SIZE;
	mouseTileY = (SCREEN_HEIGHT-y) / TILE_SIZE;
	printf("%d %d\n", mouseTileX, mouseTileY);

	LMBDown = (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
	RMBDown = (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);

	if(editMode)
	{
		if(LMBDown)
			setTile(mouseTileX,mouseTileY, -1.0);
		else if(RMBDown)
			setTile(mouseTileX,mouseTileY, 0.0);  
	}
}

// ----------------------------------------------------------
void motionCallback(int x, int y)
{
	mouseTileX = x / TILE_SIZE;
	mouseTileY = (SCREEN_HEIGHT-y) / TILE_SIZE;
	printf("%d %d\n", mouseTileX, mouseTileY);

	if(editMode)
	{
		if(LMBDown)
			setTile(mouseTileX,mouseTileY, -1.0);
		else if(RMBDown)
			setTile(mouseTileX,mouseTileY, 0.0);  
	}
}

// ----------------------------------------------------------
// custom keyFunc with preset keys
void keyCallback(unsigned char ch, int x, int y) {
	switch(ch) {
	case 'd':
	case 'D':
		editMode = !editMode;
		break;
	}
}

// ----------------------------------------------------------
void idleCallback(void)
{
	glutPostRedisplay();
}

// ----------------------------------------------------------
int main(int argc, char **argv)
{
	//moving here to see if works
	tiles = new GLfloat[TILES_WIDE * TILES_HIGH];

	glutInit(&argc,argv);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(SCREEN_WIDTH,SCREEN_HEIGHT);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	int win = glutCreateWindow("2D Radiosity Demo");
	glutSetWindow(win);

	glutKeyboardFunc(keyCallback);
	glutMouseFunc(mouseCallback);
	glutMotionFunc(motionCallback);
	glutDisplayFunc(displayCallback);
	glutIdleFunc(idleCallback);

	initGraphics();
	initScene();

	glutMainLoop();

	cleanupScene();
	cleanupGraphics();
}
