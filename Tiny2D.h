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
#include <string>
#include <functional>

#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>

#include <linux/fb.h>

/**
 * @brief define USE_X11_EMULATION for a system running X11.
 * This codebase is targeting systems without X11, but sometimes we want to develop on a system with it.
 * This define allows that. But is expected to be used ONLY for development.
 * To set window draw size define X11_EMULATION_WIDTH and X11_EMULATION_HEIGHT in your build settings.
 * These below are here for default behaviour.
 * Doing this saves on params that are not needed 99% of the time.
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


/**
 * @brief Convert RGB to HSV
 * Very handy for creating colour palettes.
 * See:- (thanks David H)
 * 	https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
 */
extern void RGB2HSV(uint8_t pRed,uint8_t pGreen, uint8_t pBlue,float& rH,float& rS, float& rV);

/**
 * @brief Convert HSV to RGB
 * Very handy for creating colour palettes.
 * See:- (thanks David H)
 * 	https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
 */
extern void HSV2RGB(float H,float S, float V,uint8_t &rRed,uint8_t &rGreen, uint8_t &rBlue);

/**
 * @brief This uses RGB space as input and output but does the belending in the HSV space.
 * This creates the best tweening of colours for palettes and graduations.
 */
extern void TweenColoursHSV(uint8_t pFromRed,uint8_t pFromGreen, uint8_t pFromBlue,uint8_t pToRed,uint8_t pToGreen, uint8_t pToBlue,uint8_t rBlendTable[256][3]);

/**
 * @brief By using the RGB space this creates an accurate reproduction of the 'alpha blend' operation.
 */
extern void TweenColoursRGB(uint8_t pFromRed,uint8_t pFromGreen, uint8_t pFromBlue,uint8_t pToRed,uint8_t pToGreen, uint8_t pToBlue,uint8_t rBlendTable[256][3]);
	
///////////////////////////////////////////////////////////////////////////////////////////////////////////
class FrameBuffer;

// This define allows me to play with the colour order of the offscreen buffer without having to keep search the source.
// This is only to do with the format of the data in the buffer. Not RGB buffers passed in. These are always r[0],g[1]],b[2].
// Linux FB seems to be the wrong way around. BGR
#define RED_PIXEL_INDEX 	2
#define GREEN_PIXEL_INDEX 	1
#define BLUE_PIXEL_INDEX	0
#define ALPHA_PIXEL_INDEX	3

#define WRITE_RGB_TO_PIXEL(PIXEL_BUFFER,RED_VALUE,GREEN_VALUE,BLUE_VALUE)	\
{																			\
	PIXEL_BUFFER[ RED_PIXEL_INDEX ] = RED_VALUE;							\
	PIXEL_BUFFER[ GREEN_PIXEL_INDEX ] = GREEN_VALUE;						\
	PIXEL_BUFFER[ BLUE_PIXEL_INDEX ] = BLUE_VALUE;							\
}

/**
 * @brief Checks that the address passed, with pixel width, if written to, will not overlow the buffer.
 * has to be a define so that you get told where the error is.
 * @param pPixel 
 */
#ifdef	NDEBUG
	#define AssertPixelIsInBuffer(pPixel)	(__ASSERT_VOID_CAST (0))
#else
	#define AssertPixelIsInBuffer(pPixel)	{ assert( pPixel >= mPixels.data() ); assert( pPixel <= mPixels.data() + mPixels.size() - mPixelSize ); }
#endif

/**
 * @brief This is the main off screen drawing / image buffer.
 * This can be used to simply hold an image as well as creating new images from primitive calls.
 * This images can then be presented to the display buffer for viewing by the user.
 * This is the object that most of your interations will be with.
 */
class DrawBuffer
{
public:
	// For simplicity and flexibility all visable and modifiable. So don't be daft. :) 
	std::vector<uint8_t> mPixels;

	/**
	 * @brief Construct a new Tiny Image object
	 */
	DrawBuffer(int pWidth, int pHeight, size_t pPixelSize,bool pHasAlpha = false,bool pPreMultipliedAlpha = false);

	/**
	 * @brief Construct a new Tiny Image object assumes stride is width * height * 3 or 4 bytes based on alpha.
	 */
	DrawBuffer(int pWidth, int pHeight,bool pHasAlpha = false,bool pPreMultipliedAlpha = false);

	/**
	 * @brief Construct a draw buffer that is suitable for use as a render target.
	 * This makes it the same size as dest so in present we can do a memcpy. Gives us a four fold speed up.
	 * For most chips, don't matter. But when you get to low speed, 40Mhz chips, every little helps.
	 * This is about the only optimisation I will do. I expect people to use this by creating offscreen buffers
	 * that only get updated when something changes and composite the changes together at end of frame.
	 * So most of the time all the rendering is done is to make sure the display has been updated. Just incase TTY had put something up.
	 * @param pFB 
	 */
	DrawBuffer(const FrameBuffer* pFB);

	/**
	 * @brief Default empty constructor
	 */
	DrawBuffer();

	inline int GetWidth()const{return mWidth;}
	inline int GetHeight()const{return mHeight;}
	inline size_t GetPixelSize()const{return mPixelSize;}
	inline size_t GetStride()const{return mStride;}

	/**
	 * @brief Get the index of the first byte of the pixel at x,y.
	 */
	inline size_t GetPixelIndex(int pX,int pY)const{return (pX * mPixelSize) + (pY * mStride);}

	/**
	 * @brief Resets the image into a new different size / format.
	 * Expect image pixels to vanish after calling. If they don't, it's luck!
	 * Does NOT scale the image!
	 */
	void Resize(int pWidth, int pHeight,size_t pPixelSize,bool pHasAlpha = false,bool pPreMultipliedAlpha = false);
	void Resize(int pWidth, int pHeight,bool pHasAlpha = false,bool pPreMultipliedAlpha = false)
	{
		Resize(pWidth,pHeight,pHasAlpha?4:3,pHasAlpha,pPreMultipliedAlpha);
	}

	/**
	 * @brief Writes a single pixel with the passed red, green and blue values. 0 -> 255, 0 being off 255 being full on.
	 * The pixel will not be written if it's outside the frame buffers bounds.
	 * pAlpha is ignored if destination has no alpha channel.
	 */
	inline void WritePixel(int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha = 255)
	{
		if( pX >= 0 && pX < mWidth && pY >= 0 && pY < mHeight )
		{
			// When optimized by compiler these const vars will
			// all move to generate the same code as if I made it all one line and unreadable!
			// Trust your optimizer. :)
			const size_t index = GetPixelIndex(pX,pY);
			uint8_t* dst = mPixels.data() + index;

			AssertPixelIsInBuffer(dst);

			WRITE_RGB_TO_PIXEL(dst,pRed,pGreen,pBlue);

			if( mHasAlpha )
			{
				assert( index + 3 < mPixels.size() );
				dst[ 3 ] = pAlpha;
			}
		}
	}

	/**
	 * @brief Blends a single pixel with the frame buffer. does (S*A) + (D*(1-A))
	 * The pixel will not be written if it's outside the frame buffers bounds.
	 * 
	 * @param pRGBA Four bytes, pRGBA[0] == red, pRGBA[1] == green, pRGBA[2] == blue, pRGBA[3] == alpha
	 */
	void BlendPixel(int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha);
	void BlendPixel(int pX,int pY,const uint8_t* pRGBA)
	{
		BlendPixel(pX,pY,pRGBA[0],pRGBA[1],pRGBA[2],pRGBA[3]);
	}

	/**
	 * @brief Blends a single pixel with the frame buffer. does S + (D * A) Quicker but less flexable.
	 * The pixel will not be written if it's outside the frame buffers bounds.
	 * 
	 * @param pRGBA Four bytes, pRGBA[0] == red, pRGBA[1] == green, pRGBA[2] == blue, pRGBA[3] == alpha
	 */
	void BlendPreAlphaPixel(int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha);
	void BlendPreAlphaPixel(int pX,int pY,const uint8_t* pRGBA)
	{
		BlendPreAlphaPixel(pX,pY,pRGBA[0],pRGBA[1],pRGBA[2],pRGBA[3]);
	}
	
	/**
	 * @brief Clears the entire screen to the passed colour.
	 * Alpha ignored it dest has no alpha channel
	 */
	void Clear(uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha = 255);

	/**
	 * @brief sets all the pixels to one colour.
	 * Does one fill with memset!
	 */
	void Clear(uint8_t pValue = 0);

	/**
	 * @briefExpects source to be 24bit, three 8 bit bytes in R G B order.
	 * IE pSourcePixels[0] is red, pSourcePixels[1] is green and pSourcePixels[2] is blue.
	 * Renders the image to pX,pY without scaling. Most basic blit.
	*/
	void BlitRGB(const uint8_t* pSourcePixels,int pX,int pY,int pSourceWidth,int pSourceHeight);

	/**
	 * @brief Expects source to be 24bit, three 8 bit bytes in R G B order.
	 * IE pSourcePixels[0] is red, pSourcePixels[1] is green and pSourcePixels[2] is blue.
	 * Renders the image to pX,pY from pSourceX,pSourceY in the source without scaling.
	 * pSourceStride is the byte size of one scan line in the source data.
	 * Allows sub rect render of the source image.
	 */
	void BlitRGB(const uint8_t* pSourcePixels,int pX,int pY,int pWidth,int pHeight,int pSourceX,int pSourceY,int pSourceStride);

	/**
	 * @brief Draws the entire image to the draw buffer,
	 * Expects source to be 32bit, four 8 bit bytes in R G B A order.
	 * IE pSourcePixels[0] is red, pSourcePixels[1] is green, pSourcePixels[2] is blue and pSourcePixels[3] is alpha.
	 * Renders the image to pX,pY without scaling. Most basic blit.
	 */
	void BlitRGBA(const uint8_t* pSourcePixels,int pX,int pY,int pSourceWidth,int pSourceHeight,bool pPreMultipliedAlpha = false);

	/**
	 * @brief Draws the sub rectangle of the image to the draw buffer,
	 * Expects source to be 32bit, four 8 bit bytes in R G B A order.
	 * IE pSourcePixels[0] is red, pSourcePixels[1] is green, pSourcePixels[2] is blue and pSourcePixels[3] is alpha.
	 * Renders the image to pX,pY without scaling. Most basic blit.
	 */
	void BlitRGBA(const uint8_t* pSourcePixels,int pX,int pY,int pWidth,int pHeight,int pSourceX,int pSourceY,int pSourceStride,bool pPreMultipliedAlpha = false);

	/**
	 * @brief Draws the entire image to the draw buffer.
	 * Does a pixel for pixel copy, no alpha blending.
	 */
	void Blit(const DrawBuffer& pImage,int pX,int pY);

	/**
	 * @brief Draws the entire image to the draw buffer, does alpha blending if source has alpha.
	 * Does a pixel for pixel copy, no alpha blending.
	 */
	void Blend(const DrawBuffer& pImage,int pX,int pY);

	/**
	 * @brief Draws a horizontal line.
	 */
	void DrawLineH(int pFromX,int pFromY,int pToX,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha = 255);

	/**
	 * @brief Draws a vertical line.
	 */
	void DrawLineV(int pFromX,int pFromY,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha = 255);

	/**
	 * @brief Draws an arbitrary line.
	 * Will take a short cut if the line is horizontal or vertical.
	 */
	void DrawLine(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);

	/**
	 * @brief Draws a circle using the Midpoint algorithm.
	 * https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
	 */
	void DrawCircle(int pCenterX,int pCenterY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha = 255);
	void FillCircle(int pCenterX,int pCenterY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha = 255);

	/**
	 * @brief Draws a rectangle with the passed in RGB values either filled or not.
	 */
	void DrawRectangle(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha = 255);
	void FillRectangle(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha = 255);

	/**
	 * @brief Draws a rectangle with rounder corners in the passed in RGB values either filled or not.
	 */
	void DrawRoundedRectangle(int pFromX,int pFromY,int pToX,int pToY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha = 255);
	void FillRoundedRectangle(int pFromX,int pFromY,int pToX,int pToY,int pRadius,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,uint8_t pAlpha = 255);

	/**
	 * @brief Fills a rect tangle based on count * size starting at pos with the two passed colours
	 * So if count is 8 and size is 16 pixel width will be 128
	 * @param pRGBA Two Four byte arrays for RGBA, A ignored if dest has no alpha channel. Done like this else we'll have 8 extra params.
	 * [0][0] == red, [0][1] == green, [0][2] == blue, [0][3] == alpha, 
	 */
	void FillCheckerBoard(int pX,int pY,int pXCount,int pYCount,int pXSize,int pYSize,const uint8_t pRGBA[2][4]);
	void FillCheckerBoard(int pX,int pY,int pXCount,int pYCount,int pXSize,int pYSize,uint8_t pA,uint8_t pB);

	/**
	 * @brief Fills the entire buffer with the checker board with each cells size being defined by size.
	 */
	void FillCheckerBoard(int pXSize,int pYSize,const uint8_t pRGBA[2][4]);
	void FillCheckerBoard(int pXSize,int pYSize,uint8_t pA,uint8_t pB);

	/**
	 * @brief Draws a gradient using HSV colour space. Don't expect high speed, is doing a lot of math!
	 */
	void DrawGradient(int pFromX,int pFromY,int pToX,int pToY,uint8_t pFormRed,uint8_t pFormGreen,uint8_t pFormBlue,uint8_t pToRed,uint8_t pToGreen,uint8_t pToBlue);

	/**
	 * @brief Shifts the pixels in the X and Y direction by their magnitude. The area that is uncovered by this shit is filled with the values passed in.
	 * This is used to easily create data plots. You just scroll then add the new pixel.
	 * Means you don't have to keep redrawing the entire graph.
	 */
	void ScrollBuffer(int pXDirection,int pYDirection,int8_t pRedFill = 0,uint8_t pGreenFill = 0,uint8_t pBlueFill = 0,uint8_t pAlphaFill = 255);

	/**
	 * @brief Makes the pixels pre multiplied, sets RGB to RGB*A then inverts A.
 	 * Speeds up rending when alpha is not being modified from (S*A) + (D*(1-A)) to S + (D*A)
 	 * For a simple 2D rendering system that's built for portablity that is an easy speed up.
 	 * Tiny2D goal is portablity and small code base. Not and epic SIMD / NEON / GL / DX / Volcan monster. :)
	 */
	void PreMultiplyAlpha();

private:
	int mWidth;
	int mHeight;
	size_t mPixelSize;	//!< The number of bytes per pixel.
	size_t mStride;	//!< The number of bytes per scan line.
	size_t mLastlineOffset; //!< The first pixel of the last line.
	bool mHasAlpha;
	bool mPreMultipliedAlpha;

	/*
		Draws an arbitrary line.
		Using Bresenham's line algorithm
		https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
	*/
	void DrawLineBresenham(int pFromX,int pFromY,int pToX,int pToY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
struct X11FrameBufferEmulation;
/**
 * @brief Represents the linux frame buffer display.
 * Is able to deal with and abstract out the various pixel formats. 
 * For a simple 2D rendering system that's built for portablity that is an easy speed up.
 * Tiny2D goal is portablity and small code base. Not and epic SIMD / NEON / GL / DX / Volcan monster. :)
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

	int GetPixelSize()const{return mDisplayBufferPixelSize;}
	int GetStride()const{return mDisplayBufferStride;}

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
	void Present(const DrawBuffer& pImage);

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

	/**
	 * @brief Handle ctrl + c event.
	 * 
	 * @param SigNum 
	 */
	static void CtrlHandler(int SigNum);

	const int mWidth,mHeight;

	const size_t mDisplayBufferStride;	// Num bytes between each line.
	const size_t mDisplayBufferPixelSize;	// The byte count of each pixel. So to move in the x by one pixel.
	const size_t mDisplayBufferSize;
	const int mDisplayBufferFile;
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
	void DrawChar(DrawBuffer& pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,int pChar)const;
	void Print(DrawBuffer& pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,const char* pText)const;
	void Printf(DrawBuffer& pDest,int pX,int pY,uint8_t pRed,uint8_t pGreen,uint8_t pBlue,const char* pFmt,...)const;

	// These use current pen. Just a way to reduce the number of args you need to use for a property that does not change that much.
	void Print(DrawBuffer& pDest,int pX,int pY,const char* pText)const;
	void Printf(DrawBuffer& pDest,int pX,int pY,const char* pFmt,...)const;
	
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
	int DrawChar(DrawBuffer& pDest,int pX,int pY,char pChar)const;
	void Print(DrawBuffer& pDest,int pX,int pY,const char* pText)const;
	void Printf(DrawBuffer& pDest,int pX,int pY,const char* pFmt,...)const;
	
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

class MillisecondTicker
{
public:
	MillisecondTicker() = default;
    MillisecondTicker(int pMilliseconds);

	/**
	 * @brief Sets the new timeout interval, resets internal counter.
	 * 
	 * @param pMilliseconds 
	 */
	void SetTimeout(int pMilliseconds);

	/**
	 * @brief Returns true if trigger ticks is less than now
	 */
    bool Tick();
    bool Tick(const clock_t pNow);

	/**
	 * @brief Calls the function if trigger ticks is less than now. 
	 */
    void Tick(std::function<void()> pCallback);
    void Tick(const clock_t pNow,std::function<void()> pCallback );

private:
    clock_t mTimeout;
    clock_t mTrigger;

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
};//namespace tiny2d
	
#endif //TINY_2D_H
