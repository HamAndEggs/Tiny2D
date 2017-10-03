/*
   Copyright (C) 2017, Richard e Collins.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef __FRAME_BUFFER_H__
#define __FRAME_BUFFER_H__

#include <stdint.h>
#include <linux/fb.h>

namespace FBIO{	// Using a namespace to try to prevent name clashes as my class name is kind of obvious. :)
///////////////////////////////////////////////////////////////////////////////////////////////////////////
class Font;
   
class FrameBuffer
{
public:
	static FrameBuffer* Open(bool pVerbose = false);

	~FrameBuffer();

	int GetWidth()const{return mWidth;}
	int GetHeight()const{return mHeight;}

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
	
private:
	FrameBuffer(int pFile,uint8_t* pFrameBuffer,struct fb_fix_screeninfo pFixInfo,struct fb_var_screeninfo pScreenInfo,bool pVerbose);

	/*
		Draws an arbitrary line.
		Using Bresenham's line algorithm
		https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
	*/
	void DrawLineBresenham(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

	const int mWidth,mHeight;
	const int mStride;// Num bytes between each line.
	const int mPixelSize;	// The byte count of each pixel. So to move in the x by one pixel.
	const int mFrameBufferFile;
	const int mFrameBufferSize;
	const bool mVerbose;
	const struct fb_var_screeninfo mVaribleScreenInfo;
	uint8_t* mFrameBuffer;
};

class Font
{
public:
	Font();
	~Font();

	int GetCharWidth()const{return 8;}
	int GetCharHeight()const{return 13;}

	void DrawChar(FrameBuffer* pDest,int pX,int pY,int pChar)const;
	void Print(FrameBuffer* pDest,int pX,int pY,const char* pText)const;
	void Printf(FrameBuffer* pDest,int pX,int pY,const char* pFmt,...)const;
	
	void SetPenColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);
private:
	struct
	{
		uint8_t r,g,b;
	}mPenColour;
};

// Very simple button rendering class. No logic.
class Button
{
public:
	Button(int pX,int pY,int pWidth,int pHeight,const char* pText);// Colours default to windows 3.1 type colours.
	~Button();

	void Render(FrameBuffer* pDest,bool pPressed);

	void SetText(const char* pText);
	void SetTextf(const char* pFmt,...);
	void SetColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);	// Makes mLightColour a bright version of the colour, mDarkColour a darkened version and sets the mFillColour to the colour passed in. Does not change text colour.
	void SetLightColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);
	void SetFillColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);
	void SetDarkColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);
	void SetTextColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);
	void SetTextPressedColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);
	
private:
	
	struct
	{
		int FromX,FromY,ToX,ToY;
	}mRect;

	struct
	{
		uint8_t r,g,b;
	}mLightColour,mFillColour,mDarkColour,mTextColour,mTextPressedColour;

	const char* mText;
	Font mTheFont;
};
   
///////////////////////////////////////////////////////////////////////////////////////////////////////////
};//namespace FBIO
	
#endif //__FRAME_BUFFER_H__
	  