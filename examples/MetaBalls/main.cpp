#include <iostream>
#include <cstdlib>
#include <time.h>
#include <signal.h>
#include <thread>
#include <math.h>
#include <unistd.h>
#include <vector>

#include "framebuffer.h"

bool KeepGoing = true;

void static CtrlHandler(int SigNum)
{
	static int numTimesAskedToExit = 0;
	std::cout << std::endl << "Asked to quit, please wait" << std::endl;
	if( numTimesAskedToExit > 2 )
	{
		std::cout << "Asked to quit to many times, forcing exit in bad way" << std::endl;
		exit(1);
	}
	KeepGoing = false;
}

class Ball
{
public:
	Ball(int pWidth,int pHeight,int pRadius):radius((float)pRadius)
	{
		x = rand()%pWidth;
		y = rand()%pHeight;

		dx = GetNewDelta();
		dy = GetNewDelta();

		if( rand()&1 )
			dx = -dx;

		if( rand()&1 )
			dy = -dy;

		
	}

	void Update(int pWidth,int pHeight)
	{
		x += dx;
		y += dy;

		if( x < 0 )
		{
			x = 0;
			dx = GetNewDelta();
		}
		else if( x >= pWidth )
		{
			x = pWidth - 1;
			dx = -GetNewDelta();
		}

		if( y < 0 )
		{
			y = 0;
			dy = GetNewDelta();
		}
		else if( y >= pHeight )
		{
			y = pHeight - 1;
			dy = -GetNewDelta();
		}
	}

	int GetX()const{return x;}
	int GetY()const{return y;}
	
	float GetMeta(int pX,int pY)const
	{
		if( pX != x || pY != y )// Prevent div by zero.
		{
			const float diffX = (pX - x);
			const float diffY = (pY - y);
			return radius / (((diffX * diffX) + (diffY * diffY)));
		}

		return radius;
	}
	
private:
	float GetNewDelta()const
	{
		const float d = (float) (16 + (rand()&31));
		return d / 50.0f;
	}

	float dx,dy;
	float x,y;
	const float radius;
};

static const int PixelSize = 4;
static uint8_t RED[256];
static uint8_t GREEN[256];
static uint8_t BLUE[256];

void RenderScanLine(FBIO::FrameBuffer* FB,int pFromY,int pToY,const std::vector<Ball>& pBalls)
{
	const int Width = FB->GetWidth();

	for( int y = pFromY ; y < pToY ; y+=PixelSize )
	{
		for( int x = 0 ; x < Width ; x+=PixelSize )
		{
			float TotalDist = 0; 
			for( auto &ball : pBalls )
				TotalDist += ball.GetMeta(x,y);

			uint8_t c = (uint8_t)(std::min(255,(int)(TotalDist*3000)));
			FB->DrawRectangle(x,y,x+PixelSize,y+PixelSize,RED[c],GREEN[c],BLUE[c],true);
		}
	}
}

//void RenderFrame(

int main(int argc, char *argv[])
{	
	FBIO::FrameBuffer* FB = FBIO::FrameBuffer::Open(true);
	if( !FB )
		return 1;

	signal (SIGINT,CtrlHandler);

	srand(time(NULL));

	FB->ClearScreen(0,0,0);

	// Build colours.
	for( int i = 0 ; i < 256 ; i++ )
	{
		FB->HSV2RGB( (float)i * (360.0f / 255.0f) ,1.0f,1.0f - (float)i / 600.0f,RED[i],GREEN[i],BLUE[i]);
	}

	const int Width = FB->GetWidth();
	const int Height = FB->GetHeight();

	// Make the balls.
	std::vector<Ball> TheBalls;

	for(int n = 0 ; n < 15 ; n++ )
		TheBalls.emplace_back(Width,Height,160 + (rand()&127));

	const int HeightSplit = Height/4;

	while( KeepGoing )
	{
		for( auto &ball : TheBalls )
			ball.Update(Width,Height);
		
//		for( int y = 0 ; y < Height ; y+=PixelSize )
		int y = 0;

		std::thread thread1 = std::thread(RenderScanLine,FB,y,y+HeightSplit,TheBalls);
		y += HeightSplit;

		std::thread thread2 = std::thread(RenderScanLine,FB,y,y+HeightSplit,TheBalls);
		y += HeightSplit;

		std::thread thread3 = std::thread(RenderScanLine,FB,y,y+HeightSplit,TheBalls);
		y += HeightSplit;

		std::thread thread4 = std::thread(RenderScanLine,FB,y,y+HeightSplit,TheBalls);

		// Wait till all done.
		thread1.join();
		thread2.join();
		thread3.join();
		thread4.join();
	};

	// Stop monitor burn in...
	FB->ClearScreen(0,0,0);

	delete FB;
	return 0;
}