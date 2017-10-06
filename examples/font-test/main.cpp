
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <signal.h>
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

int main(int argc, char *argv[])
{	
	FBIO::FrameBuffer* FB = FBIO::FrameBuffer::Open(true);
	if( !FB )
		return 1;

	signal (SIGINT,CtrlHandler);

	srand(time(NULL));


	uint8_t col[8][3] = {{0,0,0},{255,0,0},{0,255,0},{0,0,255},{255,255,255},{255,0,255},{255,255,0},{0,255,255}};

	FB->ClearScreen(150,150,150);

	FBIO::Font TheFont(3);
	FBIO::Button TheButton(100,100,150,50,"This is a button");
	FBIO::Button GreenButton(200,200,150,50,"A green button");
	GreenButton.SetColour(30,150,30);

	TheFont.SetPenColour(0,0,0);

	int n = 149;
	timespec SleepTime = {0,10000000};
	while(KeepGoing)
	{
		if( ((n)>>4) != ((n-1)>>4) )
		{
			FB->DrawRectangle(9*8*5,0,19*8*5,13*5,150,150,150,true);
			TheFont.SetPixelSize(5);
			TheFont.Printf(FB,0,0,"Counting %d",n>>4);
		}

		TheFont.SetPixelSize(1);
		if( (((n-1)%150)&128) != ((n%150)&128) )// Only draw when it changes to stop flicker. This is a simple frame buffer lib. If you want offscreen buffers and high speed, use SDL.
		{
			TheButton.Render(FB,TheFont,((n%150)&128) != 0);
			GreenButton.Render(FB,TheFont,((n%150)&128) != 0);
		}
		
		n++;
		nanosleep(&SleepTime,NULL);
	};

	delete FB;
	return 0;
}
