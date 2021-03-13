
#include <iostream>
#include <cstdlib>
#include <time.h>

#include "Tiny2D.h"


int main(int argc, char *argv[])
{	
	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(true);
	if( !FB )
		return 1;

	tiny2d::DrawBuffer RT(FB);
	tiny2d::PixelFont Font(3);
	Font.SetBorder(true);

	srand(time(NULL));

	RT.Clear(0,0,0);

	uint8_t col[8][3] = {{0,0,0},{255,0,0},{0,255,0},{0,0,255},{255,255,255},{255,0,255},{255,255,0},{0,255,255}};
	
	float anim = 0.0f;
	while(FB->GetKeepGoing())
	{
		anim += 0.001f;
		if( anim > 1.0f )
		{
			anim -= 1.0f;
		}

		{
			int r = (rand()%50)+10;
			int x = (rand()%(RT.GetWidth()-r-r))+r;
			int y = (rand()%(RT.GetHeight()-r-r))+r;
			int c = rand()&7;

			RT.DrawCircle(x,y,r,col[c][0],col[c][1],col[c][2],(rand()&1) == 0);
		}

		{
			int FromX = rand()%RT.GetWidth();
			int FromY = rand()%RT.GetHeight();
			int ToX = rand()%RT.GetWidth();
			int ToY = rand()%RT.GetHeight();

			int c = rand()&7;
			
			RT.DrawLine(FromX,FromY,ToX,ToY,col[c][0],col[c][1],col[c][2]);
		}

		{
			int FromX = rand()%RT.GetWidth();
			int FromY = rand()%RT.GetHeight();
			int ToX = rand()%RT.GetWidth();
			int ToY = rand()%RT.GetHeight();

			int c = rand()&7;

			if( (rand()&1) == 0 )
				RT.DrawRectangle(FromX,FromY,ToX,ToY,col[c][0],col[c][1],col[c][2]);
			else
				RT.FillRectangle(FromX,FromY,ToX,ToY,col[c][0],col[c][1],col[c][2]);
		}

		{
			int FromX = rand()%RT.GetWidth();
			int FromY = rand()%RT.GetHeight();
			int ToX = rand()%RT.GetWidth();
			int ToY = rand()%RT.GetHeight();

			int c = rand()&7;
			int r = 5 + (rand()&15);

			if( (rand()&1) == 0 )
				RT.DrawRoundedRectangle(FromX,FromY,ToX,ToY,r,col[c][0],col[c][1],col[c][2]);
			else
				RT.FillRoundedRectangle(FromX,FromY,ToX,ToY,r,col[c][0],col[c][1],col[c][2]);
		}

		RT.FillRectangle(200,20,600,220,0,0,0);
		RT.FillRectangle(220,40,580,200,0,255,0);
		RT.DrawRectangle(230,50,570,190,255,0,255);

		RT.FillRoundedRectangle(200,320,600,520,20,0,0,0);
		RT.FillRoundedRectangle(220,340,580,500,20,0,255,0);
		RT.DrawRoundedRectangle(230,350,570,490,20,255,0,255);

		{
			int cX = (int)(70.0f + ((RT.GetWidth()-130.0f) * anim));
			RT.FillCircle(cX,90,50,0,0,0);
			RT.FillCircle(cX,90,40,255,0,0);
			RT.DrawCircle(cX,90,30,0,0,255);
		}

		Font.SetPenColour(255,255,255);
		Font.Print(RT,100,100,"This is a simple pixel font!");

		RT.FillRectangle(200,200,800,400,0,0,0);
		RT.FillRectangle(220,220,780,380,255,255,255);

		RT.FillRectangle(250,250,350,350,255,0,0);
		RT.FillRectangle(450,250,550,350,0,255,0);
		RT.FillRectangle(650,250,750,350,0,0,255);

		Font.SetPenColour(255,0,0);
		Font.Print(RT,250,300,"RED");

		Font.SetPenColour(0,255,0);
		Font.Print(RT,450,300,"GREEN");

		Font.SetPenColour(0,0,255);
		Font.Print(RT,650,300,"BLUE");

		FB->Present(RT);
	}

	delete FB;
	return 0;
}
