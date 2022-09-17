#include <iostream>
#include <cstdlib>
#include <vector>
#include <array>
#include <time.h>
#include <thread>
#include <math.h>
#include <chrono>
#include <unistd.h>

#include "Tiny2D.h"

static const int MaxItterartions = 1000;

static std::array<uint8_t,MaxItterartions> RED;
static std::array<uint8_t,MaxItterartions> GREEN;
static std::array<uint8_t,MaxItterartions> BLUE;
static double fyInc;
static double fxInc;
static double xMul = 0.3822701;

bool KeepGoing = true;

class Mandelbrot
{
public:
	Mandelbrot()
	{
	}

	void Update(tiny2d::DrawBuffer& RT,int pYStart,int pYStep,double pZoom)
	{
		double fy = -1 + ((pZoom - 1.0)*0.2001401);

		fy += (fyInc*pYStart);
		for(int y = pYStart ; y < RT.GetHeight() && KeepGoing ;y += pYStep , fy += (fyInc*pYStep))
		{
			double fx = -2.5 + ((pZoom - 1.0) * xMul);

			for(int x = 0 ; x < RT.GetWidth() ;x++ , fx += fxInc)
			{
				const int i = GetIndex(fx/pZoom,fy/pZoom);
				RT.WritePixel(x,y,RED[i],GREEN[i],BLUE[i]);
			}
		}
	}

private:
	int GetIndex(double x0,double y0)
	{
		//x0 = scaled x coordinate of pixel (scaled to lie in the Mandelbrot X scale (-2.5, 1))
		//y0 = scaled y coordinate of pixel (scaled to lie in the Mandelbrot Y scale (-1, 1))

		double x = 0;
		double y = 0;
		int iteration = 0;
		while( x*x + y*y < 2 && iteration < MaxItterartions )
		{
			const double tx = x*x - y*y + x0;
			const double ty = 2.0*x*y + y0;

			x = tx;
			y = ty;
			iteration++;
		};

		return iteration;
	}

};

void Render(double pZoom,tiny2d::DrawBuffer& RT)
{
	const int numThreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;

	for(int y = 0 ; y < numThreads ; y++ )
	{
		threads.emplace_back([pZoom,&RT,y,numThreads](){
			Mandelbrot Cool;
			Cool.Update(RT,y,numThreads,pZoom);
		});
	}

	// Wait till all done.
	for( auto &t : threads )
	{
		t.join();
	}
}

int main(int argc, char *argv[])
{	
	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(tiny2d::FrameBuffer::VERBOSE_MESSAGES);
	if( !FB )
		return 1;

	srand(time(NULL));

    tiny2d::DrawBuffer RT(FB);

	fyInc = 2.0 / (double)RT.GetHeight();
	fxInc = 3.5 / (double)RT.GetWidth();

	RT.Clear(0,0,0);

	// Build colours.
	for( int i = 0 ; i < MaxItterartions ; i++ )
	{
		tiny2d::HSV2RGB( (double)i * (360.0f / ((double)MaxItterartions)) ,1.0f,1.0f - (double)i / 600.0f,RED[i],GREEN[i],BLUE[i]);		
	}
	RED[MaxItterartions-1] = 0;
	GREEN[MaxItterartions-1] = 0;
	BLUE[MaxItterartions-1] = 0;

	double zoom,zoomstep;

	zoom = 0.1f;
	zoomstep = 1.0f;

	// Timer thread. Sets KeepGoing to false after a set time.
	// Score is number of loops in one minute.
	std::thread timer([]
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(5min);
		KeepGoing = false;
	});

	int loops = 0;
	while( KeepGoing && FB->GetKeepGoing() )
	{
		Render(zoom,RT);
		FB->Present(RT);
		// Now zoom in a bit.
		zoom += zoomstep;
		zoomstep *= 1.001f;
		loops++;
	};
	KeepGoing = false;
	timer.join();

	std::clog << "Benchmark score == " << loops << "\n";

	delete FB;
	return 0;
}