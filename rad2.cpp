#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <math.h>

#define __MAC__

#ifdef _WIN32
#include <windows.h>
#endif
#ifdef __MAC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int TILE_SIZE=20;
const int TILES_WIDE = SCREEN_WIDTH/TILE_SIZE;
const int TILES_HIGH = SCREEN_HEIGHT/TILE_SIZE;
const int FALLOFF_PIXELS = 200;

GLubyte *frameBuffer = NULL;
const int RED_OFS = 0;      /* offset to red byte */
const int GREEN_OFS = 1;    /* offset to green byte */
const int BLUE_OFS = 2;     /* offset to blue byte */

GLfloat * tiles = NULL;

int LMBDown=0;
int RMBDown=0;
int mouseTileX=0;
int mouseTileY=0;

bool drawMode = false;

// ----------------------------------------------------------
inline float max(float a, float b)
{
  if(a > b)
    return a;
  else
    return b;
}

// ----------------------------------------------------------
inline float min(float a, float b)
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
  
  for(int i=0;i<TILES_WIDE;i++)
    for(int j=0;j<TILES_HIGH;j++)
      setTile(i, j, 0.0);

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
bool raycast(int srcX, int srcY, int destX, int destY, int &hitX, int &hitY, float &dist)
{
  // source is blocked
  if( getTile(srcX, srcY) < 0.0 ) return true;
 
  // normalize a 2d vector to the light
  float deltaX = destX - srcX;
  float deltaY = destY - srcY;
  float mag = sqrt(deltaX * deltaX + deltaY * deltaY);
  deltaX /= mag;
  deltaY /= mag;
  
  float posX = srcX;
  float posY = srcY;
  float distSoFar = 0.0f;
  
  while( (int) posX != destX && (int) posY != destY)
  {
    if( getTile(posX, posY) < 0.0 )
    {
      hitX = posX - deltaX;
      hitY = posY - deltaY;
      dist = distSoFar - mag;
      return false;
    }
    
    posX += deltaX;
    posY += deltaY;
    distSoFar += mag;
  }
  
  dist = sqrt( (destX - srcX) * (destX - srcX) + (destY - srcY) * (destY - srcY)); 
  return true;
}

// ----------------------------------------------------------
void renderScene(void)
{
  // clear screen
  memset(frameBuffer, 0x22, SCREEN_WIDTH * SCREEN_HEIGHT * 3);

  // cast VPLs out from current mouse position


  // cast light to each pixel
  for(int i=0;i<TILES_WIDE;i++)
  {
    for(int j=0;j<TILES_HIGH;j++)
    {
      // only cast from open tiles
      if(getTile(i,j) >= 0.0)
      {
        int hitX=0,hitY=0;
        float dist = 0.0;
        if(raycast(i, j, mouseTileX, mouseTileY, hitX, hitY, dist))
        {
          printf("hit\n");
          //float intensity = max(FALLOFF_PIXELS / (dist * TILE_SIZE), 1.0);
          setTile(i,j,1.0);  
        }
        else
        {
          setTile(i,j, 0.75);
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
      float intensity = max(getTile(i, j), 0.0);
  
      for(int x=0;x < TILE_SIZE;x++)
        for(int y=0;y < TILE_SIZE;y++)
          setPixel(i * TILE_SIZE + x, j * TILE_SIZE + y, intensity, intensity, intensity);
    }
  }
}

// ----------------------------------------------------------
void displayCallback() {
  glClear(GL_COLOR_BUFFER_BIT);
  
  renderScene();
  
  // present frame buffer to screen
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glRasterPos3f(0.0,0.0,0.0);
  glDrawPixels(SCREEN_WIDTH,SCREEN_HEIGHT,GL_RGB,GL_UNSIGNED_BYTE,frameBuffer);

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

  if(drawMode)
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
  
  if(drawMode)
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
			drawMode = !drawMode;
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
