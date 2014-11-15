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

GLubyte *frameBuffer = NULL;
const int RED_OFS = 0;      /* offset to red byte */
const int GREEN_OFS = 1;    /* offset to green byte */
const int BLUE_OFS = 2;     /* offset to blue byte */

GLfloat * tiles = NULL;

int LMBDown=0;
int RMBDown=0;
int mouseTileX=0;
int mouseTileY=0;

// ----------------------------------------------------------
inline float max(float a, float b)
{
  if(a > b)
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

  setTile(1, 1, -1.0);
  setTile(2, 2, -1.0);
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
void renderScene(void)
{
  // clear screen
  memset(frameBuffer, 0x22, SCREEN_WIDTH * SCREEN_HEIGHT * 3);

  // clear illumination tiles
  for(int i=0;i<TILES_WIDE;i++)
    for(int j=0;j<TILES_HIGH;j++)
      if(getTile(i,j) > 0.0)
        setTile(i,j, 0.0);

  for(int xTile=0;xTile<TILES_WIDE;xTile++)
  {
    for(int yTile=0;yTile<TILES_HIGH;yTile++)
    {
      float r,g,b;
    
      if(getTile(xTile, yTile) < 0.0f)
      {
        r = 0.0f; g = 0.0f; b = 0.0f;
      }
      else
      {
        float dist = sqrt( (xTile - mouseTileX)*(xTile-mouseTileX) + 
                         (yTile - mouseTileY)*(yTile-mouseTileY));
      
        float brightness = max(1.0f - dist*0.025f, 0.0);
      
        r = brightness; g = brightness; b = brightness;      
      }
    
      for(int xofs=0;xofs < TILE_SIZE;xofs++)
        for(int yofs=0;yofs < TILE_SIZE;yofs++)
          setPixel(xTile * TILE_SIZE + xofs, yTile * TILE_SIZE + yofs, r, g, b);
    }
  }
    
    
  // clear the framebuffer to gray
//  for(int j=100;j<400;j++)
//    setPixel(100, j, 1.0, 0.0, 0.0);
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

//  if(LMBDown)
//    setTile(mouseTileX,mouseTileY, -1.0);
//  else if(RMBDown)
//    setTile(mouseTileX,mouseTileY, 0.0);
}

// ----------------------------------------------------------
void motionCallback(int x, int y)
{
  mouseTileX = x / TILE_SIZE;
  mouseTileY = (SCREEN_HEIGHT-y) / TILE_SIZE;
  printf("%d %d\n", mouseTileX, mouseTileY);
  
//  if(LMBDown)
//    setTile(mouseTileX,mouseTileY, -1.0);
//  else if(RMBDown)
//    setTile(mouseTileX,mouseTileY, 0.0);
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
  //glutKeyboardFunc(keyFunc);
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