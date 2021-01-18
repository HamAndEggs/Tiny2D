
#include <iostream>
#include <cstdlib>
#include <time.h>

#include "framebuffer.h"


int main(int argc, char *argv[])
{	
	FBIO::FrameBuffer* FB = FBIO::FrameBuffer::Open(true);
	if( !FB )
		return 1;

	srand(time(NULL));

	FB->ClearScreen(0,0,0);

	uint8_t col[8][3] = {{0,0,0},{255,0,0},{0,255,0},{0,0,255},{255,255,255},{255,0,255},{255,255,0},{0,255,255}};
	
	while(FB->GetKeepGoing())
	{
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

		{
			int FromX = rand()%FB->GetWidth();
			int FromY = rand()%FB->GetHeight();
			int ToX = rand()%FB->GetWidth();
			int ToY = rand()%FB->GetHeight();

			int c = rand()&7;

			FB->DrawRectangle(FromX,FromY,ToX,ToY,col[c][0],col[c][1],col[c][2],(rand()&1) == 0);
		}

		FB->Present();
	}

	// Stop monitor burn in...
	FB->ClearScreen(0,0,0);	

	delete FB;
	return 0;
}
