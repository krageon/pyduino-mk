/*
 * Copyright (c) 2015 Nelson Tran
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Arduino.h>
#include "mouse.h"
#include <Mouse.h>

Point SCREEN_DIMENSIONS;
Point MOUSE_POSITION;

void parseMouseCommand()
{
  while (!Serial.available());
  uint8_t command = Serial.read();
  
  switch (command)
  {
    case MOUSE_CLICK:
      humanClick(readMouseButton());
      break;
    
    case MOUSE_FAST_CLICK:
      Mouse.click(readMouseButton());
      break;
      
    case MOUSE_PRESS:
      Mouse.press(readMouseButton());
      break;
      
    case MOUSE_RELEASE:
      Mouse.release(readMouseButton());
      break;
      
    default:
      Point destination;
      readPoint(destination);
      
      switch (command)
      {
        case MOUSE_MOVE:
          besenhamMove(destination);
          break;
        
        case MOUSE_BEZIER:
          bezierMove(destination);
          break;
      }
  }
  
  Serial.write(COMMAND_COMPLETE);
}

void readPoint(Point &p)
{
  while (!Serial.available()); p.x = Serial.read();
  while (!Serial.available()); p.x += Serial.read() << 8;
  while (!Serial.available()); p.y = Serial.read();
  while (!Serial.available()); p.y += Serial.read() << 8;
}

void calibrateMouse()
{
  Serial.write(MOUSE_CALIBRATE);
  readPoint(MOUSE_POSITION);
}

void calibrateScreen()
{
  Serial.write(SCREEN_CALIBRATE);
  readPoint(SCREEN_DIMENSIONS);
}

int readMouseButton()
{
  while (!Serial.available());
  switch (Serial.read())
  {
    case LEFT_BUTTON:
      return MOUSE_LEFT;
      break;
    case MIDDLE_BUTTON:
      return MOUSE_MIDDLE;
      break;
    case RIGHT_BUTTON:
      return MOUSE_RIGHT;
      break;
  }
}

void humanClick(int mouseButton)
{
  Mouse.press(mouseButton);
  delay(random(50, 100));
  Mouse.release(mouseButton);
}

void bezierMove(Point destination)
{
  calibrateMouse();
  calibrateScreen();
  
  // STEP * NUM_STEP = 1.0
  const double STEP = 0.01;
  const int NUM_STEP = 100;
  
  // number of control points
  const int NUM_CONTROL_POINTS = 2;
  
  int x1 = MOUSE_POSITION.x;
  int y1 = MOUSE_POSITION.y;
  int x2 = constrain(destination.x, 0, SCREEN_DIMENSIONS.x - 1);
  int y2 = constrain(destination.y, 0, SCREEN_DIMENSIONS.y - 1);
  
  Point controlPoints[NUM_CONTROL_POINTS];
  
  for (int i = 0; i < NUM_CONTROL_POINTS; i++)
  {
    controlPoints[i].x = random(0, SCREEN_DIMENSIONS.x);
    controlPoints[i].y = random(0, SCREEN_DIMENSIONS.y);
  }
  
  for (int i = 0; i <= NUM_STEP; i++)
  {
    double x_weights[] = 
    {
      x1,
      controlPoints[0].x,
      controlPoints[1].x,
      x2
    };
    
    double y_weights[] = 
    {
      y1,
      controlPoints[0].y,
      controlPoints[1].y,
      y2
    };
    
    Point point = 
    {
      cubicBezier(STEP * i, x_weights),
      cubicBezier(STEP * i, y_weights)
    };
    
    besenhamMove(point);
  }
}

double cubicBezier(double t, double weights[3])
{
  double t2 = t * t;
  double t3 = t2 * t;
  double mt = 1 - t;
  double mt2 = mt * mt;
  double mt3 = mt2 * mt;
  
  return 1 * weights[0] * mt3 +
         3 * weights[1] * mt2 * t +
         3 * weights[2] * mt * t2 +
         1 * weights[3] * t3;
}

double quarticBezier(double t, double weights[4])
{
  double t2 = t * t;
  double t3 = t2 * t;
  double t4 = t3 * t;
  double mt = 1 - t;
  double mt2 = mt * mt;
  double mt3 = mt2 * mt;
  double mt4 = mt3 * mt;
  
  return 1 * weights[0] * mt4 +
         4 * weights[1] * mt3 * t +
         6 * weights[2] * mt2 * t2 +
         4 * weights[3] * mt * t3 +
         1 * weights[4] * t4;
}

int step(int p1, int p2, int step)
{
  int dp = p2 - p1;
  if (abs(dp) < step)
  {
    return dp;
  }

  return (p1 < p2) ? step : (step * -1);
}

void besenhamMove(Point destination)
{
  calibrateMouse();
  if (pointsEqual(destination, MOUSE_POSITION))
  {
    return;
  }

  destination.x = constrain(destination.x, 0, SCREEN_DIMENSIONS.x - 1);
  destination.y = constrain(destination.y, 0, SCREEN_DIMENSIONS.y - 1);

  do
  {
    calibrateScreen();
    calibrateMouse();

    int legalmove = 127;

    int dx = step(MOUSE_POSITION.x, destination.x, legalmove);
    int dy = step(MOUSE_POSITION.y, destination.y, legalmove);

    Mouse.move(dx,dy);
  } while (!pointsEqual(MOUSE_POSITION, destination));
}

bool pointsEqual(Point a, Point b)
{
  return (a.x == b.x && a.y == b.y) ? true : false;
}

void swap(int &a, int &b)
{
  int t = a;
  a = b;
  b = t;
}

