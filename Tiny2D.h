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

#ifndef TINY_2D_H
#define TINY_2D_H

#include <vector>

#include <signal.h>
#include <stdint.h>
#include <linux/fb.h>

/**
 * @brief define USE_X11_EMULATION for a system running X11.
 * This codebase is targeting systems without X11, but sometimes we want to develop on a system with it.
 * This define allows that. But is expected to be used ONLY for development.
 * To set window draw size define X11_EMULATION_WIDTH and X11_EMULATION_HEIGHT
 */
#ifdef USE_X11_EMULATION
	#ifndef X11_EMULATION_WIDTH
		#define X11_EMULATION_WIDTH 1024
	#endif

	#ifndef X11_EMULATION_HEIGHT
		#define X11_EMULATION_HEIGHT 600
	#endif
#endif

/**
 * @brief The define USE_FREETYPEFONTS allows users of this lib to disable freetype support to reduce code size and dependencies.
 * Make sure you have freetype dev installed. sudo apt install libfreetype6-dev
 * Also add /usr/include/freetype2 to your build paths. The include macros need this.
 */
#ifdef USE_FREETYPEFONTS
	#include <freetype2/ft2build.h>
	#include FT_FREETYPE_H
#endif

namespace tiny2d{	// Using a namespace to try to prevent name clashes as my class name is kind of obvious. :)
///////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef uint32_t PixelColour;

#define MAKE_PIXEL_COLOUR(R__,G__,B__)	(((uint32_t)R__<<16)|((uint32_t)G__<<8)|((uint32_t)B__))

#define PIXEL_COLOUR_RED(COLOUR__)		( (uint8_t)((COLOUR__&0x00ff0000)>>16) )
#define PIXEL_COLOUR_GREEN(COLOUR__)	( (uint8_t)((COLOUR__&0x0000ff00)>>8) )
#define PIXEL_COLOUR_BLUE(COLOUR__)		( (uint8_t)(COLOUR__&0x000000ff) )

struct X11FrameBufferEmulation;
/**
 * @brief Represents the linux frame buffer display.
 * Is able to deal with and abstract out the various pixel formats. 
 */
class FrameBuffer
{
public:

	/**
	 * @brief Creates and opens a FrameBuffer object.
	 * If the OS does not support the frame buffer driver or there is some other error,
	 * this function will return NULL.
	 * For simplicity all drawing is done at eight bits per channel to an off screen bufffer.
	 * This makes the code very simple as the colour space conversion is only done when the
	 * offscreen buffer is copied to the display.
	 * 
	 * @param pVerbose get debugging information as the object is created.
	 * @return FrameBuffer* 
	 */
	static FrameBuffer* Open(bool pVerbose = false);

	~FrameBuffer();

	/**
	 * @brief Get the setting for Verbose debug output.
	 * 
	 * @return true 
	 * @return false 
	 */
	bool GetVerbose()const{return mVerbose;}

	/*
		Returns the width of the frame buffer.
	*/
	int GetWidth()const{return mWidth;}

	/*
		Returns the height of the frame buffer.
	*/
	int GetHeight()const{return mHeight;}

	/**
	 * @brief See if the app main loop should keep going.
	 * 
	 * @return true All is ok, so keep running.
	 * @return false Either the user requested an exit with ctrl+c or there was an error.
	 */
	bool GetKeepGoing()const{return FrameBuffer::mKeepGoing;}

	/**
	 * @brief Sets the flag for the main loop to false.
	 * You can only set it to false to prevet miss uses.
	 */
	static void SetKeepGoingFalse(){FrameBuffer::mKeepGoing = false;}

	/**
	 * @brief Depending on the buffering mode and the render environment will display the changes to the frame buffer on screen.
	 * 
	 */
	void Present();

	/**
	 * @brief Writes a single pixel with the passed red, green and blue values. 0 -> 255, 0 being off 255 being full on.
	 * The pixel will not be written if it's outside the frame buffers bounds.
	 * @param pX 
	 * @param pY 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 */
	void WritePixel(int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

	/**
	 * @brief Writes a single pixel with the passed red, green and blue values. 0 -> 255, 0 being off 255 being full on.
	 * The pixel will not be written if it's outside the frame buffers bounds.
	 * 
	 * @param pX 
	 * @param pY 
	 * @param pColour 
	 */
	void WritePixel(int pX,int pY,PixelColour pColour)
	{
		WritePixel(pX,pY,PIXEL_COLOUR_RED(pColour),PIXEL_COLOUR_GREEN(pColour),PIXEL_COLOUR_BLUE(pColour));
	}

	/**
	 * @brief Blends a single pixel with the frame buffer. does (S*A) + (D*(1-A))
	 * The pixel will not be written if it's outside the frame buffers bounds.
	 * 
	 * @param pX 
	 * @param pY 
	 * @param pRGBA Four bytes, pRGBA[0] == red, pRGBA[1] == green, pRGBA[2] == blue, pRGBA[3] == alpha
	 */
	void BlendPixel(int pX,int pY,const uint8_t* pRGBA);

	/**
	 * @brief Blends a single pixel with the frame buffer. does S + (D * A) Quicker but less flexable.
	 * The pixel will not be written if it's outside the frame buffers bounds.
	 * 
	 * @param pX 
	 * @param pY 
	 * @param pRGBA Four bytes, pRGBA[0] == red, pRGBA[1] == green, pRGBA[2] == blue, pRGBA[3] == alpha
	 */
	void BlendPreAlphaPixel(int pX,int pY,const uint8_t* pRGBA);
	
	
	/**
	 * @brief Clears the entire screen.
	 * 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 */
	void ClearScreen(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

	/**
	 * @brief Clears the entire screen.
	 * 
	 * @param pColour 
	 */
	void ClearScreen(PixelColour pColour)
	{
		ClearScreen(PIXEL_COLOUR_RED(pColour),PIXEL_COLOUR_GREEN(pColour),PIXEL_COLOUR_BLUE(pColour));
	}

	/* 
		Expects source to be 24bit, three 8 bit bytes in R G B order.
		IE pSourcePixels[0] is red, pSourcePixels[1] is green and pSourcePixels[2] is blue.
		Renders the image to pX,pY without scaling. Most basic blit.
	*/
	void BlitRGB(const uint8_t* pSourcePixels,int pX,int pY,int pSourceWidth,int pSourceHeight);
	
	/* 
		Expects source to be 24bit, three 8 bit bytes in R G B order.
		IE pSourcePixels[0] is red, pSourcePixels[1] is green and pSourcePixels[2] is blue.
		Renders the image to pX,pY from pSourceX,pSourceY in the source without scaling.
		pSourceStride is the byte size of one scan line in the source data.
		Allows sub rect render of the source image.
	*/
	void BlitRGB(const uint8_t* pSourcePixels,int pX,int pY,int pWidth,int pHeight,int pSourceX,int pSourceY,int pSourceStride);

	/**
	 * @brief Draws the entire image to the draw buffer,
	 * Expects source to be 32bit, four 8 bit bytes in R G B A order.
	 * IE pSourcePixels[0] is red, pSourcePixels[1] is green, pSourcePixels[2] is blue and pSourcePixels[3] is alpha.
	 * Renders the image to pX,pY without scaling. Most basic blit.
	 * 
	 * @param pSourcePixels 
	 * @param pX 
	 * @param pY 
	 * @param pSourceWidth 
	 * @param pSourceHeight 
	 * @param pPreMultipliedAlpha 
	 */
	void BlitRGBA(const uint8_t* pSourcePixels,int pX,int pY,int pSourceWidth,int pSourceHeight,bool pPreMultipliedAlpha = false);

	/**
	 * @brief Draws the sub rectangle of the image to the draw buffer,
	 * Expects source to be 32bit, four 8 bit bytes in R G B A order.
	 * IE pSourcePixels[0] is red, pSourcePixels[1] is green, pSourcePixels[2] is blue and pSourcePixels[3] is alpha.
	 * Renders the image to pX,pY without scaling. Most basic blit.
	 * 
	 * @param pSourcePixels 
	 * @param pX 
	 * @param pY 
	 * @param pWidth 
	 * @param pHeight 
	 * @param pSourceX 
	 * @param pSourceY 
	 * @param pSourceStride 
	 * @param pPreMultipliedAlpha 
	 */
	void BlitRGBA(const uint8_t* pSourcePixels,int pX,int pY,int pWidth,int pHeight,int pSourceX,int pSourceY,int pSourceStride,bool pPreMultipliedAlpha = false);

	/**
	 * @brief Draws a horizontal line.
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToX 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 */
	void DrawLineH(int pFromX,int pFromY,int pToX,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

	/**
	 * @brief  Draws a horizontal line.
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToX 
	 * @param pColour 
	 */
	void DrawLineH(int pFromX,int pFromY,int pToX,PixelColour pColour)
	{
		DrawLineH(pFromX,pFromY,pToX,PIXEL_COLOUR_RED(pColour),PIXEL_COLOUR_GREEN(pColour),PIXEL_COLOUR_BLUE(pColour));
	}

	/**
	 * @brief Draws a vertical line.
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToY 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 */
	void DrawLineV(int pFromX,int pFromY,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

	/**
	 * @brief Draws a vertical line.
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToY 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 */
	void DrawLineV(int pFromX,int pFromY,int pToY,PixelColour pColour)
	{
		DrawLineH(pFromX,pFromY,pToY,PIXEL_COLOUR_RED(pColour),PIXEL_COLOUR_GREEN(pColour),PIXEL_COLOUR_BLUE(pColour));
	}

	/**
	 * @brief Draws an arbitrary line.
	 * Will take a short cut if the line is horizontal or vertical.
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToX 
	 * @param pToY 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 */
	void DrawLine(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

	/**
	 * @brief Draws an arbitrary line.
	 * Will take a short cut if the line is horizontal or vertical.
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToX 
	 * @param pToY 
	 * @param pColour 
	 */
	void DrawLine(int pFromX,int pFromY,int pToX,int pToY,PixelColour pColour)
	{
		DrawLine(pFromX,pFromY,pToX,pToY,PIXEL_COLOUR_RED(pColour),PIXEL_COLOUR_GREEN(pColour),PIXEL_COLOUR_BLUE(pColour));
	}

	/**
	 * @brief Draws a circle using the Midpoint algorithm.
	 * https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
	 * 
	 * @param pCenterX 
	 * @param pCenterY 
	 * @param pRadius 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 * @param pFilled 
	 */
	void DrawCircle(int pCenterX,int pCenterY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,bool pFilled = false);

	/**
	 * @brief Draws a circle using the Midpoint algorithm.
	 * https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
	 * 
	 * @param pCenterX 
	 * @param pCenterY 
	 * @param pRadius 
	 * @param pColour 
	 * @param pFilled 
	 */
	void DrawCircle(int pCenterX,int pCenterY,int pRadius,PixelColour pColour,bool pFilled = false)
	{
		DrawCircle(pCenterX,pCenterY,pRadius,PIXEL_COLOUR_RED(pColour),PIXEL_COLOUR_GREEN(pColour),PIXEL_COLOUR_BLUE(pColour),pFilled);
	}

	/**
	 * @brief Draws a rectangle with the passed in RGB values either filled or not.
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToX 
	 * @param pToY 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 * @param pFilled 
	 */
	void DrawRectangle(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,bool pFilled = false);

	/**
	 * @brief Draws a rectangle with the passed in pixel colour either filled or not.
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToX 
	 * @param pToY 
	 * @param pColour 
	 * @param pFilled 
	 */
	void DrawRectangle(int pFromX,int pFromY,int pToX,int pToY,PixelColour pColour,bool pFilled = false)
	{
		DrawRectangle(pFromX,pFromY,pToX,pToY,PIXEL_COLOUR_RED(pColour),PIXEL_COLOUR_GREEN(pColour),PIXEL_COLOUR_BLUE(pColour),pFilled);
	}

	/**
	 * @brief Draws a rectangle with rounder corners in the passed in RGB values either filled or not.
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToX 
	 * @param pToY 
	 * @param pRadius 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 * @param pFilled 
	 */
	void DrawRoundedRectangle(int pFromX,int pFromY,int pToX,int pToY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,bool pFilled = false);

	/**
	 * @brief Draws a rectangle with rounder corners in the passed in pixel colour either filled or not.
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToX 
	 * @param pToY 
	 * @param pRadius 
	 * @param pColour 
	 * @param pFilled 
	 */
	void DrawRoundedRectangle(int pFromX,int pFromY,int pToX,int pToY,int pRadius,PixelColour pColour,bool pFilled = false)
	{
		DrawRoundedRectangle(pFromX,pFromY,pToX,pToY,pRadius,PIXEL_COLOUR_RED(pColour),PIXEL_COLOUR_GREEN(pColour),PIXEL_COLOUR_BLUE(pColour),pFilled);
	}

	/**
	 * @brief Draws a gradient using HSV colour space. Don't expect high speed, is doing a lot of math!
	 * 
	 * @param pFromX 
	 * @param pFromY 
	 * @param pToX 
	 * @param pToY 
	 * @param pFormRed 
	 * @param pFormGreen 
	 * @param pFormBlue 
	 * @param pToRed 
	 * @param pToGreen 
	 * @param pToBlue 
	 */
	void DrawGradient(int pFromX,int pFromY,int pToX,int pToY,uint8_t pFormRed,uint8_t pFormGreen,uint8_t pFormBlue,uint8_t pToRed,uint8_t pToGreen,uint8_t pToBlue);

	/**
	 * @brief Convert RGB to HSV
	 * Very handy for creating colour palettes.
	 * See:- (thanks David H)
	 * 	https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
	 * 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 * @param rH 
	 * @param rS 
	 * @param rV 
	 */
	static void RGB2HSV(uint8_t pRed,uint8_t pGreen, uint8_t pBlue,float& rH,float& rS, float& rV);

	/**
	 * @brief Convert HSV to RGB
	 * Very handy for creating colour palettes.
	 * See:- (thanks David H)
	 * 	https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
	 * 
	 * @param H 
	 * @param S 
	 * @param V 
	 * @param rRed 
	 * @param rGreen 
	 * @param rBlue 
	 */
	static void HSV2RGB(float H,float S, float V,uint8_t &rRed,uint8_t &rGreen, uint8_t &rBlue);

	/**
	 * @brief This uses RGB space as input and output but does the belending in the HSV space.
	 * This creates the best tweening of colours for palettes and graduations.
	 * 
	 * @param pFromRed 
	 * @param pFromGreen 
	 * @param pFromBlue 
	 * @param pToRed 
	 * @param pToGreen 
	 * @param pToBlue 
	 * @param rBlendTable 
	 */
	static void TweenColoursHSV(uint8_t pFromRed,uint8_t pFromGreen, uint8_t pFromBlue,uint8_t pToRed,uint8_t pToGreen, uint8_t pToBlue,uint32_t rBlendTable[256]);

	/**
	 * @brief By using the RGB space this creates an accurate reproduction of the 'alpha blend' operation.
	 * 
	 * @param pFromRed 
	 * @param pFromGreen 
	 * @param pFromBlue 
	 * @param pToRed 
	 * @param pToGreen 
	 * @param pToBlue 
	 * @param rBlendTable 
	 */
	static void TweenColoursRGB(uint8_t pFromRed,uint8_t pFromGreen, uint8_t pFromBlue,uint8_t pToRed,uint8_t pToGreen, uint8_t pToBlue,uint32_t rBlendTable[256]);

	/**
	 * @brief Makes pixels pre multiplied, sets RGB to RGB*A then inverts A.
 	 * Expects source to be 32bit, four 8 bit bytes in R G B A order.
 	 * IE pSourcePixels[0] is red, pSourcePixels[1] is green, pSourcePixels[2] is blue and pSourcePixels[3] is alpha.		
 	 * Speeds up rending when alpha is not being modified from (S*A) + (D*(1-A)) to S + (D*A)
 	 * For a simple 2D rendering system that's built for portablity that is an easy speed up.
 	 * Tiny2D goal is portablity and small code base. Not and epic SIMD / NEON / GL / DX / Volcan monster. :)
	 * 
	 * @param pRGBA 
	 * @param pPixelCount
	 */
	static void PreMultiplyAlphaChannel(uint8_t* pRGBA, int pPixelCount);

	/**
	 * @brief Makes pixels pre multiplied, sets RGB to RGB*A then inverts A.
	 * Handy wrapper.
	 * @param pRGBA 
	 */
	static void PreMultiplyAlphaChannel(std::vector<uint8_t>& pRGBA)
	{
		PreMultiplyAlphaChannel(pRGBA.data(),pRGBA.size()/4);
	}

private:
	/**
	 * @brief Construct a new Frame Buffer object
	 * @param pFile 
	 * @param pDisplayBuffer 
	 * @param pFixInfo 
	 * @param pScreenInfo 
	 * @param pVerbose 
	 */
	FrameBuffer(int pFile,uint8_t* pDisplayBuffer,struct fb_fix_screeninfo pFixInfo,struct fb_var_screeninfo pScreenInfo,bool pVerbose);

	/*
		Draws an arbitrary line.
		Using Bresenham's line algorithm
		https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
	*/
	void DrawLineBresenham(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

	/**
	 * @brief Handle ctrl + c event.
	 * 
	 * @param SigNum 
	 */
	static void CtrlHandler(int SigNum);

	const int mWidth,mHeight;

	// We always render to the frame buffer, do the conversion to the display buffer format when the image is presented.
	// This makes the drawing code a lot simpler and easier to maintain.
	const int mDrawBufferStride;		// Num bytes between each line.
	const int mDrawBufferSize;
	uint8_t* mDrawBuffer;

	const int mDisplayBufferStride;		// Num bytes between each line.
	const int mDisplayBufferPixelSize;	// The byte count of each pixel. So to move in the x by one pixel.
	const int mDisplayBufferFile;
	const int mDisplayBufferSize;
	uint8_t*  mDisplayBuffer;

	const struct fb_var_screeninfo mVariableScreenInfo;
	const bool mVerbose;

	/**
	 * @brief set to false by the ctrl + c handler.
	 */
	static bool mKeepGoing;

	/**
	 * @brief I trap ctrl + c. Because someone may also do this I record their handler and call it when mine is.
	 * You do not need to handle ctrl + c if you use the member function GetKeepGoing to keep your rendering look going.
	 */
	static sighandler_t mUsersSignalAction;

#ifdef USE_X11_EMULATION
	X11FrameBufferEmulation* mX11;
#endif
};

/**
 * @brief This class is for a fast but low quality pixel font.
 * The base size of the font is 8 by 13 pixels. Any scaling is very crude.
 * No fancy antialising here. But is very fast.
 */
class PixelFont
{
public:
	PixelFont(int pPixelSize = 1);
	~PixelFont();

	int GetCharWidth()const{return 8*mPixelSize;}
	int GetCharHeight()const{return 13*mPixelSize;}

	// These render with the passed in colour, does not change the pen colour.
	void DrawChar(FrameBuffer* pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,int pChar)const;
	void Print(FrameBuffer* pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,const char* pText)const;
	void Printf(FrameBuffer* pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,const char* pFmt,...)const;

	// These use current pen. Just a way to reduce the number of args you need to use for a property that does not change that much.
	void Print(FrameBuffer* pDest,int pX,int pY,const char* pText)const;
	void Printf(FrameBuffer* pDest,int pX,int pY,const char* pFmt,...)const;
	
	void SetPenColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);
	void SetPixelSize(int pPixelSize);

	/**
	 * @brief Turns on pixel bordering, renders one pixel size boarder set to the passed in colour.
	 * Helps text to stand out. Will cost more to render!
	 * 
	 * @param pOn 
	 * @param pRed 
	 * @param pGreen 
	 * @param pBlue 
	 */
	void SetBorder(bool pOn,uint8_t pRed = 0,uint8_t pGreen = 0,uint8_t pBlue = 0);
private:

	int mPixelSize;
	bool mBorderOn;
	struct
	{
		uint8_t r,g,b;
	}mPenColour,mBorderColour;
};

#ifdef USE_FREETYPEFONTS
/**
 * @brief This adds functionality for rendering free type fonts into the framebuffer object.
 * 
 */
class FreeTypeFont
{
public:
	FreeTypeFont(const std::string& pFontName,int pPixelHeight = 40,bool pVerbose = false);
	~FreeTypeFont();

	bool GetOK()const{return mOK;}
	int GetCharWidth()const{return 8;}
	int GetCharHeight()const{return 13;}

	// These render with the passed in colour, does not change the pen colour.
	int DrawChar(FrameBuffer* pDest,int pX,int pY,char pChar)const;
	void Print(FrameBuffer* pDest,int pX,int pY,const char* pText)const;
	void Printf(FrameBuffer* pDest,int pX,int pY,const char* pFmt,...)const;
	
	void SetPenColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);
	void SetBackgroundColour(uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

private:

	/**
	 * @brief Rebuilds the mBlended table from the pen and background colours.
	 */
	void RecomputeBlendTable();

	struct
	{// These are 32bit values to
		uint8_t r,g,b;
	// mBlended is a precomputed lookup for blend from background to pen colour to speed things up a bit.
	}mPenColour,mBackgroundColour,mBlended[256];

	const bool mVerbose;
	FT_Face mFace;
	bool mOK;

	/**
	 * @brief This is for the free type font support.
	 */
	static FT_Library mFreetype;
	static int mFreetypeRefCount;

};
#endif //#ifdef USE_FREETYPEFONTS

///////////////////////////////////////////////////////////////////////////////////////////////////////////
};//namespace tiny2d
	
#endif //TINY_2D_H
