#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

SDL_Window* win;
SDL_Renderer* rend;

//Width and Height of window
const int WIDTH = 1024;
const int HEIGHT = 1024;
const int DEPTH = 1024;

class coordInfo
{
  public:
    double start_x;
    double end_x;
    double start_y;
    double end_y;
    double start_z;
    double end_z;

    double step_x;
    double step_y;
    double step_z;

    //Step sizes of real and imaginary axis
    void Steps()
    {
      step_x = (end_x - start_x) / (WIDTH - 1);
      step_y = (end_y - start_y) / (HEIGHT - 1);
      step_z = (end_z - start_z) / (DEPTH - 1);
    }

    //returns x coordinate given the pixel value on the x axis
    double xValue(int i)
    {
      return(start_x + i * step_x);
    }

    //returns y coordinate given the pixel value on the y axis
    double yValue(int j)
    {
      return(start_y + j * step_y);
    }

    double zValue(int k)
    {
      return(start_z + k * step_z);
    }

    int xPix(double x)
    {
      int i;
      i = (int)round((x - start_x) / step_x);
      return(i);
    }

    int yPix(double y)
    {
      int j;
      j = (int)round((y - start_y) / step_y);
      return(j);
    }

    int zPix(double z)
    {
      int k;
      k = (int)round((z - start_z) / step_z);
      return(k);
    }
};

class Corners //Corners have 1 coordinate
{
  public:
    int Dim = 3;
    double* Corner;

    Corners()
    {
      Corner = (double*)calloc(Dim, sizeof(double));
    }

    void MakeCorners(double x, double y, double z)
    {
      Corner[0] = x;
      Corner[1] = y;
      Corner[2] = z;
    }
};

class Edges //Edges have 2 points
{
  public:
    int NCorner = 2;
    Corners* Edge;

    Edges()
    {
      Edge = (Corners*)calloc(NCorner, sizeof(Corners));
    }
};

class Surfaces //Surfaces have atleast 3 Edges
{
  public:
    int NEdge;
    Edges* Surface;

    Surfaces(int N)
    {
      NEdge = N;
      Surface = (Edges*)calloc(NEdge, sizeof(Edges));
    }
};

class Translation
{
  public:
    int NPoints;
    Corners* Corner;

    void Rotate(double ThetaX, double ThetaY, double ThetaZ)
    {
      double R[3][3];
      double CX, CY, CZ, SX, SY, SZ;

      CX = cos(ThetaX);
      CY = cos(ThetaY);
      CZ = cos(ThetaZ);
      SX = sin(ThetaX);
      SY = sin(ThetaY);
      SZ = sin(ThetaZ);

      R[0][0] = CX * CY;
      R[0][1] = CX * SY * SZ - SX * CZ;
      R[0][2] = CX * SY * CZ + SX * SZ;

      R[1][0] = SX * CY;
      R[1][1] = SX * SY * SZ + CX * CZ;
      R[1][2] = SX * SY * CZ - CX * SZ;

      R[2][0] = - SY;
      R[2][1] = CY * SZ;
      R[2][2] = CY * CZ;

      Corners Temp;
      int i, j, k;

      for(i = 0; i < NPoints; i++)
      {
        Temp.MakeCorners(0, 0, 0);
        for(j = 0; j < 3; j++)
        {
          for(k = 0; k < 3; k++)
          {
            Temp.Corner[j]+= R[j][k] * Corner[i].Corner[k];
          }
        }

        for(j = 0; j < 3; j++)
        {
          Corner[i].Corner[j] = Temp.Corner[j];
        }
      }
    }

    void Shift(double X, double Y, double Z)
    {
      int i;
      for(i = 0; i < NPoints; i++)
      {
        Corner[i].Corner[0]+=X;
        Corner[i].Corner[1]+=Y;
        Corner[i].Corner[2]+=Z;
      }
    }

    void Enlarge(double EnlargementFactor)
    {
      int i;
      for(i = 0; i < NPoints; i++)
      {
        Corner[i].Corner[0]*=EnlargementFactor;
        Corner[i].Corner[1]*=EnlargementFactor;
        Corner[i].Corner[2]*=EnlargementFactor;
      }
    }

};

class Cubes: public Translation
{
  public:
    //Corners* Corner = (Corners*)calloc(8, sizeof(Corners)); //Decleared by Translation
    Edges* Edge = (Edges*)calloc(12, sizeof(Edges));
    Surfaces* Surface = (Surfaces*)calloc(6, sizeof(Surfaces));

    Corners Center;

    void SetCenter(Corners CenterTemp)
    {
      Center = CenterTemp;
    }

    Cubes(Corners Center, double size)
    {
      Corner = (Corners*)calloc(8, sizeof(Corners));


      int NSurface = 6; //6 faces per cube
      int NEdge = 4; //4 edges per face
      int NCorner = 2; //2 corners per edge
      int Dim = 3; //each point in 3 dimensions

      int NCubeSurface = 6;
      int NCubeEdge = 12;
      int NCubeCorner = 8;

      NPoints = NCubeCorner;


      //MakeEdges()
      // Face 0, 1, 2, 3, 4, 5
      int FaceToEdge[NCubeSurface][NEdge] = {{0, 3, 5, 1}, {6, 9, 11, 7},
                                {0, 8, 6, 2}, {4, 11, 10, 5},
                                {1, 10, 9, 2}, {3, 8, 7, 4}};
      //Edge 0, 1, 2, ..., 11
      int EdgeToCorner[NCubeEdge][NCorner] = {{0, 1}, {0, 2}, {0, 4},
                                {1, 3}, {3, 7}, {2, 3},
                                {4, 5}, {5, 7}, {1, 5},
                                {4, 6}, {2, 6}, {6, 7}};

      int i, j, k;
      double x, y, z;

      for(i = 0; i < 2; i++)
      {
        x = ((2 * i) - 1) * size / 2;
        for(j = 0; j < 2; j++)
        {
          y = ((2 * j) - 1) * size / 2;
          for(k = 0; k < 2; k++)
          {
            z = ((2 * k) - 1) * size / 2;
            Corners CornerTemp;
            Corner[i * 2 * 2 + j * 2 + k] = CornerTemp;
            Corner[i * 2 * 2 + j * 2 + k].MakeCorners(x, y, z);
          }
        }
      }

      for(i = 0; i < NCubeEdge; i++)
      {
        Edges EdgeTemp;
        Edge[i] = EdgeTemp;

        for(j = 0; j < NCorner; j++)
        {
          Edge[i].Edge[j] = Corner[EdgeToCorner[i][j]];
        }
      }

      for(i = 0; i < NCubeSurface; i++)
      {
        Surfaces SurfaceTemp(NEdge);
        Surface[i] = SurfaceTemp;

        for(j = 0; j < NEdge; j++)
        {
          Surface[i].Surface[j] = Edge[FaceToEdge[i][j]];
        }
      }
    }
};

////////////////////////////////////////////////////////////////////////////////

void init_SDL()
{
  // retutns zero on success else non-zero
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
      printf("error initializing SDL: %s\n", SDL_GetError());
  }
  win = SDL_CreateWindow("GAME", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);

  // triggers the program that controls
  // your graphics hardware and sets flags
  Uint32 render_flags = SDL_RENDERER_ACCELERATED;

  // creates a renderer to render our images
  rend = SDL_CreateRenderer(win, -1, render_flags);
}

void close_SDL()
{
  // destroy renderer
  SDL_DestroyRenderer(rend);

  // destroy window
  SDL_DestroyWindow(win);

  // close SDL
  SDL_Quit();
}


void DrawCirc_SDL(int x, int y, int z, int r)
{
  int i, j, k;
  int iView, jView;
  double ViewChange;
  for(i = -r; i < r + 1; i++)
  {
    for(j = -r; j < r + 1; j++)
    {
      for(k = -r; k < r + 1; k++)
      {
        if(i * i + j * j + k * k < r * r && k + z < DEPTH)
        {
          ViewChange = (1.0 * DEPTH) / (2 * DEPTH - k - z);
          SDL_SetRenderDrawColor(rend, 255, 255, 255, 0xFF); //Draw pixel
          SDL_RenderDrawPoint(rend, int((i + x - WIDTH / 2) * ViewChange + WIDTH / 2), int((j + y - HEIGHT / 2) * ViewChange) + HEIGHT / 2); //Draw pixel
        }
      }
    }
  }
}

void ClearRender_SDL()
{
  SDL_RenderClear(rend);
  SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
  SDL_RenderClear(rend);
}

void draw_Points(Corners* Corner, Corners Center, int NCorners, coordInfo Grid)
{
  int i, j, k, l;
  int CircRadius = 10;
  // Clear the entire screen to our selected color.

  for(l = 0; l < NCorners; l++)
  {
    i = Grid.xPix(Corner[l].Corner[0] + Center.Corner[0]);
    j = Grid.yPix(Corner[l].Corner[1] + Center.Corner[1]);
    k = Grid.zPix(Corner[l].Corner[2] + Center.Corner[2]);

    DrawCirc_SDL(i, j, k, CircRadius);
  }
}


//SDL event function
void events_SDL()
{
  double CubeSize = 1.0;
  Corners MyCubeCenter;
  MyCubeCenter.MakeCorners(0.0, 0.0, 0.0);

  Cubes MyCube(MyCubeCenter, CubeSize);

  int i, j, k, l;

  coordInfo Grid;
  Grid.start_x = -2;
  Grid.end_x = 2;
  Grid.start_y = -2;
  Grid.end_y = 2;
  Grid.start_z = -2;
  Grid.end_z = 2;
  Grid.Steps();


  // controls annimation loop
  int close = 0;

  // annimation loop
  Uint32 buttons;
  SDL_Event event;

  double phiX, phiY, phiZ, x, y, z, EnlargementFactor;
  phiX = 0;
  phiY = 0;
  phiZ = 0;

  x = 0;
  y = 0;
  z = 0;

  EnlargementFactor = 1.03;

  int FPS = 60;
  double speedPix = 1000 / FPS;
  double speedRot = 3.141 / FPS;


  while (!close) //While SDL window is open
  {
    // Events management
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
        case SDL_QUIT:
          // handling of close button
          close = 1;
          break;

        case SDL_KEYDOWN:
  				// keyboard API for key pressed
  				switch (event.key.keysym.scancode)
          {
    				case SDL_SCANCODE_W: //down y
              MyCube.Center.Corner[1]+=- speedPix * Grid.step_y;
              MyCube.SetCenter(MyCube.Center);
    					break;
    				case SDL_SCANCODE_A: //left x
              MyCube.Center.Corner[0]+=- speedPix * Grid.step_x;
              MyCube.SetCenter(MyCube.Center);
    					break;
    				case SDL_SCANCODE_S: //up y
              MyCube.Center.Corner[1]+= speedPix * Grid.step_y;
              MyCube.SetCenter(MyCube.Center);
    					break;
    				case SDL_SCANCODE_D: //right x
              MyCube.Center.Corner[0]+= speedPix * Grid.step_x;
              MyCube.SetCenter(MyCube.Center);
    					break;
            case SDL_SCANCODE_Q: //toward screen z
              MyCube.Center.Corner[2]+= speedPix * Grid.step_y;
              MyCube.SetCenter(MyCube.Center);
    					break;
            case SDL_SCANCODE_E: //away screen z
              MyCube.Center.Corner[2]+=- speedPix * Grid.step_y;
              MyCube.SetCenter(MyCube.Center);
    					break;
            case SDL_SCANCODE_X: //rotate in x
    					MyCube.Rotate(speedRot, 0, 0);
    					break;
            case SDL_SCANCODE_Y: //rotate in y
    					MyCube.Rotate(0, speedRot, 0);
    					break;
            case SDL_SCANCODE_Z: //rotate in z
    					MyCube.Rotate(0, 0, speedRot);
    					break;
            case SDL_SCANCODE_UP: //increase size
    					MyCube.Enlarge(EnlargementFactor);
    					break;
            case SDL_SCANCODE_DOWN: //decrease size
    					MyCube.Enlarge(1 / EnlargementFactor);
    					break;
            case SDL_SCANCODE_BACKSPACE: //reset
            {
              MyCubeCenter.MakeCorners(0.0, 0.0, 0.0);
              Cubes MyCubeNew(MyCubeCenter, CubeSize);
              MyCube = MyCubeNew;
              break;
            }
    				default:
    					break;
  				}
      }
    }

    ClearRender_SDL();
    draw_Points(MyCube.Corner, MyCube.Center, 8, Grid);


    SDL_RenderPresent(rend);

    SDL_Delay(1000 / FPS);

    //SDL_DestroyRenderer(rend);
  }
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  //Start SDL
  init_SDL();

  //SDL events
  events_SDL();

  //Close SDL
  close_SDL();

  return 0;
}
