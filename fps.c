#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"

#define screenWidth 800
#define screenHeight 600
#define mapWidth 20
#define mapHeight 20

void setWallColor (Color*, int);

int main (void) {
  typedef struct {
    int x;
    int y;
  } iVector2;

  int mode = 0;
  Vector2 position = { 1.5f, 4.5f };
  Vector2 direction = { 1.0f, 0.0f };
  double moveSpeed;
  double frontSpeedBoost = 2.0f;
  double rotSpeed;
  Vector2 plane = { 0.0f, -0.66f };
  int mouseX = 0, oldMouseX = 0;
  double time = 0.0f;
  double oldTime = 0.0f;
  double frameTime = 0.0f;

  iVector2 posInMap;
  double wallDist;
  iVector2 step;
  int hit = 0;
  int side = 0;
  Color wallColor;
  double cameraX = 0.0f;
  Vector2 rayDir = { 0.0f, 0.0f };
  Vector2 sideDist;
  Vector2 deltaDist = { 0.0f, 0.0f };
  int lineHeight = 0;

  Image maze_img = LoadImage("maze.png");
  Color *maze_col = LoadImageColors(maze_img);
  int map[mapHeight][mapWidth];

  // Switched x and y because otherwise the matrix would seem rotated in the code
  // TODO: Make possible for other people to replace the default map with others with different dimensions
  // TODO: Make game map follow the colors in the image that serves as its recipe
  for (int y = 0; y < mapHeight; ++y) {
      for (int x = 0; x < mapWidth; ++x) {
          if (maze_col[y*mapWidth+x].r == 0 &&
              maze_col[y*mapWidth+x].g == 0 &&
              maze_col[y*mapWidth+x].b == 0)
              map[y][x] = 0;
          if (maze_col[y*mapWidth+x].r == 255 &&
              maze_col[y*mapWidth+x].g == 255 &&
              maze_col[y*mapWidth+x].b == 0)
              map[y][x] = 1;
          if (maze_col[y*mapWidth+x].r == 82 &&
              maze_col[y*mapWidth+x].g == 85 &&
              maze_col[y*mapWidth+x].b == 148)
              map[y][x] = 2;
          if (maze_col[y*mapWidth+x].r == 255 &&
              maze_col[y*mapWidth+x].g == 0 &&
              maze_col[y*mapWidth+x].b == 0)
              map[y][x] = 3;
          if (maze_col[y*mapWidth+x].r == 0 &&
              maze_col[y*mapWidth+x].g == 255 &&
              maze_col[y*mapWidth+x].b == 0)
              map[y][x] = 4;
      }
  }

  // Create window
  InitWindow (screenWidth, screenHeight, "FPS");
  SetTargetFPS (60);

  DisableCursor();
  // Get cursor position before entering game loop in order to keep the camera
  // at its original position on game start
  mouseX = GetMouseX();

  // Main loop
  while (! WindowShouldClose ()) {
    // Set speeds based on how many seconds the last frame took to render
    oldTime = time;
    time = GetTime();
    frameTime = (time - oldTime) / 1000.0f;
    moveSpeed = frameTime * 650.0f;
    // Input handling
    /// Key to exit the game
    /// TODO: Hold instead of simply pressing to exit
    if (IsKeyDown(KEY_ESCAPE)) break;
    /// Movement control
    if (IsKeyDown(KEY_LEFT_SHIFT)) frontSpeedBoost = 3.5f;
    else frontSpeedBoost = 2.0f;
    if (IsKeyDown(KEY_W) &&
        map[(int)(position.y + direction.y * moveSpeed * frontSpeedBoost)][(int)(position.x + direction.x * moveSpeed * frontSpeedBoost)] > 1) {
          position.x += direction.x * moveSpeed * frontSpeedBoost;
          position.y += direction.y * moveSpeed * frontSpeedBoost;
    }
    if (IsKeyDown(KEY_S) &&
	map[(int)(position.y - direction.y * moveSpeed)][(int)(position.x - direction.x * moveSpeed)] > 1) {
          position.x -= direction.x * moveSpeed;
          position.y -= direction.y * moveSpeed;
    }
    if (IsKeyDown(KEY_A) &&
	map[(int)(position.y + (direction.x * sin (M_PI/2) + direction.y * cos (M_PI/2)) * moveSpeed)][(int)(position.x + (direction.x * cos (M_PI/2) - direction.y * sin (M_PI/2)) * moveSpeed)] > 1) {
          position.x += (direction.x * cos (M_PI/2) - direction.y * sin (M_PI/2)) * moveSpeed;
          position.y += (direction.x * sin (M_PI/2) + direction.y * cos (M_PI/2)) * moveSpeed;
    }
    if (IsKeyDown(KEY_D) &&
	map[(int)(position.y + (direction.x * sin (-M_PI/2) + direction.y * cos (-M_PI/2)) * moveSpeed)][(int)(position.x + (direction.x * cos (-M_PI/2) - direction.y * sin (-M_PI/2)) * moveSpeed)] > 1) {
          position.x += (direction.x * cos (-M_PI/2) - direction.y * sin (-M_PI/2)) * moveSpeed;
          position.y += (direction.x * sin (-M_PI/2) + direction.y * cos (-M_PI/2)) * moveSpeed;
    }
    /// Vision control
    double oldDirectionX = direction.x;
    double oldPlaneX = plane.x;
    oldMouseX = mouseX;
    mouseX = GetMouseX();
    rotSpeed = -(mouseX - oldMouseX) / 600.0f;
    direction.x = direction.x * cos (rotSpeed) - direction.y * sin (rotSpeed);
    direction.y = oldDirectionX * sin (rotSpeed) + direction.y * cos (rotSpeed);
    plane.x = plane.x * cos (rotSpeed) - plane.y * sin (rotSpeed);
    plane.y = oldPlaneX * sin (rotSpeed) + plane.y * cos (rotSpeed);

    BeginDrawing ();
    ClearBackground (DARKGRAY);
    DrawRectangle(0, 0, screenWidth, screenHeight * 0.5f, BLACK);

    // Rays and their calculations
    if (mode == 0) for (int x = 0; x < screenWidth; ++x) {
      cameraX = 2 * x / (double) screenWidth - 1;
      rayDir.x = direction.x + plane.x * cameraX;
      rayDir.y = direction.y + plane.y * cameraX;

      deltaDist.x = rayDir.x == 0 ? 1e30 : fabs(1 / rayDir.x);
      deltaDist.y = rayDir.y == 0 ? 1e30 : fabs(1 / rayDir.y);

      posInMap.x = (int) position.x;
      posInMap.y = (int) position.y;
	  if (posInMap.x == 15 && posInMap.y == 19) {
		  mode = 1;
		  moveSpeed = 0;
		  break;
	  } else if (posInMap.x == 10 && posInMap.y == 19) {
		  mode = 2;
		  moveSpeed = 0;
		  break;
	  }
	  printf("x: %d, y: %d\n", posInMap.x, posInMap.y);

      if (rayDir.x < 0) {
    	step.x = -1;
    	sideDist.x = (position.x - posInMap.x) * deltaDist.x;
      } else {
    	step.x = 1;
    	sideDist.x = (posInMap.x + 1 - position.x) * deltaDist.x;
      }
      if (rayDir.y < 0) {
    	step.y = -1;
    	sideDist.y = (position.y - posInMap.y) * deltaDist.y;
      } else {
    	step.y = 1;
    	sideDist.y = (posInMap.y + 1 - position.y) * deltaDist.y;
      }
      hit = 0;
      while (hit == 0) {
        if (sideDist.x < sideDist.y) {
    	  sideDist.x += deltaDist.x;
          posInMap.x += step.x;
    	  side = 0;
        } else {
    	  sideDist.y += deltaDist.y;
          posInMap.y += step.y;
    	  side = 1;
        }
	  if (map[posInMap.y][posInMap.x] != 2) hit = 1;
      }
      if (side == 0) wallDist = (sideDist.x - deltaDist.x);
      else wallDist = (sideDist.y - deltaDist.y);
      lineHeight = (int) (screenHeight / wallDist);

      int drawStart = -lineHeight / 2 + screenHeight / 2;
      if (drawStart < 0) drawStart = 0;
      if (map[posInMap.y][posInMap.x] == 0) wallColor = RED;
      if (map[posInMap.y][posInMap.x] == 1) wallColor = BLUE;
      if (map[posInMap.y][posInMap.x] == 3) wallColor = BLACK;
      if (map[posInMap.y][posInMap.x] == 4) wallColor = PURPLE;
      if (side == 1) setWallColor(&wallColor, wallDist);
      DrawRectangle (x, drawStart, 1, lineHeight, wallColor);

      // Debugging
      //printf ("%f %d %d %d %f %d\n", cameraX, side, posInMap.x, posInMap.y, wallDist, BLUE.b);
    } else if (mode == 1) {
		ClearBackground (RED);
		DrawText(TextFormat("Você achou que seria uma boa ideia se\njogar no abismo, e assim o fez."), 70, 80, 20, WHITE);
		DrawText(TextFormat("Agora você está no inferno e o Diabo\n está vindo te buscar. Sinto muito."), 330, 400, 20, WHITE);
		DrawText(TextFormat("FINAL RUIM"), 70, 530, 35, BLACK);
	} else {
		ClearBackground (GREEN);
		DrawText(TextFormat("Após atravessar a sala iluminada pela\n luz negra, você encontrou um enorme campo."), 310, 80, 20, WHITE);
		DrawText(TextFormat("O céu está sem estrelas e a grama brilha\n em seu verde neon. Você nunca esteve\n tão confuso, mas está feliz."), 70, 400, 20, WHITE);
		DrawText(TextFormat("FINAL BOM"), 480, 530, 35, BLACK);
	}

    EndDrawing ();
  }

  // Exit, finally
  CloseWindow ();

  return 0;
}

void setWallColor (Color* color, int distance) {
  color->r *= 0.7f;
  color->g *= 0.7f;
  color->b *= 0.7f;
}
