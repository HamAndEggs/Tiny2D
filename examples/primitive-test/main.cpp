
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
			int x = (rand()%(RT.mWidth-r-r))+r;
			int y = (rand()%(RT.mHeight-r-r))+r;
			int c = rand()&7;

			RT.DrawCircle(x,y,r,col[c][0],col[c][1],col[c][2],(rand()&1) == 0);
		}

		{
			int FromX = rand()%RT.mWidth;
			int FromY = rand()%RT.mHeight;
			int ToX = rand()%RT.mWidth;
			int ToY = rand()%RT.mHeight;

			int c = rand()&7;
			
			RT.DrawLine(FromX,FromY,ToX,ToY,col[c][0],col[c][1],col[c][2]);
		}

		{
			int FromX = rand()%RT.mWidth;
			int FromY = rand()%RT.mHeight;
			int ToX = rand()%RT.mWidth;
			int ToY = rand()%RT.mHeight;

			int c = rand()&7;

			RT.DrawRectangle(FromX,FromY,ToX,ToY,col[c][0],col[c][1],col[c][2],(rand()&1) == 0);
		}

		{
			int FromX = rand()%RT.mWidth;
			int FromY = rand()%RT.mHeight;
			int ToX = rand()%RT.mWidth;
			int ToY = rand()%RT.mHeight;

			int c = rand()&7;
			int r = 5 + (rand()&15);

			RT.DrawRoundedRectangle(FromX,FromY,ToX,ToY,r,col[c][0],col[c][1],col[c][2],(rand()&1) == 0);
		}

		RT.DrawRectangle(200,20,600,220,0,0,0,true);
		RT.DrawRectangle(220,40,580,200,0,255,0,true);
		RT.DrawRectangle(230,50,570,190,255,0,255,false);

		RT.DrawRoundedRectangle(200,320,600,520,20,0,0,0,true);
		RT.DrawRoundedRectangle(220,340,580,500,20,0,255,0,true);
		RT.DrawRoundedRectangle(230,350,570,490,20,255,0,255,false);

		{
			int cX = (int)(70.0f + ((RT.mWidth-130.0f) * anim));
			RT.DrawCircle(cX,90,50,0,0,0,true);
			RT.DrawCircle(cX,90,40,255,0,0,true);
			RT.DrawCircle(cX,90,30,0,0,255,false);
		}

		Font.Print(RT,100,100,"This is a simple pixel font!");

		FB->Present(RT);
	}

	delete FB;
	return 0;
}
