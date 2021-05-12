/***********************************************
 * Mousemic: Simple utility to display mouse movements 
 *           as a crude oscilloscope
 *
 * Alfredo Ortega (c) 2021
 */

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <SDL/SDL.h>
#include <X11/Xlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

// Window parameters
#define WIDTH 1024
#define HEIGHT 680
#define DEPTH 32
#define ZOOM 100.0

int *buf; // input buffer
int buflen=32;

// Main thread: Output mouse distance as int to stdout
unsigned int eventcounter;
void * threadFunc(void * arg)
{
    Display *display;
    XEvent xevent;
    Window window;
    struct timespec ts,oldts;
    int x,y,oldx=0,oldy=0,distance;

    if( (display = XOpenDisplay(NULL)) == NULL )
        return NULL;


    window = DefaultRootWindow(display);
    XAllowEvents(display, AsyncBoth, CurrentTime);

    XGrabPointer(display, 
                 window,
                 1, 
                 PointerMotionMask | ButtonPressMask | ButtonReleaseMask , 
                 GrabModeAsync,
                 GrabModeAsync, 
                 None,
                 None,
                 CurrentTime);

    eventcounter=0;
    oldts.tv_nsec=0;
    while(1) {
        XNextEvent(display, &xevent);
        switch (xevent.type) {
            case MotionNotify:
		clock_gettime(CLOCK_REALTIME,&ts);
		x=xevent.xmotion.x_root;y=xevent.xmotion.y_root;
		long long int totalnsecs=ts.tv_nsec-oldts.tv_nsec;
		oldts=ts;
		if (totalnsecs<0) 
			break;
		distance=(x-oldx)+(y-oldy);
		buf[eventcounter % buflen]=distance;
		eventcounter+=1;
		oldx=x;oldy=y;
                break;
        }
    }
    return NULL;
}

void setpixel(SDL_Surface *screen, int x, int y, Uint32 color)
{
    Uint32 *pixmem32;
    pixmem32 = (Uint32*) screen->pixels  + (y*WIDTH)+x;
    *pixmem32 = color;
}

#define SGN(x) ((x)>0 ? 1 : ((x)==0 ? 0:(-1)))
#define ABS(x) ((x)>0 ? (x) : (-x))

/* Basic unantialiased Bresenham line algorithm */
static void bresenham_line(SDL_Surface *screen, Uint32 x1, Uint32 y1, Uint32 x2, Uint32 y2,
			   Uint32 color)
{
  int lg_delta, sh_delta, cycle, lg_step, sh_step;

  lg_delta = x2 - x1;
  sh_delta = y2 - y1;
  lg_step = SGN(lg_delta);
  lg_delta = ABS(lg_delta);
  sh_step = SGN(sh_delta);
  sh_delta = ABS(sh_delta);
  if (sh_delta < lg_delta) {
    cycle = lg_delta >> 1;
    while (x1 != x2) {
      setpixel(screen,x1, y1, color);
      cycle += sh_delta;
      if (cycle > lg_delta) {
	cycle -= lg_delta;
	y1 += sh_step;
      }
      x1 += lg_step;
    }
    setpixel(screen, x1, y1, color);
  }
  cycle = sh_delta >> 1;
  while (y1 != y2) {
    setpixel(screen,x1, y1, color);
    cycle += lg_delta;
    if (cycle > sh_delta) {
      cycle -= sh_delta;
      x1 += lg_step;
    }
    y1 += sh_step;
  }
 setpixel(screen, x1, y1, color);
}



void DrawScreen(SDL_Surface* screen, int bufsize)
{ 
    float y;
    float oldy=HEIGHT/2;
    SDL_Rect srcrect,dstrect;

usleep(3000);
// Draw data slow-oscilloscope
srcrect.x = 1;
srcrect.y = 0;
srcrect.w = WIDTH;
srcrect.h = HEIGHT;

dstrect.x = 0;
dstrect.y = 0;
dstrect.w = WIDTH;
dstrect.h = HEIGHT;


SDL_BlitSurface(screen,&srcrect,screen,&dstrect);
SDL_Rect rect2 = {WIDTH-1,0,2,HEIGHT};
SDL_FillRect( SDL_GetVideoSurface(), &rect2, 0 );

float cont=0;
for (int i=0;i<buflen;i++)  {
	cont+=buf[i];
	buf[i]=0;
	}
cont/=buflen;
y=(HEIGHT/2)+cont*ZOOM;
if (y>=(HEIGHT-1)) y=HEIGHT-1;
if (y<0) y=0;
bresenham_line(screen, WIDTH-1, HEIGHT-y-1 ,WIDTH-1, oldy,0xF0F000);
oldy=HEIGHT-y-1;
SDL_Flip(screen); 
}


int main(int argc, char* argv[])
{

buf= (int *) calloc(buflen,sizeof(int));

    SDL_Surface *screen;
    SDL_Event event;
  
    int keypress = 0;
  
    if (SDL_Init(SDL_INIT_VIDEO) < 0 ) return 1;
   
    if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_SWSURFACE|SDL_RESIZABLE)))
    {
        SDL_Quit();
        return 1;
    }

 // Mouse thread
    pthread_t threadId;
    pthread_create(&threadId, NULL, &threadFunc, NULL);

 // Lock screen
    if(SDL_MUSTLOCK(screen)) 
    {
        if(SDL_LockSurface(screen) < 0) return 1;
    }
 // main loop
    while(!keypress) 
    {
         DrawScreen(screen,1);
         while(SDL_PollEvent(&event)) 
         {      
              switch (event.type) 
              {
                  case SDL_QUIT:
	              keypress = 1;
	              break;
                  case SDL_KEYDOWN:
                       keypress = 1;
                       break;
              }
         }
    }

// unlock screen
    if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);  

    SDL_Quit();
  
    return 0;
}




