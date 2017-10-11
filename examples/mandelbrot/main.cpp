#include <iostream>
#include <cstdlib>
#include <time.h>
#include <signal.h>
#include <thread>
#include <math.h>
#include <unistd.h>

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

static uint8_t RED[256];
static uint8_t GREEN[256];
static uint8_t BLUE[256];

class Mandelbrot
{
public:
	Mandelbrot():MaxItterartions(255)
	{
	}

	void Update(FBIO::FrameBuffer* pFB,int pYStart,int pYStep,float pZoom)
	{
		float fy = -1 + ((pZoom - 1.0f)*0.2f);
		const float fyInc = 2.0f / (float)pFB->GetHeight();
		const float fxInc = 3.5f / (float)pFB->GetWidth();

		fy += (fyInc*pYStart);
		for(int y = pYStart ; y < pFB->GetHeight() && KeepGoing ;y += pYStep , fy += (fyInc*pYStep))
		{
			float fx = -2.5f + ((pZoom - 1.0f)*0.385f);

			for(int x = 0 ; x < pFB->GetWidth() ;x++ , fx += fxInc)
			{
				int i =  GetIndex(fx/pZoom,fy/pZoom);
				pFB->WritePixel(x,y,RED[i],GREEN[i],BLUE[i]);
			}
		}
	}

private:


	const int MaxItterartions;

	int GetIndex(float x0,float y0)
	{
		//x0 = scaled x coordinate of pixel (scaled to lie in the Mandelbrot X scale (-2.5, 1))
		//y0 = scaled y coordinate of pixel (scaled to lie in the Mandelbrot Y scale (-1, 1))

		float x = 0;
		float y = 0;
		int iteration = 0;
		while( x*x + y*y < 4 && iteration < MaxItterartions )
		{
			float tx = x*x - y*y + x0;
			float ty = 2.0f*x*y + y0;

			x = tx;
			y = ty;
			iteration++;
		};

		return iteration;
	}

};

FBIO::FrameBuffer* FB = NULL;

Mandelbrot Cool1;
Mandelbrot Cool2;
Mandelbrot Cool3;
Mandelbrot Cool4;

void MandelbrotMain1(float pZoom)
{
	Cool1.Update(FB,1,4,pZoom);
}

void MandelbrotMain2(float pZoom)
{
	Cool2.Update(FB,2,4,pZoom);
}

void MandelbrotMain3(float pZoom)
{
	Cool3.Update(FB,3,4,pZoom);

}void MandelbrotMain4(float pZoom)
{
	Cool4.Update(FB,4,4,pZoom);
}

void Render(float pZoom)
{
	std::thread thread1 = std::thread(MandelbrotMain1,pZoom);
	std::thread thread2 = std::thread(MandelbrotMain2,pZoom);
	std::thread thread3 = std::thread(MandelbrotMain3,pZoom);
	std::thread thread4 = std::thread(MandelbrotMain4,pZoom);

	// Wait till all done.
	thread1.join();
	thread2.join();
	thread3.join();
	thread4.join();
}

int main(int argc, char *argv[])
{	
	FB = FBIO::FrameBuffer::Open(true);
	if( !FB )
		return 1;

	signal (SIGINT,CtrlHandler);

	srand(time(NULL));

	FB->ClearScreen(0,0,0);

	// Build colours.
	for( int i = 0 ; i < 256 ; i++ )
	{
		/*
		float fRed = sin((float)(i*1) / 83.0f);
		float fGrn = sin((float)(i*2) / 83.0f);
		float fBlu = sin((float)(i*4) / 83.0f);

		if( fRed < 0.0f )
			fRed = -fRed;		
		
		RED[i] = (uint8_t)(fRed * 255.0f);
		GREEN[i] = (uint8_t)(fGrn * 255.0f);
		BLUE[i] = (uint8_t)(fBlu * 255.0f);*/
		FB->HSV2RGB( (float)i * (360.0f / 255.0f) ,1.0f,1.0f - (float)i / 600.0f,RED[i],GREEN[i],BLUE[i]);		
	}

	float zoom,zoomstep;

	zoomstep = 1.0f;
	zoom = 1.0f;
	while( KeepGoing )
	{
		Render(zoom);
		// Now zoom in a bit.
		zoom += zoomstep;
		if( zoomstep < 10.0f )
			zoomstep += 0.5f;
	};

	// Stop monitor burn in...
	FB->ClearScreen(0,0,0);
	
	delete FB;
	return 0;
}