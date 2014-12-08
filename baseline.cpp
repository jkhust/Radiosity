#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <math.h>
#include <chrono>

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

// ----------------------------------------------------------
struct RadLight
{
  int tileX;
  int tileY;
};

// ----------------------------------------------------------
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int SAMPLES_TO_TAKE = 20;

int TILE_SIZE = 0;
int TILES_WIDE = 0;
int TILES_HIGH = 0;
int TOTAL_TILES = 0;

int drawSize = 2;

GLubyte *frameBuffer = NULL;
const int RED_OFS = 0;      /* offset to red byte */
const int GREEN_OFS = 1;    /* offset to green byte */
const int BLUE_OFS = 2;     /* offset to blue byte */

GLfloat * tiles = NULL;

// mouse
int LMBDown=0;
int RMBDown=0;
int mouseTileX=0;
int mouseTileY=0;

bool drawMode = false;
bool eraseMode = false;

// lights
const int NUM_VPLS = 64;
const int TOTAL_LIGHTS = 1 + NUM_VPLS;
RadLight lights[TOTAL_LIGHTS];

const int ONE_BILLION = 1000000000;

// ----------------------------------------------------------
int random_between(int min, int max) {
  return rand() % (max-min) + min;  
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
  if(tileX < 0) return -1.0;
  if(tileY < 0) return -1.0;
  if(tileX >= TILES_WIDE) return -1.0;
  if(tileY >= TILES_HIGH) return -1.0;

  return tiles[tileY * TILES_WIDE + tileX];
}

// ----------------------------------------------------------
void initScene(void)
{
  tiles = new GLfloat[TILES_WIDE * TILES_HIGH];
 
  for(int i=0;i<TILES_WIDE;i++)
    for(int j=0;j<TILES_HIGH;j++)
      setTile(i, j, 0.0);

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
bool raycast(int x0, int y0, int x1, int y1, int &xhit, int &yhit)
{
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
  int err = (dx>dy ? dx : -dy)/2, e2;

  int x=x0;
  int y=y0;
  
  int stepped=0;
 
  for(;;){
    if( getTile(x, y) < 0.0) {
     xhit=x-stepped*sx;
     yhit=y-stepped*sy;
     return false;
     
    }
  
    if (x==x1 && y==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x += sx; }
    if (e2 < dy) { err += dx; y += sy; }
    stepped=1;
  }
  
  return true;
}

// ----------------------------------------------------------
void spawnLights(void)
{
  lights[0].tileX = mouseTileX;
  lights[0].tileY = mouseTileY;
  
  // *** CAST FIRST BOUNCE ***
  float deltaDegrees = 360.0f / (float) NUM_VPLS;
  float deltaRadians = deltaDegrees * M_PI / 180.0f;
  float twopi = 2.0f * M_PI;

  int vpl=1;

  float radians=0.0f;
  while(radians < twopi )
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
void renderSceneEditable(void)
{
  // clear screen
  memset(frameBuffer, 0x22, SCREEN_WIDTH * SCREEN_HEIGHT * 3);

  for(int i=0;i<TILES_WIDE;i++)
  {
    for(int j=0;j<TILES_HIGH;j++)
    {
      int intensity = (getTile(i,j) == -1) ? 0.0 : 1.0;
      
      for(int x=0;x < TILE_SIZE;x++)
        for(int y=0;y < TILE_SIZE;y++)
          setPixel(i * TILE_SIZE + x, j * TILE_SIZE + y, intensity, intensity, intensity);
          
    }
  }
}

// ----------------------------------------------------------
void renderScene(void)
{
  // clear screen
  memset(frameBuffer, 0x22, SCREEN_WIDTH * SCREEN_HEIGHT * 3);

  spawnLights();
  
  // cast light to each pixel
  for(int i=0;i<TILES_WIDE;i++)
  {
    for(int j=0;j<TILES_HIGH;j++)
    {
      // only cast from open tiles
      if(getTile(i,j) != -1)
      {
        int lightsSeen = 0;
        int hitX;
        int hitY;
        
        for(int n=0;n<TOTAL_LIGHTS; n++)
        {
          if(raycast(i, j, lights[n].tileX, lights[n].tileY, hitX, hitY))
          {
            lightsSeen++;
          }
        }

        setTile(i,j, 1.0f * lightsSeen / (1.0f * TOTAL_LIGHTS));
      }
    }
  }
  
  // *** render world pixels based on their intensities ***
  for(int i=0;i<TILES_WIDE;i++)
  {
    for(int j=0;j<TILES_HIGH;j++)
    {
      // convert blocker tiles to black
      float intensity = getTile(i,j);
      if(intensity < 0.0) intensity = 0.0;
      
      for(int x=0;x < TILE_SIZE;x++)
        for(int y=0;y < TILE_SIZE;y++)
          setPixel(i * TILE_SIZE + x, j * TILE_SIZE + y, intensity, intensity, intensity);

    }
  }
}

// ----------------------------------------------------------
void displayCallback() {
  glClear(GL_COLOR_BUFFER_BIT);
  
  if(drawMode || eraseMode)
    renderSceneEditable();
  else
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
  printf("mouse move %d %d\n", mouseTileX, mouseTileY);
  
  LMBDown = (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);

  if(LMBDown)
  {
    if(drawMode || eraseMode)
    {
      float valueToPlace = drawMode ? -1.0 : 0.0;
    
      int xEnd = mouseTileX + drawSize;
      int yEnd = mouseTileY + drawSize;
      
      if(xEnd > TILES_WIDE) xEnd = TILES_WIDE;
      if(yEnd > TILES_HIGH) yEnd = TILES_HIGH;

      for(int i=mouseTileX; i < xEnd; i++)
        for(int j=mouseTileY; j < yEnd; j++)
            setTile(i,j, valueToPlace);

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
void fillToDensity(float density)
{
  for(int i=0;i<TILES_WIDE;i++)
    for(int j=0;j<TILES_HIGH;j++)
      setTile(i,j,0);
  
  int tilesSet = 0;
  
  while( (1.0f * tilesSet / TOTAL_TILES) < density)
  {
    int i = random_between(0, TILES_WIDE - 1);
    int j = random_between(0, TILES_HIGH - 1);
    
    if(getTile(i,j) == 0)
    {
      setTile(i,j, -1);
      tilesSet++;
    }
  }

}

// --------------------------------------------------------
void tryLoadMap(const char *mapID)
{
  char filePath[80];
  sprintf(filePath, "maps/%d_%d_%s.map", TILES_WIDE, TILES_HIGH, mapID);
  FILE * fin = fopen(filePath, "rb");

  if(fin == NULL)
  {
    printf("Map %s not found.\n", filePath);
    return;
  }

  int tilesWide=0;
  int tilesHigh=0;
  fread(&tilesWide, sizeof(__uint16_t), 1, fin);  
  fread(&tilesHigh, sizeof(__uint16_t), 1, fin);  

  assert(tilesWide == TILES_WIDE);
  assert(tilesHigh == TILES_HIGH);
  
  for(int i=0;i<TILES_WIDE;i++)
    for(int j=0;j<TILES_HIGH;j++)
    {
      float val;
      fread(&val, sizeof(float), 1, fin);
    
      setTile(i,j, val);
    }
  
  fclose(fin);
  fin = NULL;
}

// ----------------------------------------------------------
void saveMap(const char *mapID)
{
  char filePath[80];
  sprintf(filePath, "maps/%d_%d_%s.map", TILES_WIDE, TILES_HIGH, mapID);

  FILE * fout = fopen(filePath, "wb");

  fwrite(&TILES_WIDE, sizeof(__uint16_t), 1, fout);  
  fwrite(&TILES_HIGH, sizeof(__uint16_t), 1, fout);  

  for(int i=0;i<TILES_WIDE;i++)
  {
    for(int j=0;j<TILES_HIGH;j++)
    {
      float val = getTile(i,j);
      if(val != -1.0) val = 0.0;
      
      fwrite(&val, sizeof(float), 1, fout);
    }
  }
  
  fclose(fout);
  fout = NULL;
  
  printf("Map saved to %s\n", filePath);
}

// ----------------------------------------------------------
// custom keyFunc with preset keys
void keyCallback(unsigned char ch, int x, int y) {
	switch(ch) {
    case '[':
    {
      drawSize /= 2;
      if(drawSize < 1) drawSize = 1;
      printf("Draw size changed to %d\n", drawSize);
      break;
    }
    case ']':
    {
      drawSize *= 2;
      printf("Draw size changed to %d\n", drawSize);
      break;
    }
    case '1':
    {
      tryLoadMap("1");
      break;
    }
    case '2':
    {
      tryLoadMap("2");
      break;
    }
    case '9':
    {
      saveMap("1");
      break;
    }
    case '0':
    {
      saveMap("2");
      break;
    }
		case 'd':
    case 'D':
    {
      drawMode = true;
      eraseMode = false;
      
      printf("Draw mode ON.\n");        
			break;
    }
		case 'e':
    case 'E':
    {
      drawMode = false;
      eraseMode = true;
      
      printf("Erase mode ON.\n");        
			break;
    }
		case 'r':
    case 'R':
    {
      drawMode = false;
      eraseMode = false;
      
      printf("Render mode ON.\n");        
			break;
    }
    case 'f':
    case 'F':
    {
      fillToDensity(0.01);
      break;
    }
    case 'g':
    case 'G':
    {
      fillToDensity(0.10);
      break;
    }
    case 'h':
    case 'H':
    {
      fillToDensity(0.25);
      break;
    }
    case 't':
    case 'T':
    {
      // --- TIME THE RENDERING FOR THE SCREEN ---
      typedef std::chrono::high_resolution_clock Time;
      typedef std::chrono::nanoseconds ns;
      typedef std::chrono::duration<float> fsec;
      auto t0 = Time::now();
    
      int samplesTaken=0;
      
      while(samplesTaken < SAMPLES_TO_TAKE)
      {
        mouseTileX = random_between(0, TILES_WIDE);
        mouseTileY = random_between(0, TILES_HIGH);
        
        if( getTile(mouseTileX, mouseTileY) != -1.0)
        {
          renderScene();
          samplesTaken++;
        }
      }
            
      auto t1 = Time::now();
      fsec duration = t1 - t0;
      //ns nanoseconds = std::chrono::duration_cast<ns>(duration);
    
      float avgRenderSec = 1.0f * duration.count() / samplesTaken;
      float secPerTile = avgRenderSec / TOTAL_TILES;

      printf("%d tiles in %f sec =\n\t(%f sec)\n", TOTAL_TILES, avgRenderSec, secPerTile);
      
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
  if(argc != 2)
  {
    printf("Usage:\nrad <tilesize>\n");
    exit(1);
  }
 
  srand(0); 
  TILE_SIZE = atoi(argv[1]);
  TILES_WIDE = SCREEN_WIDTH / TILE_SIZE;
  TILES_HIGH = SCREEN_HEIGHT / TILE_SIZE;
  TOTAL_TILES = TILES_WIDE * TILES_HIGH;
  
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
