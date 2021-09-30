
#include <iostream>
#include <cstdlib>
#include <time.h>

#include "Tiny2D.h"


int main(int argc, char *argv[])
{	
	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(tiny2d::FrameBuffer::VERBOSE_MESSAGES | tiny2d::FrameBuffer::DISPLAY_ROTATED_90);
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

		{
			int cX = (int)(70.0f + ((RT.GetWidth()-130.0f) * anim));
			const int y = RT.GetHeight() - 60;
			RT.FillCircle(cX,y,50,0,0,0);
			RT.FillCircle(cX,y,40,255,0,0);
			RT.DrawCircle(cX,y,30,0,0,255);

			Font.SetPenColour(255,255,255);
			Font.Print(RT,100,y,"This is a simple pixel font!");

		}

		{
			const int x = 20;
			const int y = 20;

			RT.FillRectangle(x,y,x+460,y+160,0,0,0);
			RT.FillRectangle(x+10,y+10,x+450,y+150,255,255,255);

			RT.FillRectangle(x+20,y+30,x+140,y+130,255,0,0);
			RT.FillRectangle(x+170,y+30,x+290,y+130,0,255,0);
			RT.FillRectangle(x+320,y+30,x+440,y+130,0,0,255);

			Font.SetPenColour(255,0,0);
			Font.Print(RT,x+20,y+90,"RED");

			Font.SetPenColour(0,255,0);
			Font.Print(RT,x+170,y+90,"GREEN");

			Font.SetPenColour(0,0,255);
			Font.Print(RT,x+320,y+90,"BLUE");
		}

		{
			const int x = (RT.GetWidth() / 2) - (522/2);
			const int y = (RT.GetHeight() / 2) - (150/2);

			RT.FillRectangle(x,y,x+552,y+150,0,0,0);
			RT.FillRectangle(x+10,y+10,x+542,y+140,255,255,255);

			for(int n = 0 ; n < 256 ; n++ )
			{
				const int i = x + 20 + (n*2);
				RT.DrawLineV(i,y+20,y+50,n,0,0);
				RT.DrawLineV(i+1,y+20,y+50,n,0,0);

				RT.DrawLineV(i,y+60,y+90,0,n,0);
				RT.DrawLineV(i+1,y+60,y+90,0,n,0);

				RT.DrawLineV(i,y+100,y+130,0,0,n);
				RT.DrawLineV(i+1,y+100,y+130,0,0,n);

			}
		}

		FB->Present(RT);
	}

	delete FB;
	return 0;
}
