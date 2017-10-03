
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

	FBIO::Font TheFont;
	FBIO::Button TheButton(100,100,150,50,"This is a button");
	FBIO::Button GreenButton(200,200,150,50,"A green button");
	GreenButton.SetColour(30,150,30);

	TheFont.SetPenColour(0,0,0);

	int n = 149;
	timespec SleepTime = {0,10000000};
	while(KeepGoing)
	{
		FB->DrawRectangle(0,0,300,13,150,150,150,true);
		TheFont.Printf(FB,0,0,"Counting %d",n++);

		if( (((n-1)%150)&128) != ((n%150)&128) )// Only draw when it changes to stop flicker. Later I'll implement an offscreen buffer.
		{
			TheButton.Render(FB,((n%150)&128) != 0);
			GreenButton.Render(FB,((n%150)&128) != 0);
		}
		

		nanosleep(&SleepTime,NULL);
	};
	

/*
	const FBIO::Font *small = FBIO::Font::AllocateSmallFont();
	const FBIO::Font *big = FBIO::Font::AllocateBigFont();

	small->Print(FB,100,100,"This is a test");
	big->Print(FB,100,120,"This is a test");

	delete small;
	delete big;*/

	delete FB;
	return 0;
}
