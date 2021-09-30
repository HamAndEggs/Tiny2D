#include <iostream>
#include <cstdlib>
#include <time.h>
#include <thread>
#include <math.h>
#include <unistd.h>

#include "Tiny2D.h"

static uint8_t RED[256];
static uint8_t GREEN[256];
static uint8_t BLUE[256];

bool KeepGoing = true;

class Mandelbrot
{
public:
	Mandelbrot():MaxItterartions(255)
	{
	}

	void Update(tiny2d::DrawBuffer& RT,int pYStart,int pYStep,float pZoom)
	{
		float fy = -1 + ((pZoom - 1.0f)*0.2f);
		const float fyInc = 2.0f / (float)RT.GetHeight();
		const float fxInc = 3.5f / (float)RT.GetWidth();

		fy += (fyInc*pYStart);
		for(int y = pYStart ; y < RT.GetHeight() && KeepGoing ;y += pYStep , fy += (fyInc*pYStep))
		{
			float fx = -2.5f + ((pZoom - 1.0f)*0.385f);

			for(int x = 0 ; x < RT.GetWidth() ;x++ , fx += fxInc)
			{
				int i =  GetIndex(fx/pZoom,fy/pZoom);
				RT.WritePixel(x,y,RED[i],GREEN[i],BLUE[i]);
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

void Render(float pZoom,tiny2d::DrawBuffer& RT)
{
	std::thread thread1 = std::thread([pZoom,&RT](){
		Mandelbrot Cool;
		Cool.Update(RT,1,4,pZoom);
	});

	std::thread thread2 = std::thread([pZoom,&RT](){
		Mandelbrot Cool;
		Cool.Update(RT,2,4,pZoom);
	});

	std::thread thread3 = std::thread([pZoom,&RT](){
		Mandelbrot Cool;
		Cool.Update(RT,3,4,pZoom);
	});

	std::thread thread4 = std::thread([pZoom,&RT](){
		Mandelbrot Cool;
		Cool.Update(RT,4,4,pZoom);
	});

	// Wait till all done.
	thread1.join();
	thread2.join();
	thread3.join();
	thread4.join();
}

int main(int argc, char *argv[])
{	
	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(tiny2d::FrameBuffer::VERBOSE_MESSAGES);
	if( !FB )
		return 1;

	srand(time(NULL));

    tiny2d::DrawBuffer RT(FB);
	RT.Clear(0,0,0);

	// Build colours.
	for( int i = 0 ; i < 256 ; i++ )
	{
		tiny2d::HSV2RGB( (float)i * (360.0f / 255.0f) ,1.0f,1.0f - (float)i / 600.0f,RED[i],GREEN[i],BLUE[i]);		
	}

	float zoom,zoomstep;

	zoomstep = 1.0f;
	zoom = 1.0f;
	while( (KeepGoing = FB->GetKeepGoing()) == true )
	{
		Render(zoom,RT);
		FB->Present(RT);
		// Now zoom in a bit.
		zoom += zoomstep;
		if( zoomstep < 10.0f )
			zoomstep += 0.5f;
	};

	delete FB;
	return 0;
}