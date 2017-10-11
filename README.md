# FrameBuffer
Simple light weight direct frame buffer rendering with a built in font. Just one header and source file. 

I wrote this as I wanted a simple and easy way to render stuff on the Raspberr Pi without trying to get SDL2 to work in the console. This is just a plug'n'play source file to add gfx to your console app. Ideal for building GUI's using the small displays you can buy for the Raspberry PI.

## Usage
Call the static member function Open to create the FrameBuffer object, will return NULL if it did not work. When finished delete the object you created. As simple as that.

## Adding to your project
As it's only one header and source file I did not bother creating make / build files. Just copy the files into your project and go. Don't get much more simple than that, which is the aim of the project. When you just want something on screen.

## Basic example
```c++
#include "framebuffer.h"
FBIO::FrameBuffer* FB = FBIO::FrameBuffer::Open(true);
if( FB )
{
FBIO::Font TheFont(3);

FB->ClearScreen(0,0,0);

FB->DrawCircle(100,100,40,255,0,255,true);

TheFont.Print(FB,10,10,"Hello world");

delete FB;
}
```

## API.
```c++
/*
	Creates and opens a FrameBuffer object.
	If the OS does not support the frame buffer driver or there is some other error,
	this function will return NULL.
	Set pVerbose to true to get debugging information as the object is created.
*/
static FrameBuffer* Open(bool pVerbose = false);

/*
	Returns the width of the frame buffer.
*/
int GetWidth()const;

/*
	Returns the height of the frame buffer.
*/
int GetHeight()const;

/*
	Writes a single pixel with the passed red, green and blue values. 0 -> 255, 0 being off 255 being full on.
*/
void WritePixel(int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

/*
	Clears the entrie screen.
*/
void ClearScreen(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

/* 
	Expects source to be 24bit, three 8 bit bytes in R G B order.
	IE pSourcePixels[0] is red, pSourcePixels[1] is green and pSourcePixels[2] is blue.
	Renders the image to pX,pY without scaling. Most basic blit.
*/
void BlitRGB24(const uint8_t* pSourcePixels,int pX,int pY,int pSourceWidth,int pSourceHeight);

/* 
	Expects source to be 24bit, three 8 bit bytes in R G B order.
	IE pSourcePixels[0] is red, pSourcePixels[1] is green and pSourcePixels[2] is blue.
	Renders the image to pX,pY from pSourceX,pSourceY in the source without scaling.
	pSourceStride is the byte size of one scan line in the source data.
	Allows sub rect render of the source image.
*/
void BlitRGB24(const uint8_t* pSourcePixels,int pX,int pY,int pWidth,int pHeight,int pSourceX,int pSourceY,int pSourceStride);

/*
	Draws a horizontal line.
*/
void DrawLineH(int pFromX,int pFromY,int pToX,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

/*
	Draws a vertical line.
*/
void DrawLineV(int pFromX,int pFromY,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

/*
	Draws an arbitrary line.
	Will take a short cut if the line is horizontal or vertical.
*/
void DrawLine(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

/*
	Draws a circle using the Midpoint algorithm.
	https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
*/
void DrawCircle(int pCenterX,int pCenterY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,bool pFilled = false);

/*
	Draws a rectangle with the passed in RGB values either filled or not.
*/
void DrawRectangle(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,bool pFilled = false);


/*
	Converts from HSV to RGB.
	Very handy for creating colour palettes.
	See:- (thanks David H)
		https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both

*/
void HSV2RGB(float H,float S, float V,uint8_t &rRed,uint8_t &rGreen, uint8_t &rBlue)const;
```
