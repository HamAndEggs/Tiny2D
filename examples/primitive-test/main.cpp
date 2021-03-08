
#include <iostream>
#include <cstdlib>
#include <time.h>

#include "Tiny2D.h"


int main(int argc, char *argv[])
{	
	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(true);
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

		{
			int FromX = rand()%FB->GetWidth();
			int FromY = rand()%FB->GetHeight();
			int ToX = rand()%FB->GetWidth();
			int ToY = rand()%FB->GetHeight();

			int c = rand()&7;
			int r = 5 + (rand()&15);

			FB->DrawRoundedRectangle(FromX,FromY,ToX,ToY,r,col[c][0],col[c][1],col[c][2],(rand()&1) == 0);
		}

		FB->DrawCircle(70,70,50,0,0,0,true);
		FB->DrawCircle(70,70,40,255,0,0,true);
		FB->DrawCircle(70,70,30,0,0,255,false);

		FB->DrawRectangle(200,20,600,220,0,0,0,true);
		FB->DrawRectangle(220,40,580,200,0,255,0,true);
		FB->DrawRectangle(230,50,570,190,255,0,255,false);

		FB->DrawRoundedRectangle(200,320,600,520,20,0,0,0,true);
		FB->DrawRoundedRectangle(220,340,580,500,20,0,255,0,true);
		FB->DrawRoundedRectangle(230,350,570,490,20,255,0,255,false);


		FB->Present();
	}

	// Stop monitor burn in...
	FB->ClearScreen(0,0,0);	

	delete FB;
	return 0;
}
