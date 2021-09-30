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

#include <stdint.h>
#include <iostream>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdarg>
#include <string.h>

#include <linux/fb.h>
#include <linux/videodev2.h>
#include <linux/input.h>

#include <sys/ioctl.h>
#include <sys/mman.h>

#ifdef USE_X11_EMULATION
	#include <X11/Xlib.h>
	#include <X11/Xutil.h>
	#include <thread>
#endif

#include "Tiny2D.h"

namespace tiny2d{	// Using a namespace to try to prevent name clashes as my class name is kind of obvious. :)

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Colour space Implementation.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
void RGB2HSV(uint8_t pRed,uint8_t pGreen, uint8_t pBlue,float& rH,float& rS, float& rV)
{
    float min, max, delta;

	const float red = (float)pRed / 255.0f;
	const float green = (float)pGreen / 255.0f;
	const float blue = (float)pBlue / 255.0f;

    min = red < green ? red : green;
    min = min  < blue ? min  : blue;

    max = red > green ? red : green;
    max = max  > blue ? max  : blue;

    rV = max;                                // v
    delta = max - min;
    if (delta < 0.00001)
    {
        rS = 0;
        rH = 0; // undefined, maybe nan?
        return;
    }
    if( max > 0.0 )
	{ // NOTE: if Max is == 0, this divide would cause a crash
        rS = (delta / max);                  // s
    }
	else
	{
        // if max is 0, then r = g = b = 0              
        // s = 0, h is undefined
        rS = 0.0;
        rH = 0.0;                            // its now undefined
        return;
    }
    if( red >= max )                           // > is bogus, just keeps compiler happy
        rH = ( green - blue ) / delta;        // between yellow & magenta
    else
    if( green >= max )
        rH = 2.0 + ( blue - red ) / delta;  // between cyan & yellow
    else
        rH = 4.0 + ( red - green ) / delta;  // between magenta & cyan

    rH *= 60.0;                              // degrees

    if( rH < 0.0 )
        rH += 360.0;
}

void HSV2RGB(float H,float S, float V,uint8_t &rRed,uint8_t &rGreen, uint8_t &rBlue)
{
	float R;
	float G;
	float B;
	if(S <= 0.0f)
	{
		R = V;
		G = V;
		B = V;
	}
	else
	{
		float hh, p, q, t, ff;
		long i;

		hh = H;
		if(hh >= 360.0) hh = 0.0;
		hh /= 60.0;
		i = (long)hh;
		ff = hh - i;
		p = V * (1.0 - S);
		q = V * (1.0 - (S * ff));
		t = V * (1.0 - (S * (1.0 - ff)));

		switch(i)
		{
		case 0:
			R = V;
			G = t;
			B = p;
			break;
		
		case 1:
			R = q;
			G = V;
			B = p;
			break;
		
		case 2:
			R = p;
			G = V;
			B = t;
			break;

		
		case 3:
			R = p;
			G = q;
			B = V;
			break;
		
		case 4:
			R = t;
			G = p;
			B = V;
			break;

		case 5:
		default:
			R = V;
			G = p;
			B = q;
			break;
		}
	}

	rRed = (uint8_t)(R * 255.0f);
	rGreen = (uint8_t)(G * 255.0f);
	rBlue = (uint8_t)(B * 255.0f);
}

void TweenColoursHSV(uint8_t pFromRed,uint8_t pFromGreen, uint8_t pFromBlue,uint8_t pToRed,uint8_t pToGreen, uint8_t pToBlue,uint8_t rBlendTable[256][3])
{
	float fromH,fromS,fromV;
	float toH,toS,toV;

	RGB2HSV(pFromRed,pFromGreen,pFromBlue,fromH,fromS,fromV);
	RGB2HSV(pToRed,pToGreen,pToBlue,toH,toS,toV);

	float a = 0.0f;
	for( int n = 0 ; n < 256 ; n++, a += (1.0f/255.0f) )
	{
		const float H = ((1.0f-a)*fromH) + (a * toH);
		const float S = ((1.0f-a)*fromS) + (a * toS);
		const float V = ((1.0f-a)*fromV) + (a * toV);

		uint8_t r,g,b;

		HSV2RGB(H,S,V,r,g,b);

		rBlendTable[n][0] = r;
		rBlendTable[n][1] = g;
		rBlendTable[n][2] = b;
	}
}

void TweenColoursRGB(uint8_t pFromRed,uint8_t pFromGreen, uint8_t pFromBlue,uint8_t pToRed,uint8_t pToGreen, uint8_t pToBlue,uint8_t rBlendTable[256][3])
{
	for( uint32_t n = 0 ; n < 256 ; n++ )
	{
		rBlendTable[n][0] = ( (pFromRed   * (255 - n)) + (pToRed   * n) ) / 255;
		rBlendTable[n][1] = ( (pFromGreen * (255 - n)) + (pToGreen * n) ) / 255;
		rBlendTable[n][2] = ( (pFromBlue  * (255 - n)) + (pToBlue  * n) ) / 255;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// DrawBuffer Implementation.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
DrawBuffer::DrawBuffer(int pWidth, int pHeight,bool pHasAlpha,bool pPreMultipliedAlpha)
{
	Resize(pWidth,pHeight,pHasAlpha,pPreMultipliedAlpha);
}

DrawBuffer::DrawBuffer(const FrameBuffer* pFB)
{
	assert( pFB );
	Resize(pFB->GetWidth(),pFB->GetHeight(),false,false);
}

DrawBuffer::DrawBuffer() :
	mWidth(0),
	mHeight(0),
	mPixelSize(0),
	mStride(0),
	mHasAlpha(false),
	mPreMultipliedAlpha(false)
{
}

void DrawBuffer::Resize(int pWidth, int pHeight, size_t pPixelSize,bool pHasAlpha,bool pPreMultipliedAlpha)
{
	assert( pWidth > 0 );
	assert( pHeight > 0 );
	assert( pPixelSize > 2 );

	mWidth = pWidth;
	mHeight = pHeight;
	mPixelSize = pPixelSize;
	mStride = pWidth * pPixelSize;
	mLastlineOffset = mStride * (mHeight - 1);
	mHasAlpha = pHasAlpha;
	mPreMultipliedAlpha = pPreMultipliedAlpha;
	mPixels.resize(mHeight * mStride);
}

void DrawBuffer::BlendPixel(int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
	if( pX >= 0 && pX < mWidth && pY >= 0 && pY < mHeight )
	{
		// When optimized by compiler these const vars will
		// all move to generate the same code as if I made it all one line and unreadable!
		// Trust your optimizer. :)
		const size_t index = GetPixelIndex(pX,pY);
		uint8_t* dst = mPixels.data() + index;

		AssertPixelIsInBuffer(dst);

		const uint32_t sA = pAlpha;
		const uint32_t dA = 255 - sA;

		const uint32_t sR = (pRed * sA) / 255;
		const uint32_t sG = (pGreen * sA) / 255;
		const uint32_t sB = (pBlue * sA) / 255;

		const uint32_t dR = (dst[RED_PIXEL_INDEX] * dA) / 255;
		const uint32_t dG = (dst[GREEN_PIXEL_INDEX] * dA) / 255;
		const uint32_t dB = (dst[BLUE_PIXEL_INDEX] * dA) / 255;

		WRITE_RGB_TO_PIXEL(dst,( sR + dR ),( sG + dG ),( sB + dB ));

		// If dest has alpha, we need to pic the max value. Blending will just make everything vanish.
		if( mHasAlpha && dst[3] < sA )
		{
			dst[3] = sA;
		}
	}
}

void DrawBuffer::BlendPreAlphaPixel(int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
	if( pX >= 0 && pX < mWidth && pY >= 0 && pY < mHeight )
	{
		// When optimized by compiler these const vars will
		// all move to generate the same code as if I made it all one line and unreadable!
		// Trust your optimizer. :)
		const size_t index = GetPixelIndex(pX,pY);
		uint8_t* dst = mPixels.data() + index;

		AssertPixelIsInBuffer(dst);

		const uint32_t dA = pAlpha;	// Will already have been subtracted from 255. So just use value.

		// Already had Alpha applied, just grab values.
		const uint32_t sR = pRed;
		const uint32_t sG = pGreen;
		const uint32_t sB = pBlue;

		// Apply alpha to unpredictable colour values.
		const uint32_t dR = (dst[RED_PIXEL_INDEX] * dA) / 255;
		const uint32_t dG = (dst[GREEN_PIXEL_INDEX] * dA) / 255;
		const uint32_t dB = (dst[BLUE_PIXEL_INDEX] * dA) / 255;

		// Write new values
		WRITE_RGB_TO_PIXEL(dst,( sR + dR ),( sG + dG ),( sB + dB ));

		// For pre calculated alpha there is no good choice for combining the source and dest alpha. So we just ignore it.
	}
}

void DrawBuffer::Clear(uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
	uint8_t *dest = mPixels.data();
	for( int y = 0 ; y < mHeight ; y++ )
	{
		for( int x = 0 ; x < mWidth ; x++, dest += mPixelSize )
		{
			WRITE_RGB_TO_PIXEL(dest,pRed,pGreen,pBlue);
		}
	}

	if( mHasAlpha )
	{
		uint8_t *dest = mPixels.data();
		for( int y = 0 ; y < mHeight ; y++ )
		{
			for( int x = 0 ; x < mWidth ; x++, dest += mPixelSize )
			{
				dest[3] = pAlpha;
			}
		}
	}
}

void DrawBuffer::Clear(uint8_t pValue)
{
	memset(mPixels.data(),pValue,mPixels.size());
}

void DrawBuffer::BlitRGB(const uint8_t* pSourcePixels,int pX,int pY,int pSourceWidth,int pSourceHeight)
{
	int EndX = pSourceWidth + pX; 
	int EndY = pSourceHeight + pY;
	for( int y = pY ; y < mHeight && y < EndY ; y++, pSourcePixels += pSourceWidth * 3 )
	{
		const uint8_t* pixel = pSourcePixels;
		for( int x = pX ; x < mWidth && x < EndX ; x++, pixel += 3 )
		{
			WritePixel(x,y,pixel[0],pixel[1],pixel[2]);
		}
	}
}
	
void DrawBuffer::BlitRGB(const uint8_t* pSourcePixels,int pX,int pY,int pWidth,int pHeight,int pSourceX,int pSourceY,int pSourceStride)
{
	pWidth += pX; 
	pHeight += pY;
	pSourcePixels += (pSourceX*3) + (pSourceY * pSourceStride);
	for( int y = pY ; y < mHeight && y < pHeight ; y++, pSourcePixels += pSourceStride )
	{
		const uint8_t* pixel = pSourcePixels;
		for( int x = pX ; x < mWidth && x < pWidth ; x++, pixel += 3 )
		{
			WritePixel(x,y,pixel[0],pixel[1],pixel[2]);
		}
	}
}

void DrawBuffer::BlitRGBA(const uint8_t* pSourcePixels,int pX,int pY,int pSourceWidth,int pSourceHeight,bool pPreMultipliedAlpha)
{
	int EndX = pSourceWidth + pX; 
	int EndY = pSourceHeight + pY;
	if( pPreMultipliedAlpha )
	{
		for( int y = pY ; y < mHeight && y < EndY ; y++, pSourcePixels += pSourceWidth * 4 )
		{
			const uint8_t* pixel = pSourcePixels;
			for( int x = pX ; x < mWidth && x < EndX ; x++, pixel += 4 )
			{
				BlendPreAlphaPixel(x,y,pixel);
			}
		}
	}
	else
	{
		for( int y = pY ; y < mHeight && y < EndY ; y++, pSourcePixels += pSourceWidth * 4 )
		{
			const uint8_t* pixel = pSourcePixels;
			for( int x = pX ; x < mWidth && x < EndX ; x++, pixel += 4 )
			{
				BlendPixel(x,y,pixel);
			}
		}
	}
}

void DrawBuffer::BlitRGBA(const uint8_t* pSourcePixels,int pX,int pY,int pWidth,int pHeight,int pSourceX,int pSourceY,int pSourceStride,bool pPreMultipliedAlpha)
{
	pWidth += pX; 
	pHeight += pY;
	pSourcePixels += (pSourceX*4) + (pSourceY * pSourceStride);
	if( pPreMultipliedAlpha )
	{
		for( int y = pY ; y < mHeight && y < pHeight ; y++, pSourcePixels += pSourceStride )
		{
			const uint8_t* pixel = pSourcePixels;
			for( int x = pX ; x < mWidth && x < pWidth ; x++, pixel += 4 )
			{
				BlendPreAlphaPixel(x,y,pixel);
			}
		}
	}
	else
	{
		for( int y = pY ; y < mHeight && y < pHeight ; y++, pSourcePixels += pSourceStride )
		{
			const uint8_t* pixel = pSourcePixels;
			for( int x = pX ; x < mWidth && x < pWidth ; x++, pixel += 4 )
			{
				BlendPixel(x,y,pixel);
			}
		}
	}
}

void DrawBuffer::Blit(const DrawBuffer& pImage,int pX,int pY)
{
	// This is ripe for a big win using memcpy. But for now, just make it work!
	const int width = pX + pImage.mWidth; 
	const int height = pY + pImage.mHeight;
	
	const uint8_t* sourcePixels = pImage.mPixels.data();

	if( pImage.mHasAlpha )
	{
		for( int y = pY ; y < mHeight && y < height ; y++, sourcePixels += pImage.mStride )
		{
			const uint8_t* pixel = sourcePixels;
			for( int x = pX ; x < mWidth && x < width ; x++, pixel += pImage.mPixelSize )
			{
				WritePixel(x,y,pixel[RED_PIXEL_INDEX],pixel[GREEN_PIXEL_INDEX],pixel[BLUE_PIXEL_INDEX],pixel[ALPHA_PIXEL_INDEX]);
			}
		}
	}
	else
	{
		for( int y = pY ; y < mHeight && y < height ; y++, sourcePixels += pImage.mStride )
		{
			const uint8_t* pixel = sourcePixels;
			for( int x = pX ; x < mWidth && x < width ; x++, pixel += pImage.mPixelSize )
			{
				WritePixel(x,y,pixel[RED_PIXEL_INDEX],pixel[GREEN_PIXEL_INDEX],pixel[BLUE_PIXEL_INDEX]);
			}
		}
	}
}

void DrawBuffer::Blend(const DrawBuffer& pImage,int pX,int pY)
{
	// This is ripe for a big win using memcpy. But for now, just make it work!
	const int width = pX + pImage.mWidth; 
	const int height = pY + pImage.mHeight;
	
	const uint8_t* sourcePixels = pImage.mPixels.data();

	if( pImage.mPreMultipliedAlpha )
	{
		for( int y = pY ; y < mHeight && y < height ; y++, sourcePixels += pImage.mStride )
		{
			const uint8_t* pixel = sourcePixels;
			for( int x = pX ; x < mWidth && x < width ; x++, pixel += pImage.mPixelSize )
			{
				BlendPreAlphaPixel(x,y,pixel[RED_PIXEL_INDEX],pixel[GREEN_PIXEL_INDEX],pixel[BLUE_PIXEL_INDEX],pixel[ALPHA_PIXEL_INDEX]);
			}
		}
	}
	else if( pImage.mHasAlpha )
	{
		for( int y = pY ; y < mHeight && y < height ; y++, sourcePixels += pImage.mStride )
		{
			const uint8_t* pixel = sourcePixels;
			for( int x = pX ; x < mWidth && x < width ; x++, pixel += pImage.mPixelSize )
			{
				BlendPixel(x,y,pixel[RED_PIXEL_INDEX],pixel[GREEN_PIXEL_INDEX],pixel[BLUE_PIXEL_INDEX],pixel[ALPHA_PIXEL_INDEX]);
			}
		}
	}
	else
	{
		Blit(pImage,pX,pY);
	}
}


void DrawBuffer::DrawLineH(int pFromX,int pFromY,int pToX,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
	if( pFromY < 0 || pFromY >= mHeight )
		return;

	pFromX = std::max(0,std::min(mWidth,pFromX));
	pToX = std::max(0,std::min(mWidth,pToX));

	if( pFromX == pToX )
		return;
	
	if( pFromX > pToX )
		std::swap(pFromX,pToX);

	uint8_t *dest = mPixels.data() + (pFromX * mPixelSize) + (pFromY * mStride);
	for( int x = pFromX ; x <= pToX && x < mWidth ; x++, dest += mPixelSize )
	{
		WRITE_RGB_TO_PIXEL(dest,pRed,pGreen,pBlue);
		if( mHasAlpha )
		{
			dest[3] = pAlpha;
		}
	}

}

void DrawBuffer::DrawLineV(int pFromX,int pFromY,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
	if( pFromX < 0 || pFromX >= mWidth )
		return;

	pFromY = std::max(0,std::min(mHeight,pFromY));
	pToY = std::max(0,std::min(mHeight,pToY));

	if( pFromY == pToY )
		return;

	if( pFromY > pToY )
		std::swap(pFromY,pToY);


	uint8_t *dest = mPixels.data() + (pFromX * mPixelSize) + (pFromY*mStride);
	for( int y = pFromY ; y <= pToY && y < mHeight ; y++, dest += mStride )
	{
		WRITE_RGB_TO_PIXEL(dest,pRed,pGreen,pBlue);
		if( mHasAlpha )
		{
			dest[3] = pAlpha;
		}
	}

}

void DrawBuffer::DrawLine(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	if( pFromX == pToX )
		DrawLineV(pFromX,pFromY,pToY,pRed,pGreen,pBlue);
	else if( pFromY == pToY )
		DrawLineH(pFromX,pFromY,pToX,pRed,pGreen,pBlue);
	else
		DrawLineBresenham(pFromX,pFromY,pToX,pToY,pRed,pGreen,pBlue);
}

void DrawBuffer::DrawCircle(int pCenterX,int pCenterY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
    int x = pRadius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (pRadius << 1);

    while (x >= y)
    {
		WritePixel(pCenterX + x, pCenterY + y,pRed,pGreen,pBlue,pAlpha);
		WritePixel(pCenterX + y, pCenterY + x,pRed,pGreen,pBlue,pAlpha);
		WritePixel(pCenterX - y, pCenterY + x,pRed,pGreen,pBlue,pAlpha);
		WritePixel(pCenterX - x, pCenterY + y,pRed,pGreen,pBlue,pAlpha);
		WritePixel(pCenterX - x, pCenterY - y,pRed,pGreen,pBlue,pAlpha);
		WritePixel(pCenterX - y, pCenterY - x,pRed,pGreen,pBlue,pAlpha);
		WritePixel(pCenterX + y, pCenterY - x,pRed,pGreen,pBlue,pAlpha);
		WritePixel(pCenterX + x, pCenterY - y,pRed,pGreen,pBlue,pAlpha);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0)
        {
            x--;
            dx += 2;
            err += (-pRadius << 1) + dx;
        }
    }
}

void DrawBuffer::FillCircle(int pCenterX,int pCenterY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
    int x = pRadius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (pRadius << 1);

    while (x >= y)
    {
		DrawLineH(pCenterX - x, pCenterY + y,pCenterX + x,pRed,pGreen,pBlue,pAlpha);
		DrawLineH(pCenterX - x, pCenterY - y,pCenterX + x,pRed,pGreen,pBlue,pAlpha);
		DrawLineH(pCenterX - y, pCenterY + x,pCenterX + y,pRed,pGreen,pBlue,pAlpha);
		DrawLineH(pCenterX - y, pCenterY - x,pCenterX + y,pRed,pGreen,pBlue,pAlpha);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0)
        {
            x--;
            dx += 2;
            err += (-pRadius << 1) + dx;
        }
    }
}


void DrawBuffer::DrawRectangle(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
	DrawLineH(pFromX,pFromY,pToX,pRed,pGreen,pBlue,pAlpha);
	DrawLineH(pFromX,pToY,pToX,pRed,pGreen,pBlue,pAlpha);

	DrawLineV(pFromX,pFromY,pToY,pRed,pGreen,pBlue);
	DrawLineV(pToX,pFromY,pToY,pRed,pGreen,pBlue,pAlpha);
}

void DrawBuffer::FillRectangle(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
	pFromY = std::max(0,std::min(mHeight,pFromY));
	pToY = std::max(0,std::min(mHeight,pToY));

	if( pFromY == pToY )
		return;

	if( pFromY > pToY )
		std::swap(pFromY,pToY);

	pFromX = std::max(0,std::min(mWidth,pFromX));
	pToX = std::max(0,std::min(mWidth,pToX));

	if( pFromX == pToX )
		return;
	
	if( pFromX > pToX )
		std::swap(pFromX,pToX);

	for( int y = pFromY ; y <= pToY ; y++ )
	{
		DrawLineH(pFromX,y,pToX,pRed,pGreen,pBlue,pAlpha);
	}	
}


void DrawBuffer::DrawRoundedRectangle(int pFromX,int pFromY,int pToX,int pToY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
	if( pRadius < 1 )
	{
		DrawRectangle(pFromX,pFromY,pToX,pToY,pRed,pGreen,pBlue,pAlpha);
		return;
	}

	if( pFromY == pToY )
		return;

	if( pFromY > pToY )
		std::swap(pFromY,pToY);

	if( pFromX == pToX )
		return;
	
	if( pFromX > pToX )
		std::swap(pFromX,pToX);

	if( pRadius > pToX - pFromX && pRadius > pToY - pFromY )
	{
		pRadius = (pToX - pFromX) / 2;
		DrawCircle( (pFromX + pToX) / 2 , (pFromY + pToY) / 2 ,pRadius,pRed,pGreen,pBlue,pAlpha);
		return;
	}
	else if( pRadius*2 > pToX - pFromX )
	{
		pRadius = (pToX - pFromX) / 2;
	}
	else if( pRadius*2 > pToY - pFromY )
	{
		pRadius = (pToY - pFromY) / 2;
	}

    int x = pRadius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (pRadius << 1);

	// Values I need so that the quadrants are positioned in the corners of the rectangle.
	const int left = pFromX + pRadius; 
	const int right = pToX - pRadius;
	const int top = pFromY + pRadius;
	const int bottom = pToY - pRadius;

    while (x >= y)
    {
		WritePixel(left - x, top - y,pRed,pGreen,pBlue,pAlpha);
		WritePixel(left - y, top - x,pRed,pGreen,pBlue,pAlpha);
		WritePixel(right + y, top - x,pRed,pGreen,pBlue,pAlpha);
		WritePixel(right + x, top - y,pRed,pGreen,pBlue,pAlpha);

		WritePixel(right + x, bottom + y,pRed,pGreen,pBlue,pAlpha);
		WritePixel(right + y, bottom + x,pRed,pGreen,pBlue,pAlpha);
		WritePixel(left - y, bottom + x,pRed,pGreen,pBlue,pAlpha);
		WritePixel(left - x, bottom + y,pRed,pGreen,pBlue,pAlpha);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0)
        {
            x--;
            dx += 2;
            err += (-pRadius << 1) + dx;
        }
    }

	DrawLineH(left,pFromY,right,pRed,pGreen,pBlue,pAlpha);
	DrawLineH(left,pToY,right,pRed,pGreen,pBlue,pAlpha);

	DrawLineV(pFromX,top,bottom,pRed,pGreen,pBlue,pAlpha);
	DrawLineV(pToX,top,bottom,pRed,pGreen,pBlue,pAlpha);
}

void DrawBuffer::FillRoundedRectangle(int pFromX,int pFromY,int pToX,int pToY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha)
{
	if( pRadius < 1 )
	{
		FillRectangle(pFromX,pFromY,pToX,pToY,pRed,pGreen,pBlue,pAlpha);
		return;
	}

	if( pFromY == pToY )
		return;

	if( pFromY > pToY )
		std::swap(pFromY,pToY);

	if( pFromX == pToX )
		return;
	
	if( pFromX > pToX )
		std::swap(pFromX,pToX);

	if( pRadius > pToX - pFromX && pRadius > pToY - pFromY )
	{
		pRadius = (pToX - pFromX) / 2;
		FillCircle( (pFromX + pToX) / 2 , (pFromY + pToY) / 2 ,pRadius,pRed,pGreen,pBlue,pAlpha);
		return;
	}
	else if( pRadius*2 > pToX - pFromX )
	{
		pRadius = (pToX - pFromX) / 2;
	}
	else if( pRadius*2 > pToY - pFromY )
	{
		pRadius = (pToY - pFromY) / 2;
	}

    int x = pRadius-1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (pRadius << 1);

	// Values I need so that the quadrants are positioned in the corners of the rectangle.
	const int left = pFromX + pRadius; 
	const int right = pToX - pRadius;
	const int top = pFromY + pRadius;
	const int bottom = pToY - pRadius;

    while (x >= y)
    {
		DrawLineH(left - x, top - y,right + x,pRed,pGreen,pBlue,pAlpha);
		DrawLineH(left - y, top - x,right + y,pRed,pGreen,pBlue,pAlpha);			

		DrawLineH(left - x, bottom + y,right + x,pRed,pGreen,pBlue,pAlpha);
		DrawLineH(left - y, bottom + x,right + y,pRed,pGreen,pBlue,pAlpha);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy += 2;
        }
        if (err > 0)
        {
            x--;
            dx += 2;
            err += (-pRadius << 1) + dx;
        }
    }

	FillRectangle(pFromX,pFromY+pRadius,pToX,pToY-pRadius,pRed,pGreen,pBlue,pAlpha);
}

void DrawBuffer::FillCheckerBoard(int pX,int pY,int pXCount,int pYCount,int pXSize,int pYSize,const uint8_t pRGBA[2][4])
{
	const int width = pXSize-1;
	const int height = pYSize-1;

	for( int y = 0 ; y < pYCount ; y++, pY += pYSize )
	{
		int px = pX;
		for( int x = 0 ; x < pXCount ; x++, px += pXSize )
		{
			if( (x&1) == (y&1) )
				FillRectangle(px,pY,px+width,pY+height,pRGBA[0][0],pRGBA[0][1],pRGBA[0][2],pRGBA[0][3]);
			else
				FillRectangle(px,pY,px+width,pY+height,pRGBA[1][0],pRGBA[1][1],pRGBA[1][2],pRGBA[1][3]);
		}
	}
}

void DrawBuffer::FillCheckerBoard(int pX,int pY,int pXCount,int pYCount,int pXSize,int pYSize,uint8_t pA,uint8_t pB)
{
	const uint8_t RGBA[2][4] = {{pA,pA,pA,255},{pB,pB,pB,255}};
	FillCheckerBoard(pX, pY, pXCount, pYCount, pXSize, pYSize,RGBA);
}

void DrawBuffer::FillCheckerBoard(int pXSize,int pYSize,const uint8_t pRGBA[2][4])
{
	FillCheckerBoard(0,0,(mWidth+pXSize-1) / pXSize,(mHeight+pYSize+1) / pYSize,pXSize,pYSize,pRGBA);
}

void DrawBuffer::FillCheckerBoard(int pXSize,int pYSize,uint8_t pA,uint8_t pB)
{
	const uint8_t RGBA[2][4] = {{pA,pA,pA,255},{pB,pB,pB,255}};
	FillCheckerBoard(pXSize, pYSize,RGBA);
}

void DrawBuffer::DrawGradient(int pFromX,int pFromY,int pToX,int pToY,uint8_t pFormRed,uint8_t pFormGreen,uint8_t pFormBlue,uint8_t pToRed,uint8_t pToGreen,uint8_t pToBlue)
{
	// Do some early outs.
	if( pFromY == pToY || pFromX == pToX )
		return;

	// Make sure X is in the right direction.
	if( pFromX > pToX )
	{
		std::swap(pFromX,pToX);
	}

	float a,s;

	// See if we need to flip the direction of the gradient if they are drawing up instead of down.
	if( pFromY > pToY )
	{
		std::swap(pFromY,pToY);
		a = 1.0f;
		s = -1.0f / (float)(pToY - pFromY);
	}
	else
	{
		a = 0.0f;
		s = 1.0f / (float)(pToY - pFromY);
	}

	const float fR = (float)pFormRed / 255.0f;
	const float fG = (float)pFormGreen / 255.0f;
	const float fB = (float)pFormBlue / 255.0f;

	const float tR = (float)pToRed / 255.0f;
	const float tG = (float)pToGreen / 255.0f;
	const float tB = (float)pToBlue / 255.0f;

	for( int y = pFromY ; y <= pToY ; y++, a += s )
	{
		const float invA = 1.0f - a;
		const uint8_t r = (uint8_t)( ((fR * invA) + (tR * a) ) * 255.0f);
		const uint8_t g = (uint8_t)( ((fG * invA) + (tG * a) ) * 255.0f);
		const uint8_t b = (uint8_t)( ((fB * invA) + (tB * a) ) * 255.0f);

		DrawLineH(pFromX,y,pToX,r,g,b);
	}
}

void DrawBuffer::ScrollBuffer(int pXDirection,int pYDirection,int8_t pRedFill,uint8_t pGreenFill,uint8_t pBlueFill,uint8_t pAlphaFill)
{
	const uint8_t* src = mPixels.data() + (mStride * -pYDirection);	
	uint8_t* dst = mPixels.data();

	const int numLines = mHeight - std::abs(pYDirection);
	const int numBytes = (mWidth - (std::abs(pXDirection))) * mPixelSize;

	size_t stride = mStride;
	if( pYDirection >= 0 )
	{
		stride = -stride;
		dst += mLastlineOffset;
		src += mLastlineOffset;
	}

	if( pXDirection < 0 )
	{
		src += mPixelSize * -pXDirection;
	}
	else
	{
		dst += mPixelSize * pXDirection;
	}

	// Now do the work.
	for( int y = 0 ; y < numLines ; y++, src += stride, dst += stride )
	{
		AssertPixelIsInBuffer(src);
		AssertPixelIsInBuffer(dst);

		memcpy(dst,src,numBytes);
	}

	// Now fill in the area that has just been exposed.

	if( pYDirection > 0 )
	{
		FillRectangle(0,0,mWidth,pYDirection,pRedFill,pGreenFill,pBlueFill,pAlphaFill);
	}
	else if( pYDirection < 0 )
	{
		FillRectangle(0,mHeight + pYDirection,mWidth,mHeight,pRedFill,pGreenFill,pBlueFill,pAlphaFill);
	}

	if( pXDirection > 0 )
	{
		FillRectangle(0,0,pXDirection,mHeight,pRedFill,pGreenFill,pGreenFill,pAlphaFill);
	}
	else if( pXDirection < 0 )
	{
		FillRectangle(mWidth+pXDirection,0,mWidth,mHeight,		pRedFill,pGreenFill,pBlueFill,pAlphaFill);
	}

}

void DrawBuffer::PreMultiplyAlpha()
{
	assert( mPreMultipliedAlpha == false ); // Can't do this more than once!
	assert( mHasAlpha ); // Has to have alpha data!
	assert( (mPixels.size()&3) == 0 ); // Must be multiples for four bytes!

	uint8_t* pixel = mPixels.data();
	size_t pixelCount = mPixels.size()/4;
	mPreMultipliedAlpha = true;
	while( pixelCount-- )
	{
		AssertPixelIsInBuffer(pixel);

		const uint32_t A = pixel[3];

		pixel[0] = (uint8_t)((pixel[0] * A)/255);
		pixel[1] = (uint8_t)((pixel[1] * A)/255);
		pixel[2] = (uint8_t)((pixel[2] * A)/255);
		pixel[3] = 255 - A;

		pixel += 4;
	}
}


void DrawBuffer::DrawLineBresenham(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	// Deals with all 8 quadrants.
	int deltax = pToX - pFromX;	// The difference between the x's
	int deltay = pToY - pFromY;	// The difference between the y's	
	int x = pFromX;				// Start x off at the first pixel
	int y = pFromY;				// Start y off at the first pixel

	int xinc1 = 1;
	int xinc2 = 1;
	int yinc1 = 1;
	int yinc2 = 1;

	if( deltax < 0 )
	{// The x-values are decreasing
		deltax = -deltax;
		xinc1 = -1;
		xinc2 = -1;
	}

	if( deltay < 0 )
	{// The y-values are decreasing
		deltay = -deltay;
		yinc1 = -1;
		yinc2 = -1;
	}

	int den,num,numadd,numpixels,curpixel;
	if( deltax >= deltay )
	{// There is at least one x-value for every y-value
		xinc1 = 0;	// Don't change the x when numerator >= denominator
		yinc2 = 0;	// Don't change the y for every iteration
		den = deltax;
		num = deltax>>1;
		numadd = deltay;
		numpixels = deltax;// There are more x-values than y-values
	}
	else
	{ // There is at least one y-value for every x-value
		xinc2 = 0;	// Don't change the x for every iteration
		yinc1 = 0;	// Don't change the y when numerator >= denominator
		den = deltay;
		num = deltay>>1;
		numadd = deltax;
		numpixels = deltay;// There are more y-values than x-values
	}

	for( curpixel = 0 ; curpixel <= numpixels ; curpixel++ )
	{
		if( x > -1 && x < mWidth && y > -1 && y < mHeight )
		{
			WritePixel(x,y,pRed,pGreen,pBlue);
		}
		num += numadd;	// Increase the numerator by the top of the fraction
		if (num >= den)	// Check if numerator >= denominator
		{
			num -= den;	// Calculate the new numerator value
			x += xinc1;	// Change the x as appropriate
			y += yinc1;	// Change the y as appropriate
		}
		x += xinc2;		// Change the x as appropriate
		y += yinc2;		// Change the y as appropriate
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// X11 frame buffer emulation hidden definition.
// Implementation is at the bottom of the source file.
// This code is intended to allow development on a full desktop system for applications that
// will eventually be deployed on a minimal linux system without all the X11 + fancy UI rendering bloat.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef USE_X11_EMULATION
/**
 * @brief Emulation layer for X11
 * 
 */
struct X11FrameBufferEmulation
{
	Display *mDisplay;
	Window mWindow;
	XImage* mDisplayBufferImage;
	Atom mDeleteMessage;

	bool mWindowReady;
	uint8_t* mDisplayBuffer;
	struct fb_fix_screeninfo mFixInfo;
	struct fb_var_screeninfo mVarInfo;

	X11FrameBufferEmulation();
	~X11FrameBufferEmulation();

	bool Open(bool pVerbose);

	/**
	 * @brief Processes the X11 events then exits when all are done.
	 * 
	 * @param pEventHandler 
	 */
	void ProcessSystemEvents(FrameBuffer::SystemEventHandler pEventHandler);

	/**
	 * @brief Draws the frame buffer to the X11 window.
	 * 
	 */
	void RedrawWindow();
};
#endif //#ifdef USE_X11_EMULATION
	
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// FrameBuffer Implementation.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
FrameBuffer* FrameBuffer::Open(int pCreationFlags)
{
	FrameBuffer* newFrameBuffer = NULL;

#ifdef USE_X11_EMULATION
	X11FrameBufferEmulation* newX11 = new X11FrameBufferEmulation();
	if( newX11->Open(pCreationFlags) )
	{
		newFrameBuffer = new FrameBuffer(0,newX11->mDisplayBuffer,newX11->mFixInfo,newX11->mVarInfo,pCreationFlags);
		newFrameBuffer->mX11 = newX11;
	}
	else
	{
		delete newX11;
	}
#else
	const bool verbose = (pCreationFlags&VERBOSE_MESSAGES) != 0;
	// Open the file for reading and writing
	int File = open("/dev/fb0", O_RDWR);
	if(File)
	{
		if(verbose)
		{
			std::cout << "The framebuffer device was opened successfully.\n";
		}

		// Get fixed screen information
		struct fb_fix_screeninfo finfo;
		if( ioctl(File, FBIOGET_FSCREENINFO, &finfo) )
		{
			if(verbose)
			{
				std::cerr << "Error reading fixed information.\n";
			}
		}
		else
		{
			// Get variable screen information
			struct fb_var_screeninfo vinfo;
			if( ioctl(File, FBIOGET_VSCREENINFO, &vinfo) )
			{
				if(verbose)
				{
					std::cerr << "Error reading variable information.\n";
				}
			}
			else
			{
				if(verbose)
				{
					std::cout << "Display size: " << vinfo.xres << "x" << vinfo.yres << ", " << vinfo.bits_per_pixel << "bpp\n";
					std::cout << "Frame buffer info: Size " << finfo.smem_len << " line length " << finfo.line_length << '\n';

					std::cout << "Red bitfield: offset " << vinfo.red.offset << " length " << vinfo.red.length << " msb_right " << vinfo.red.msb_right << '\n';
					std::cout << "Green bitfield: offset " << vinfo.green.offset << " length " << vinfo.green.length << " msb_right " << vinfo.green.msb_right << '\n';
					std::cout << "Blue bitfield: offset " << vinfo.blue.offset << " length " << vinfo.blue.length << " msb_right " << vinfo.blue.msb_right << '\n';
				}

				uint8_t* DisplayRam = (uint8_t*)mmap(0,finfo.smem_len,PROT_READ | PROT_WRITE,MAP_SHARED,File, 0);
				assert(DisplayRam);
				if( DisplayRam != NULL )
				{
					newFrameBuffer = new FrameBuffer(File,DisplayRam,finfo,vinfo,pCreationFlags);					
				}
			}
		}
	}

	if( newFrameBuffer == NULL )
	{
		close(File);
		if(verbose)
		{
			std::cerr << "Error: cannot open framebuffer device.\n";
		}
	}
#endif //#ifdef USE_X11_EMULATION

	return newFrameBuffer;
}

FrameBuffer::FrameBuffer(int pFile,uint8_t* pDisplayBuffer,struct fb_fix_screeninfo pFixInfo,struct fb_var_screeninfo pScreenInfo,int pCreationFlags):
	mWidth(pScreenInfo.xres),
	mHeight(pScreenInfo.yres),

	mDisplayBufferStride(pFixInfo.line_length),
	mDisplayBufferPixelSize(pScreenInfo.bits_per_pixel/8),
	mDisplayBufferSize(pFixInfo.smem_len),
	mDisplayBufferFile(pFile),
	mDisplayBuffer(pDisplayBuffer),

	mVariableScreenInfo(pScreenInfo),
	mVerbose( (pCreationFlags&VERBOSE_MESSAGES) != 0 ),
	mRotation(pCreationFlags&(3<<1))
{
	FrameBuffer::mKeepGoing = true;

	// Lets hook ctrl + c.
	mUsersSignalAction = signal(SIGINT,CtrlHandler);

	// Try to connect to a mouse. But only if not X11
#ifndef USE_X11_EMULATION
	const bool verbose = (pCreationFlags&VERBOSE_MESSAGES) != 0;
	const char* MouseDeviceName = "/dev/input/event0";
	mPointer.mDevice = open(MouseDeviceName,O_RDONLY|O_NONBLOCK);
	if( verbose )
	{
		if(  mPointer.mDevice >  0 )
		{
			char name[256] = "Unknown";
			if( ioctl(mPointer.mDevice, EVIOCGNAME(sizeof(name)), name) == 0 )
			{
				std::clog << "Reading mouse from: handle = " << mPointer.mDevice << " name = " << name << "\n";
			}
			else
			{
				std::clog << "Open mouse device" << MouseDeviceName << "\n" ;
			}
		}
		else
		{
			std::clog << "Failed to open mouse device " << MouseDeviceName << "\n";
		}
	}
#endif
}

FrameBuffer::~FrameBuffer()
{
#ifdef USE_X11_EMULATION
	delete mX11;
#else
	if(mVerbose)
	{
		std::cout << "Freeing frame buffer resources, frame buffer object will be invalid and not unusable.\n";
	}

	// First make sure monitor is not left showing a static screen. Clear to black.
	memset(mDisplayBuffer,0,mDisplayBufferSize);

	munmap((void*)mDisplayBuffer,mDisplayBufferSize);
	close(mDisplayBufferFile);
#endif //#ifdef USE_X11_EMULATION
}

void FrameBuffer::OnApplicationExitRequest()
{
	FrameBuffer::mKeepGoing = false;
	if( FrameBuffer::mSystemEventHandler )
	{
		SystemEventData data(SYSTEM_EVENT_EXIT_REQUEST);
		FrameBuffer::mSystemEventHandler(data);
	}
}

void FrameBuffer::Present(const DrawBuffer& pImage)
{
#ifdef NDEBUG
	#define DBG_REPORT_PRESENT_SPEED(MESSAGE__){}
#else
	#define DBG_REPORT_PRESENT_SPEED(MESSAGE__)if( mVerbose && mReportedPresentSpeed == false ){mReportedPresentSpeed = true;std::clog << MESSAGE__;}
#endif


	// When optimized by compiler these const vars will
	// all move to generate the same code as if I made it all one line and unreadable!
	// Trust your optimizer. :)
	const size_t RedShift = mVariableScreenInfo.red.offset;
	const size_t GreenShift = mVariableScreenInfo.green.offset;
	const size_t BlueShift = mVariableScreenInfo.blue.offset;

	if( GetIsNativeFormat(pImage) )
	{// Early out...
		DBG_REPORT_PRESENT_SPEED("Optimal frame buffer copy mode taken\n");

		// Copy mDisplayBufferSize bytes, not the number of source, then we can't over flow what we have to write to.
		memcpy(mDisplayBuffer,pImage.mPixels.data(),mDisplayBufferSize);
	}
	else if( mDisplayBufferPixelSize == 2 )
	{
		DBG_REPORT_PRESENT_SPEED("Slow 16Bit frame buffer copy mode taken\n");
		u_int8_t* dst = (u_int8_t*)(mDisplayBuffer);
		const u_int8_t* src = pImage.mPixels.data();
		for( int y = 0 ; y < mHeight ; y++, dst += mDisplayBufferStride )
		{
			for( int x = 0 ; x < mWidth ; x++, src += pImage.GetPixelSize() )
			{
				const uint16_t b = src[BLUE_PIXEL_INDEX] >> 3;
				const uint16_t g = src[GREEN_PIXEL_INDEX] >> 2;
				const uint16_t r = src[RED_PIXEL_INDEX] >> 3;

				const uint16_t pixel = (r << RedShift) | (g << GreenShift) | (b << BlueShift);

				assert( (y*mDisplayBufferStride) + x < mDisplayBufferSize );

				((uint16_t*)dst)[x] = pixel;
			}
		}
	}
	else
	{
		DBG_REPORT_PRESENT_SPEED("Sub optimal scanline frame buffer copy mode taken\n");

		const size_t RED_OFFSET = (RedShift/8);
		const size_t GREEN_OFFSET = (GreenShift/8);
		const size_t BLUE_OFFSET = (BlueShift/8);

		assert( mDisplayBufferPixelSize == 3 || mDisplayBufferPixelSize == 4 );
		u_int8_t* dst = mDisplayBuffer;
		const u_int8_t* src = pImage.mPixels.data();

		switch( mRotation )
		{
		case DISPLAY_ROTATED_0:
			for( int y = 0 ; y < mHeight ; y++, dst += mDisplayBufferStride )
			{
				u_int8_t* pixel = dst;
				for( int x = 0 ; x < mWidth ; x++, src += pImage.GetPixelSize(), pixel += mDisplayBufferPixelSize )
				{
					assert( pixel + RED_OFFSET < mDisplayBuffer + mDisplayBufferSize );
					assert( pixel + GREEN_OFFSET < mDisplayBuffer + mDisplayBufferSize );
					assert( pixel + BLUE_OFFSET < mDisplayBuffer + mDisplayBufferSize );

					pixel[ BLUE_OFFSET ]	= src[0];
					pixel[ GREEN_OFFSET ]	= src[1];
					pixel[ RED_OFFSET ]		= src[2];
				}
			}
			break;

		case DISPLAY_ROTATED_90:
			for( int y = 0 ; y < mWidth ; y++ )
			{
				u_int8_t* pixel = dst + (y*(mDisplayBufferPixelSize)) + (mDisplayBufferStride*(mHeight-1));
				src = pImage.mPixels.data() + (pImage.GetStride()*y);
				for( int x = 0 ; x < mHeight ; x++, src += pImage.GetPixelSize(), pixel -= mDisplayBufferStride )
				{
					assert( pixel + RED_OFFSET < mDisplayBuffer + mDisplayBufferSize );
					assert( pixel + GREEN_OFFSET < mDisplayBuffer + mDisplayBufferSize );
					assert( pixel + BLUE_OFFSET < mDisplayBuffer + mDisplayBufferSize );

					pixel[ BLUE_OFFSET ]	= src[0];
					pixel[ GREEN_OFFSET ]	= src[1];
					pixel[ RED_OFFSET ]		= src[2];
				}
			}
			break;

		case DISPLAY_ROTATED_180:
			dst += mDisplayBufferSize - (mDisplayBufferPixelSize*1);
			for( int y = 0 ; y < mHeight ; y++, dst -= mDisplayBufferStride )
			{
				u_int8_t* pixel = dst;
				for( int x = 0 ; x < mWidth ; x++, src += pImage.GetPixelSize(), pixel -= mDisplayBufferPixelSize )
				{
					assert( pixel + RED_OFFSET < mDisplayBuffer + mDisplayBufferSize );
					assert( pixel + GREEN_OFFSET < mDisplayBuffer + mDisplayBufferSize );
					assert( pixel + BLUE_OFFSET < mDisplayBuffer + mDisplayBufferSize );

					pixel[ BLUE_OFFSET ]	= src[0];
					pixel[ GREEN_OFFSET ]	= src[1];
					pixel[ RED_OFFSET ]		= src[2];
				}
			}
			break;

		case DISPLAY_ROTATED_270:
			for( int y = 0 ; y < mWidth ; y++ )
			{
				u_int8_t* pixel = dst + (y*(mDisplayBufferPixelSize)) + (mDisplayBufferStride*(mHeight-1));
				src = pImage.mPixels.data() + (pImage.GetStride()*(mWidth-y));
				for( int x = 0 ; x < mHeight ; x++, src -= pImage.GetPixelSize(), pixel -= mDisplayBufferStride )
				{
					assert( pixel + RED_OFFSET < mDisplayBuffer + mDisplayBufferSize );
					assert( pixel + GREEN_OFFSET < mDisplayBuffer + mDisplayBufferSize );
					assert( pixel + BLUE_OFFSET < mDisplayBuffer + mDisplayBufferSize );

					pixel[ BLUE_OFFSET ]	= src[0];
					pixel[ GREEN_OFFSET ]	= src[1];
					pixel[ RED_OFFSET ]		= src[2];
				}
			}
			break;
		}
	}

	// Now do event processing.
	ProcessSystemEvents();
}

void FrameBuffer::ProcessSystemEvents()
{
#ifdef USE_X11_EMULATION
	if( mX11 )
	{
		mX11->ProcessSystemEvents(mSystemEventHandler);
		if( mX11->mWindowReady )
		{
			mX11->RedrawWindow();
		}
	}
#else
	// We don't bother to read the mouse if no pEventHandler has been registered. Would be a waste of time.
	if( mPointer.mDevice > 0 && mSystemEventHandler )
	{
		struct input_event ev;
		// Grab all messages and process befor going to next frame.
		while( read(mPointer.mDevice,&ev,sizeof(ev)) > 0 )
		{
			// EV_SYN is a seperator of events.
			if( mVerbose && ev.type != EV_ABS && ev.type != EV_KEY && ev.type != EV_SYN )
			{// Anything I missed? 
				std::cout << std::hex << ev.type << " " << ev.code << " " << ev.value << "\n";
			}

			switch( ev.type )
			{
			case EV_KEY:
				switch (ev.code)
				{
				case BTN_TOUCH:
					SystemEventData data((ev.value != 0) ? SYSTEM_EVENT_POINTER_DOWN : SYSTEM_EVENT_POINTER_UP);
					data.mPointer.X = mPointer.mCurrent.x;
					data.mPointer.Y = mPointer.mCurrent.y;
					mSystemEventHandler(data);
					break;
				}
				break;

			case EV_ABS:
				switch (ev.code)
				{
				case ABS_X:
					mPointer.mCurrent.x = ev.value;
					break;

				case ABS_Y:
					mPointer.mCurrent.y = ev.value;
					break;
				}
				SystemEventData data(SYSTEM_EVENT_POINTER_MOVE);
				data.mPointer.X = mPointer.mCurrent.x;
				data.mPointer.Y = mPointer.mCurrent.y;
				mSystemEventHandler(data);
				break;
			}
		}   
	}
#endif //#ifdef USE_X11_EMULATION
}

sighandler_t FrameBuffer::mUsersSignalAction = NULL;
FrameBuffer::SystemEventHandler FrameBuffer::mSystemEventHandler = nullptr;
bool FrameBuffer::mKeepGoing = false;
void FrameBuffer::CtrlHandler(int SigNum)
{
	static int numTimesAskedToExit = 0;

	// Propergate to someone elses handler, if they felt they wanted to add one too.
	if( mUsersSignalAction != NULL )
	{
		mUsersSignalAction(SigNum);
	}

	if( numTimesAskedToExit > 2 )
	{
		std::cerr << "Asked to quit to many times, forcing exit in bad way\n";
		exit(1);
	}

	OnApplicationExitRequest();
	std::cout << '\n'; // without this the command prompt may be at the end of the ^C char.
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// X11FrameBufferEmulation Implementation.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef USE_X11_EMULATION
/**
 * @brief This codebase is expected to be used for a system not running X11. But to aid development there is an option to 'emulate' a frame buffer with an X11 window.
 * 
 */
X11FrameBufferEmulation::X11FrameBufferEmulation():
	mDisplay(NULL),
	mWindow(0),
	mWindowReady(false),
	mDisplayBuffer(NULL)
{
	memset(&mFixInfo,0,sizeof(mFixInfo));
	memset(&mVarInfo,0,sizeof(mVarInfo));

}

X11FrameBufferEmulation::~X11FrameBufferEmulation()
{
	mWindowReady = false;

	// Do this after we have set the message pump flag to false so the events generated will case XNextEvent to return.
	delete []mDisplayBuffer;
	mDisplayBufferImage->data = NULL;// Keep valgrind happy. If I allow XDestroyImage to free the buffer, which it will, valgrind gets it's nickers in a twist. X11 fault!
	XDestroyImage(mDisplayBufferImage);
	XDestroyWindow(mDisplay,mWindow);
	XCloseDisplay(mDisplay);

}

bool X11FrameBufferEmulation::Open(bool pVerbose)
{
	// Before we do anything do this. So we can run message pump on it's own thread.
	XInitThreads();

	const struct fb_fix_screeninfo finfo =
	{
		"X11",
		0,
		X11_EMULATION_WIDTH * X11_EMULATION_HEIGHT * 4,
		FB_TYPE_PACKED_PIXELS,
		0,
		FB_VISUAL_TRUECOLOR,
		0,0,0,
		X11_EMULATION_WIDTH*4,
		0,0,0,0,
		{0,0}
	};
	mFixInfo = finfo;

	mVarInfo.xres = X11_EMULATION_WIDTH;
	mVarInfo.yres = X11_EMULATION_HEIGHT;
	mVarInfo.bits_per_pixel = 32;

	mVarInfo.red.offset = 16;
	mVarInfo.red.length = 8;

	mVarInfo.green.offset = 8;
	mVarInfo.green.length = 8;

	mVarInfo.blue.offset = 0;
	mVarInfo.blue.length = 8;
	
	mVarInfo.width = X11_EMULATION_WIDTH;
	mVarInfo.height = X11_EMULATION_HEIGHT;

	mDisplay = XOpenDisplay(NULL);
	if( mDisplay == NULL )
	{
		std::cerr << "Failed to open X display.\n";
		return false;
	}
		
	mWindow = XCreateSimpleWindow(
					mDisplay,
					RootWindow(mDisplay, 0),
					10, 10,
					X11_EMULATION_WIDTH, X11_EMULATION_HEIGHT,
					1,
					BlackPixel(mDisplay, 0),
					WhitePixel(mDisplay, 0));

	XSelectInput(mDisplay, mWindow, ExposureMask | KeyPressMask | StructureNotifyMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask );
	XMapWindow(mDisplay, mWindow);

	mDisplayBuffer = new uint8_t[mFixInfo.smem_len];
	memset(mDisplayBuffer,0,mFixInfo.smem_len);

	Visual *visual=DefaultVisual(mDisplay, 0);
	const int defDepth = DefaultDepth(mDisplay,DefaultScreen(mDisplay));
	mDisplayBufferImage = XCreateImage(
							mDisplay,
							visual,
							defDepth,
							ZPixmap,
							0,
							(char*)mDisplayBuffer,
							mVarInfo.width,
							mVarInfo.height,
							32,
							0);

	// So I can exit cleanly if the user uses the close window button.
	mDeleteMessage = XInternAtom(mDisplay, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(mDisplay, mWindow, &mDeleteMessage, 1);

	// wait for the expose message.
  	timespec SleepTime = {0,1000000};
	while( !mWindowReady )
	{
		ProcessSystemEvents(nullptr);
		nanosleep(&SleepTime,NULL);
	}

	return true;
}

void X11FrameBufferEmulation::ProcessSystemEvents(FrameBuffer::SystemEventHandler pEventHandler)
{
	// The message pump had to be moved to the same thread as the rendering because otherwise it would fail after a little bit of time.
	// This is dispite what the documentation stated.
	while( XPending(mDisplay) )
	{
		XEvent e;
		XNextEvent(mDisplay,&e);
		switch( e.type )
		{
		case Expose:
			mWindowReady = true;
			break;

		case ClientMessage:
			// All of this is to stop and error when we try to use the display but has been disconnected.
			// Snip from X11 docs.
			// 	Clients that choose not to include WM_DELETE_WINDOW in the WM_PROTOCOLS property
			// 	may be disconnected from the server if the user asks for one of the
			//  client's top-level windows to be deleted.
			// 
			// My note, could have been avoided if we just had something like XDisplayStillValid(my display)
			if (static_cast<Atom>(e.xclient.data.l[0]) == mDeleteMessage)
			{
				mWindowReady = false;
				FrameBuffer::OnApplicationExitRequest();
			}
			break;

		case KeyPress:
			// exit on ESC key press
			if ( e.xkey.keycode == 0x09 )
			{
				mWindowReady = false;
				FrameBuffer::OnApplicationExitRequest();
			}
			break;

		case MotionNotify:// Mouse movement
			if( pEventHandler )
			{
				SystemEventData data(SYSTEM_EVENT_POINTER_MOVE);
				data.mPointer.X = e.xmotion.x;
				data.mPointer.Y = e.xmotion.y;
				pEventHandler(data);
			}
			break;

		case ButtonPress:
			if( pEventHandler )
			{
				SystemEventData data(SYSTEM_EVENT_POINTER_DOWN);
				data.mPointer.X = e.xbutton.x;
				data.mPointer.Y = e.xbutton.y;
				pEventHandler(data);
			}
			break;

		case ButtonRelease:
			if( pEventHandler )
			{
				SystemEventData data(SYSTEM_EVENT_POINTER_UP);
				data.mPointer.X = e.xbutton.x;
				data.mPointer.Y = e.xbutton.y;
				pEventHandler(data);
			}
			break;
		}
	}
}

void X11FrameBufferEmulation::RedrawWindow()
{
	assert( mWindowReady );

	GC defGC = DefaultGC(mDisplay, DefaultScreen(mDisplay));
	const int ret = XPutImage(mDisplay, mWindow,defGC,mDisplayBufferImage,0,0,0,0,mVarInfo.width,mVarInfo.height);

	switch (ret)
	{
	case BadDrawable:
		std::cerr << "XPutImage failed BadDrawable\n";
		break;

	case BadGC:
		std::cerr << "XPutImage failed BadGC\n";
		break;

	case BadMatch:
		std::cerr << "XPutImage failed BadMatch\n";
		break;

	case BadValue:
		std::cerr << "XPutImage failed BadValue\n";
		break;

	default:
		break;
	}
}

#endif //#ifdef USE_X11_EMULATION

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Font Implementation.
///////////////////////////////////////////////////////////////////////////////////////////////////////////
static const uint8_t font8x13[] =
{
    0x00, 0x00, 0xaa, 0x00, 0x82, 0x00, 0x82, 0x00, 0x82, 0x00, 0xaa, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x7c, 0xfe, 0x7c, 0x38, 0x10, 0x00, 
    0x00, 0x00, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 
    0xaa, 0x55, 0xaa, 0x00, 0x00, 0xa0, 0xa0, 0xe0, 0xa0, 0xae, 0x04, 0x04, 
    0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x80, 0xc0, 0x80, 0x8e, 0x08, 
    0x0c, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x80, 0x80, 0x6c, 
    0x0a, 0x0c, 0x0a, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 
    0xee, 0x08, 0x0c, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x24, 
    0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 
    0x10, 0x7c, 0x10, 0x10, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 
    0xa0, 0xa0, 0xa0, 0xa8, 0x08, 0x08, 0x08, 0x0e, 0x00, 0x00, 0x00, 0x00, 
    0x88, 0x88, 0x50, 0x50, 0x2e, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x1f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x30, 0xc0, 0x30, 0x0e, 0x00, 
    0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x18, 0x06, 0x18, 0xe0, 
    0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x44, 0x44, 
    0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x7e, 0x08, 
    0x10, 0x7e, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x20, 0x70, 
    0x20, 0x20, 0x20, 0x62, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 
    0x24, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x24, 0x24, 0x7e, 0x24, 0x7e, 0x24, 0x24, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x10, 0x3c, 0x50, 0x50, 0x38, 0x14, 0x14, 0x78, 0x10, 0x00, 
    0x00, 0x00, 0x00, 0x22, 0x52, 0x24, 0x08, 0x08, 0x10, 0x24, 0x2a, 0x44, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x48, 0x48, 0x30, 0x4a, 0x44, 
    0x3a, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x08, 0x10, 0x10, 0x10, 
    0x08, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x20, 0x10, 0x10, 0x08, 0x08, 
    0x08, 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x18, 
    0x7e, 0x18, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 
    0x10, 0x7c, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x30, 0x40, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x10, 0x00, 0x00, 
    0x00, 0x02, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x80, 0x00, 0x00, 
    0x00, 0x00, 0x18, 0x24, 0x42, 0x42, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00, 
    0x00, 0x00, 0x00, 0x10, 0x30, 0x50, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 
    0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x02, 0x04, 0x18, 0x20, 0x40, 
    0x7e, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 0x04, 0x08, 0x1c, 0x02, 0x02, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0c, 0x14, 0x24, 0x44, 0x44, 
    0x7e, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x40, 0x40, 0x5c, 0x62, 
    0x02, 0x02, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x20, 0x40, 0x40, 
    0x5c, 0x62, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 0x04, 
    0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 
    0x42, 0x42, 0x3c, 0x42, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x3c, 
    0x42, 0x42, 0x46, 0x3a, 0x02, 0x02, 0x04, 0x38, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x10, 0x38, 0x10, 0x00, 0x00, 0x10, 0x38, 0x10, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x10, 0x38, 0x10, 0x00, 0x00, 0x38, 0x30, 0x40, 0x00, 
    0x00, 0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x08, 0x10, 0x20, 
    0x40, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x02, 0x04, 0x08, 0x08, 
    0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x4e, 0x52, 0x56, 
    0x4a, 0x40, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x42, 0x42, 0x42, 
    0x7e, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x78, 0x44, 0x42, 0x44, 
    0x78, 0x44, 0x42, 0x44, 0x78, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 
    0x40, 0x40, 0x40, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x78, 0x44, 
    0x42, 0x42, 0x42, 0x42, 0x42, 0x44, 0x78, 0x00, 0x00, 0x00, 0x00, 0x7e, 
    0x40, 0x40, 0x40, 0x78, 0x40, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x00, 
    0x7e, 0x40, 0x40, 0x40, 0x78, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 
    0x00, 0x3c, 0x42, 0x40, 0x40, 0x40, 0x4e, 0x42, 0x46, 0x3a, 0x00, 0x00, 
    0x00, 0x00, 0x42, 0x42, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x42, 0x00, 
    0x00, 0x00, 0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 
    0x00, 0x00, 0x00, 0x00, 0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x44, 
    0x38, 0x00, 0x00, 0x00, 0x00, 0x42, 0x44, 0x48, 0x50, 0x60, 0x50, 0x48, 
    0x44, 0x42, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 
    0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0xc6, 0xaa, 0x92, 
    0x92, 0x82, 0x82, 0x82, 0x00, 0x00, 0x00, 0x00, 0x42, 0x42, 0x62, 0x52, 
    0x4a, 0x46, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 
    0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x42, 
    0x42, 0x42, 0x7c, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x3c, 
    0x42, 0x42, 0x42, 0x42, 0x42, 0x52, 0x4a, 0x3c, 0x02, 0x00, 0x00, 0x00, 
    0x7c, 0x42, 0x42, 0x42, 0x7c, 0x50, 0x48, 0x44, 0x42, 0x00, 0x00, 0x00, 
    0x00, 0x3c, 0x42, 0x40, 0x40, 0x3c, 0x02, 0x02, 0x42, 0x3c, 0x00, 0x00, 
    0x00, 0x00, 0xfe, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 
    0x00, 0x00, 0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 
    0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x44, 0x44, 0x44, 0x28, 0x28, 0x28, 
    0x10, 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x82, 0x82, 0x92, 0x92, 0x92, 
    0xaa, 0x44, 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x44, 0x28, 0x10, 0x28, 
    0x44, 0x82, 0x82, 0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x44, 0x28, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 0x04, 0x08, 
    0x10, 0x20, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x20, 0x20, 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 
    0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x78, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x78, 0x00, 0x00, 0x00, 0x00, 
    0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 
    0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 
    0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x5c, 0x62, 0x42, 0x42, 0x62, 
    0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x40, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x3a, 0x46, 0x42, 
    0x42, 0x46, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 
    0x7e, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x20, 0x20, 
    0x7c, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x3a, 0x44, 0x44, 0x38, 0x40, 0x3c, 0x42, 0x3c, 0x00, 0x00, 0x40, 0x40, 
    0x40, 0x5c, 0x62, 0x42, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x10, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x04, 0x00, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x44, 0x44, 0x38, 0x00, 
    0x00, 0x40, 0x40, 0x40, 0x44, 0x48, 0x70, 0x48, 0x44, 0x42, 0x00, 0x00, 
    0x00, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xec, 0x92, 0x92, 0x92, 0x92, 0x82, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x62, 0x42, 0x42, 0x42, 
    0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x42, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x62, 0x42, 
    0x62, 0x5c, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x46, 
    0x42, 0x46, 0x3a, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 
    0x22, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x3c, 0x42, 0x30, 0x0c, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 
    0x20, 0x7c, 0x20, 0x20, 0x20, 0x22, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x3a, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x44, 0x44, 0x44, 0x28, 0x28, 0x10, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x82, 0x82, 0x92, 0x92, 0xaa, 0x44, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x42, 0x42, 0x46, 0x3a, 0x02, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x04, 0x08, 0x10, 0x20, 
    0x7e, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x10, 0x10, 0x08, 0x30, 0x08, 0x10, 
    0x10, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x70, 0x08, 0x08, 0x10, 0x0c, 
    0x10, 0x08, 0x08, 0x70, 0x00, 0x00, 0x00, 0x00, 0x24, 0x54, 0x48, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x54, 0x50, 
    0x50, 0x54, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x22, 0x20, 
    0x70, 0x20, 0x20, 0x20, 0x62, 0xdc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x42, 0x3c, 0x24, 0x24, 0x3c, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 
    0x82, 0x44, 0x28, 0x7c, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 
    0x10, 0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 
    0x18, 0x24, 0x20, 0x18, 0x24, 0x24, 0x18, 0x04, 0x24, 0x18, 0x00, 0x00, 
    0x00, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x38, 0x44, 0x92, 0xaa, 0xa2, 0xaa, 0x92, 0x44, 0x38, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x38, 0x04, 0x3c, 0x44, 0x3c, 0x00, 0x7c, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x24, 0x48, 0x90, 0x48, 0x24, 
    0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x02, 
    0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x44, 0x92, 0xaa, 0xaa, 
    0xb2, 0xaa, 0x44, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 
    0x24, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x10, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x30, 
    0x48, 0x08, 0x30, 0x40, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x30, 0x48, 0x10, 0x08, 0x48, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x42, 0x42, 0x42, 0x66, 0x5a, 
    0x40, 0x00, 0x00, 0x00, 0x3e, 0x74, 0x74, 0x74, 0x34, 0x14, 0x14, 0x14, 
    0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x08, 0x18, 0x00, 0x20, 0x60, 0x20, 0x20, 0x20, 0x70, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x48, 0x48, 0x30, 
    0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x48, 
    0x24, 0x12, 0x24, 0x48, 0x90, 0x00, 0x00, 0x00, 0x00, 0x40, 0xc0, 0x40, 
    0x40, 0x42, 0xe6, 0x0a, 0x12, 0x1a, 0x06, 0x00, 0x00, 0x00, 0x40, 0xc0, 
    0x40, 0x40, 0x4c, 0xf2, 0x02, 0x0c, 0x10, 0x1e, 0x00, 0x00, 0x00, 0x60, 
    0x90, 0x20, 0x10, 0x92, 0x66, 0x0a, 0x12, 0x1a, 0x06, 0x00, 0x00, 0x00, 
    0x00, 0x10, 0x00, 0x10, 0x10, 0x20, 0x40, 0x42, 0x42, 0x3c, 0x00, 0x00, 
    0x00, 0x10, 0x08, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x00, 
    0x00, 0x00, 0x08, 0x10, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7e, 0x42, 0x42, 
    0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7e, 0x42, 
    0x42, 0x00, 0x00, 0x00, 0x32, 0x4c, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7e, 
    0x42, 0x42, 0x00, 0x00, 0x00, 0x24, 0x24, 0x00, 0x18, 0x24, 0x42, 0x42, 
    0x7e, 0x42, 0x42, 0x00, 0x00, 0x00, 0x18, 0x24, 0x18, 0x18, 0x24, 0x42, 
    0x42, 0x7e, 0x42, 0x42, 0x00, 0x00, 0x00, 0x00, 0x6e, 0x90, 0x90, 0x90, 
    0x9c, 0xf0, 0x90, 0x90, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 
    0x40, 0x40, 0x40, 0x40, 0x42, 0x3c, 0x08, 0x10, 0x00, 0x10, 0x08, 0x00, 
    0x7e, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x08, 0x10, 
    0x00, 0x7e, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x18, 
    0x24, 0x00, 0x7e, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 
    0x24, 0x24, 0x00, 0x7e, 0x40, 0x40, 0x78, 0x40, 0x40, 0x7e, 0x00, 0x00, 
    0x00, 0x20, 0x10, 0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 
    0x00, 0x00, 0x08, 0x10, 0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 0x7c, 
    0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x7c, 0x00, 0x00, 0x00, 0x44, 0x44, 0x00, 0x7c, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x78, 0x44, 0x42, 0x42, 0xe2, 0x42, 
    0x42, 0x44, 0x78, 0x00, 0x00, 0x00, 0x64, 0x98, 0x00, 0x82, 0xc2, 0xa2, 
    0x92, 0x8a, 0x86, 0x82, 0x00, 0x00, 0x00, 0x20, 0x10, 0x00, 0x7c, 0x82, 
    0x82, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x7c, 
    0x82, 0x82, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 
    0x7c, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00, 0x64, 0x98, 
    0x00, 0x7c, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00, 0x44, 
    0x44, 0x00, 0x7c, 0x82, 0x82, 0x82, 0x82, 0x82, 0x7c, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00, 0x00, 0x00, 
    0x00, 0x02, 0x3c, 0x46, 0x4a, 0x4a, 0x52, 0x52, 0x52, 0x62, 0x3c, 0x40, 
    0x00, 0x00, 0x20, 0x10, 0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 
    0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 
    0x3c, 0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x24, 0x24, 0x00, 0x42, 0x42, 0x42, 0x42, 
    0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x44, 0x44, 0x28, 
    0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x40, 0x7c, 0x42, 0x42, 
    0x42, 0x7c, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x38, 0x44, 0x44, 
    0x48, 0x50, 0x4c, 0x42, 0x42, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x10, 0x08, 
    0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x04, 
    0x08, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 0x00, 0x00, 0x00, 0x00, 
    0x18, 0x24, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 0x00, 0x00, 0x00, 
    0x00, 0x32, 0x4c, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 0x00, 0x00, 
    0x00, 0x00, 0x24, 0x24, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 0x00, 
    0x00, 0x00, 0x18, 0x24, 0x18, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x46, 0x3a, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6c, 0x12, 0x7c, 0x90, 0x92, 
    0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x40, 
    0x42, 0x3c, 0x08, 0x10, 0x00, 0x00, 0x10, 0x08, 0x00, 0x3c, 0x42, 0x7e, 
    0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x3c, 0x42, 
    0x7e, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x3c, 
    0x42, 0x7e, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x24, 0x24, 0x00, 
    0x3c, 0x42, 0x7e, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x20, 0x10, 
    0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x10, 
    0x20, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00, 0x00, 0x00, 
    0x30, 0x48, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00, 0x00, 
    0x00, 0x48, 0x48, 0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x7c, 0x00, 0x00, 
    0x00, 0x24, 0x18, 0x28, 0x04, 0x3c, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00, 
    0x00, 0x00, 0x00, 0x32, 0x4c, 0x00, 0x5c, 0x62, 0x42, 0x42, 0x42, 0x42, 
    0x00, 0x00, 0x00, 0x00, 0x20, 0x10, 0x00, 0x3c, 0x42, 0x42, 0x42, 0x42, 
    0x3c, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x3c, 0x42, 0x42, 0x42, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x18, 0x24, 0x00, 0x3c, 0x42, 0x42, 
    0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x32, 0x4c, 0x00, 0x3c, 0x42, 
    0x42, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x24, 0x24, 0x00, 0x3c, 
    0x42, 0x42, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 
    0x00, 0x7c, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x02, 0x3c, 0x46, 0x4a, 0x52, 0x62, 0x3c, 0x40, 0x00, 0x00, 0x00, 0x20, 
    0x10, 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x3a, 0x00, 0x00, 0x00, 0x00, 
    0x08, 0x10, 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x3a, 0x00, 0x00, 0x00, 
    0x00, 0x18, 0x24, 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x3a, 0x00, 0x00, 
    0x00, 0x00, 0x28, 0x28, 0x00, 0x44, 0x44, 0x44, 0x44, 0x44, 0x3a, 0x00, 
    0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x42, 0x42, 0x42, 0x46, 0x3a, 0x02, 
    0x42, 0x3c, 0x00, 0x00, 0x00, 0x40, 0x40, 0x5c, 0x62, 0x42, 0x42, 0x62, 
    0x5c, 0x40, 0x40, 0x00, 0x00, 0x24, 0x24, 0x00, 0x42, 0x42, 0x42, 0x46, 
    0x3a, 0x02, 0x42, 0x3c
};

PixelFont::PixelFont(int pPixelSize):mPixelSize(1),mBorderOn(false)
{
	SetPenColour(255,255,255);
	SetPixelSize(pPixelSize);
}

PixelFont::~PixelFont()
{
}

void PixelFont::DrawChar(DrawBuffer& pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,int pChar)const
{
	const uint8_t* charBits = font8x13 + pChar * 13;
	if( mPixelSize == 1 )
	{
		if( mBorderOn )
		{
			const uint8_t* d = charBits;
			for(int y = 0 ; y < 13 ; y++ , d++)
			{
				int x = pX;
				const uint8_t bits = *d;
				if( bits != 0 )
				{
					for( int bit = 7 ; bit > -1 ; bit-- , x++ )
					{
						if( bits&(1<<bit) )
						{
							pDest.FillRectangle(x-1,pY + y - 1,x+1,pY + y + 1,mBorderColour.r,mBorderColour.g,mBorderColour.b);
						}
					}
				}
			}
		}

		const uint8_t* d = charBits;
		for(int y = 0 ; y < 13 ; y++ , d++)
		{
			int x = pX;
			const uint8_t bits = *d;
			if( bits != 0 )
			{
				for( int bit = 7 ; bit > -1 ; bit-- , x++ )
				{
					if( bits&(1<<bit) )
					{
						pDest.WritePixel(x,pY + y,pRed,pGreen,pBlue);
					}
				}
			}
		}
	}
	else
	{
		if( mBorderOn )
		{
			const uint8_t* d = charBits;
			int by = pY;
			for(int y = 0 ; y < 13 ; y++ , d++ , by += mPixelSize )
			{
				int x = pX;
				const uint8_t bits = *d;
				if( bits != 0 )
				{
					for( int bit = 7 ; bit > -1 ; bit-- , x+=mPixelSize )
					{
						if( bits&(1<<bit) )
						{
							pDest.FillRectangle(x-1,by-1,x+mPixelSize+1,by+mPixelSize+1,mBorderColour.r,mBorderColour.g,mBorderColour.b);
						}
					}
				}
			}
		}

		const uint8_t* d = charBits;
		for(int y = 0 ; y < 13 ; y++ , d++ , pY += mPixelSize )
		{
			int x = pX;
			const uint8_t bits = *d;
			if( bits != 0 )
			{
				for( int bit = 7 ; bit > -1 ; bit-- , x+=mPixelSize )
				{
					if( bits&(1<<bit) )
					{
						pDest.FillRectangle(x,pY,x+mPixelSize,pY+mPixelSize,pRed,pGreen,pBlue);
					}
				}
			}
		}
	}

}

void PixelFont::Print(DrawBuffer& pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,const char* pText)const
{
	while(*pText)
	{
		DrawChar(pDest,pX,pY,pRed,pGreen,pBlue,*pText);
		pX += 8*mPixelSize;
		pText++;
	};
}

void PixelFont::Print(DrawBuffer& pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,const std::string& pText)const
{
	Print(pDest,pX,pY,pRed,pGreen,pBlue,pText.c_str());
}

void PixelFont::Printf(DrawBuffer& pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,const char* pFmt,...)const
{
	char buf[1024];	
	va_list args;
	va_start(args, pFmt);
	vsnprintf(buf, sizeof(buf), pFmt, args);
	va_end(args);
	Print(pDest, pX,pY,pRed,pGreen,pBlue, buf);
}

void PixelFont::Print(DrawBuffer& pDest,int pX,int pY,const char* pText)const
{
	Print(pDest,pX,pY,mPenColour.r,mPenColour.g,mPenColour.b,pText);
}

void PixelFont::Print(DrawBuffer& pDest,int pX,int pY,const std::string& pText)const
{
	Print(pDest,pX,pY,pText.c_str());
}

void PixelFont::Printf(DrawBuffer& pDest,int pX,int pY,const char* pFmt,...)const
{
	char buf[1024];	
	va_list args;
	va_start(args, pFmt);
	vsnprintf(buf, sizeof(buf), pFmt, args);
	va_end(args);
	Print(pDest, pX,pY,mPenColour.r,mPenColour.g,mPenColour.b, buf);
}

void PixelFont::SetPenColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	mPenColour.r = pRed;
	mPenColour.g = pGreen;
	mPenColour.b = pBlue;
}

void PixelFont::SetPixelSize(int pPixelSize)
{
	assert( pPixelSize > 0 );
	if( pPixelSize > 0 )
		mPixelSize = pPixelSize;
}

void PixelFont::SetBorder(bool pOn,uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	mBorderOn = pOn;
	mBorderColour.r = pRed;
	mBorderColour.g = pGreen;
	mBorderColour.b = pBlue;
}


#ifdef USE_FREETYPEFONTS
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Font Implementation.
// The code at https://github.com/kevinboone/fbtextdemo/blob/main/src/main.c
// was used and copied for reference. Also see http://kevinboone.me/fbtextdemo.html?i=1
///////////////////////////////////////////////////////////////////////////////////////////////////////////
FT_Library FreeTypeFont::mFreetype = NULL;
int FreeTypeFont::mFreetypeRefCount = 0;

FreeTypeFont::FreeTypeFont(const std::string& pFontName,int pPixelHeight,bool pVerbose):
	mVerbose(pVerbose),
	mFace(NULL),
	mOK(false)
{
	mPenColour.r = mPenColour.g = mPenColour.b = 255;
	mBackgroundColour.r = mBackgroundColour.g = mBackgroundColour.b = 0;

	RecomputeBlendTable();

	if( mFreetype == NULL )
	{
		mFreetypeRefCount = 0;
		if( FT_Init_FreeType(&mFreetype) == 0 )
		{
			if(mVerbose)
			{
				std::cout << "Freetype font library created\n";
			}	
		}
		else
		{
			std::cerr << "Failed to init free type font library.\n";
		}
	}
	mFreetypeRefCount++;
	if(mVerbose)
	{
		std::cout << "mFreetypeRefCount = " << mFreetypeRefCount << '\n';
	}	

	if( FT_New_Face(mFreetype,pFontName.c_str(),0,&mFace) == 0 )
	{
		if( FT_Set_Pixel_Sizes(mFace,0,pPixelHeight) == 0 )
		{
			mOK = true;
		}
		else if( mVerbose )
		{
			std::cerr << "Failed to set pixel size " << pPixelHeight << " for true type font " << pFontName << '\n';
		}
	}
	else if( mVerbose )
	{
		std::cerr << "Failed to load true type font " << pFontName << '\n';
	}
}

FreeTypeFont::~FreeTypeFont()
{
	FT_Done_Face(mFace);
	
	mFreetypeRefCount--;
	if( mFreetypeRefCount <= 0 && mFreetype != NULL )
	{
		if( FT_Done_FreeType(mFreetype) == 0 )
		{
			mFreetype = NULL;
			if(mVerbose)
			{
				std::cout << "Freetype font library deleted\n";
			}	
		}
		else
		{
			std::cerr << "Failed to delete free type font library.\n";
		}
	}
}

int FreeTypeFont::DrawChar(DrawBuffer& pDest,int pX,int pY,char pChar)const
{
	if( !mOK )
		return pX;

	// Comments from original example source by Kevin Boone. http://kevinboone.me/fbtextdemo.html?i=1

	// Note that TT fonts have no built-in padding. 
	// That is, first,
	//  the top row of the bitmap is the top row of pixels to 
	//  draw. These rows usually won't be at the face bounding box. We need to
	//  work out the overall height of the character cell, and
	//  offset the drawing vertically by that amount. 
	//
	// Similar, there is no left padding. The first pixel in each row will not
	//  be drawn at the left margin of the bounding box, but in the centre of
	//  the screen width that will be occupied by the glyph.
	//
	//  We need to calculate the x and y offsets of the glyph, but we can't do
	//  this until we've loaded the glyph, because metrics
	//  won't be available.

	// Note that, by default, TT metrics are in 64'ths of a pixel, hence
	//  all the divide-by-64 operations below.

	// Get a FreeType glyph index for the character. If there is no
	//  glyph in the face for the character, this function returns
	//  zero.  
	FT_UInt gi = FT_Get_Char_Index (mFace, pChar);
	if( gi == 0 )
	{// Character not found, so default to space.
		return pX + (mFace->bbox.xMax / 64);
	}

	// Loading the glyph makes metrics data available
	if( FT_Load_Glyph (mFace, gi, FT_LOAD_DEFAULT ) != 0 )
	{
		return pX;
	}

	// Now we have the metrics, let's work out the x and y offset
	//  of the glyph from the specified x and y. Because there is
	//  no padding, we can't just draw the bitmap so that it's
	//  TL corner is at (x,y) -- we must insert the "missing" 
	//  padding by aligning the bitmap in the space available.

	// bbox.yMax is the height of a bounding box that will enclose
	//  any glyph in the face, starting from the glyph baseline.
// Code changed, was casing it to render in the Y center of the font not on the base line. Will add it as an option in the future. Richard.
	int bbox_ymax = 0;//mFace->bbox.yMax / 64;

	// horiBearingX is the height of the top of the glyph from
	//   the baseline. So we work out the y offset -- the distance
	//   we must push down the glyph from the top of the bounding
	//   box -- from the height and the Y bearing.
	int y_off = bbox_ymax - mFace->glyph->metrics.horiBearingY / 64;

	// glyph_width is the pixel width of this specific glyph
	int glyph_width = mFace->glyph->metrics.width / 64;

	// Advance is the amount of x spacing, in pixels, allocated
	//   to this glyph
	int advance = mFace->glyph->metrics.horiAdvance / 64;

	// Work out where to draw the left-most row of pixels --
	//   the x offset -- by halving the space between the 
	//   glyph width and the advance
	int x_off = (advance - glyph_width) / 2;

	// So now we have (x_off,y_off), the location at which to
	//   start drawing the glyph bitmap.

	// Rendering a loaded glyph creates the bitmap
	if( FT_Render_Glyph(mFace->glyph, FT_RENDER_MODE_NORMAL) != 0 )
	{
		return pX;
	}

	// Write out the glyph row-by-row using framebuffer_set_pixel
	// Note that the glyph can contain horizontal padding. We need
	//  to take this into account when working out where the pixels
	//  are in memory, but we don't actually need to "draw" these
	//  empty pixels. bitmap.width is the number of pixels that actually
	//  contain values; bitmap.pitch is the spacing between bitmap
	//  rows in memory.
	const uint8_t* src = mFace->glyph->bitmap.buffer;
	for (int i = 0; i < (int)mFace->glyph->bitmap.rows; i++ , src += mFace->glyph->bitmap.pitch )
	{
		// Row offset is the distance from the top of the framebuffer
		//  of this particular row of pixels in the glyph.
		int row_offset = pY + i + y_off;
		for (int j = 0; j < (int)mFace->glyph->bitmap.width; j++ )
		{
			const uint8_t p = src[j];

			// Working out the Y position is a little fiddly. horiBearingY 
			//  is how far the glyph extends about the baseline. We push
			//  the bitmap down by the height of the bounding box, and then
			//  back up by this "bearing" value.

			// As I am not blending against the background I have to do this test.
			// Otherwise we state to crop previous letters.
			// Should really be blending agaist the background. But that is slow.... Richard.
			if( p > 0 )
			{
				const int pixelX = pX + j + x_off;
				const int pixelY = row_offset;
				pDest.WritePixel(pixelX,pixelY, mBlended[p].r, mBlended[p].g, mBlended[p].b);
			}
		}
	}
	// horiAdvance is the nominal X spacing between displayed glyphs. 
	return pX + advance;
}

void FreeTypeFont::Print(DrawBuffer& pDest,int pX,int pY,const char* pText)const
{
	while(*pText)
	{
		pX = DrawChar(pDest,pX,pY,*pText);
		pText++;
	};
}

void FreeTypeFont::Print(DrawBuffer& pDest,int pX,int pY,const std::string& pText)const
{
	Print(pDest,pX,pY,pText.c_str());
}

void FreeTypeFont::Printf(DrawBuffer& pDest,int pX,int pY,const char* pFmt,...)const
{
	char buf[1024];	
	va_list args;
	va_start(args, pFmt);
	vsnprintf(buf, sizeof(buf), pFmt, args);
	va_end(args);
	Print(pDest,pX,pY,buf);
}

void FreeTypeFont::SetPenColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	mPenColour.r = pRed;
	mPenColour.g = pGreen;
	mPenColour.b = pBlue;
	RecomputeBlendTable();
}

void FreeTypeFont::SetBackgroundColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
	mBackgroundColour.r = pRed;
	mBackgroundColour.g = pGreen;
	mBackgroundColour.b = pBlue;
	RecomputeBlendTable();
}

void FreeTypeFont::RecomputeBlendTable()
{
	uint8_t blendTable[256][3];

	TweenColoursRGB(mBackgroundColour.r,mBackgroundColour.g,mBackgroundColour.b,mPenColour.r,mPenColour.g,mPenColour.b,blendTable);

	// Unpack to speed up rendering.
	for( uint32_t p = 0 ; p < 256 ; p++ )
	{
		mBlended[p].r = blendTable[p][0];
		mBlended[p].g = blendTable[p][1];
		mBlended[p].b = blendTable[p][2];
	}
}

#endif //#ifdef USE_FREETYPEFONTS


///////////////////////////////////////////////////////////////////////////////////////////////////////////
};//namespace tiny2d
   
