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

#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

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

namespace FBIO{	// Using a namespace to try to prevent name clashes as my class name is kind of obvious. :)
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
	 * If enabled, double buffering will reduce the amount of tearing when redrawing the display at the cost of a little speed.
	 * 
	 * @param pDoubleBuffer If true all drawing will be offscreen and you will need to call Present to see the results.
	 * @param pVerbose get debugging information as the object is created.
	 * @return FrameBuffer* 
	 */
	static FrameBuffer* Open(bool pDoubleBuffer = false,bool pVerbose = false);

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

private:
	FrameBuffer(int pFile,uint8_t* pFrameBuffer,uint8_t* pDisplayBuffer,struct fb_fix_screeninfo pFixInfo,struct fb_var_screeninfo pScreenInfo,bool pVerbose);

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
	const int mStride;// Num bytes between each line.
	const int mPixelSize;	// The byte count of each pixel. So to move in the x by one pixel.
	const int mFrameBufferFile;
	const int mFrameBufferSize;
	const bool mVerbose;
	const struct fb_var_screeninfo mVariableScreenInfo;

	/**
	 * @brief If double buffer is on then mFrameBuffer is the buffer that is rendered too and display buffer is what is on view.
	 * If no double buffering, these point to the same memory.
	 */
	uint8_t* mFrameBuffer;
	uint8_t* mDisplayBuffer;

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
private:

	int mPixelSize;

	struct
	{
		uint8_t r,g,b;
	}mPenColour;
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
};//namespace FBIO
	
#endif //FRAME_BUFFER_H
