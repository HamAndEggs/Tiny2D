
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <signal.h>

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

	FB->ClearScreen(0,0,0);

	uint8_t col[8][3] = {{0,0,0},{255,0,0},{0,255,0},{0,0,255},{255,255,255},{255,0,255},{255,255,0},{0,255,255}};
	
	while(KeepGoing)
	{
//		FB->DrawLineH(y,y,1023-y,y,0,0);
//		FB->DrawLineV(y,y,599-y,y,0,0);

		{
			int r = (rand()%50)+10;
			int x = (rand()%(FB->GetWidth()-r-r))+r;
			int y = (rand()%(FB->GetHeight()-r-r))+r;
			int c = rand()&7;

			FB->DrawCircle(x,y,r,col[c][0],col[c][1],col[c][2],(rand()&1) == 0);
		}

		{
			int FromX = rand()%FB->GetWidth();
			int FromY = rand()%FB->GetHeight();
			int ToX = rand()%FB->GetWidth();
			int ToY = rand()%FB->GetHeight();

			int c = rand()&7;
			
			FB->DrawLine(FromX,FromY,ToX,ToY,col[c][0],col[c][1],col[c][2]);
		}
	}

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
