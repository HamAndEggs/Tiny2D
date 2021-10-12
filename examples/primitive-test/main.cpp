
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <math.h>

#include "Tiny2D.h"

constexpr float GetPI()
{
	return std::acos(-1);
}

constexpr float DegreeToRadian(float pDegree)
{
	return pDegree * (GetPI()/180.0f);
}

int main(int argc, char *argv[])
{	
	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(tiny2d::FrameBuffer::VERBOSE_MESSAGES | tiny2d::FrameBuffer::ROTATE_FRAME_LANDSCAPE);
	if( !FB )
		return 1;

	tiny2d::DrawBuffer RT(FB);
	tiny2d::PixelFont Font(3);
	Font.SetBorder(true);

	srand(time(NULL));

	RT.Clear(0,0,0);

	uint8_t col[8][3] = {{0,0,0},{255,0,0},{0,255,0},{0,0,255},{255,255,255},{255,0,255},{255,255,0},{0,255,255}};

	auto GetX = [RT]()
	{
		return rand()%(RT.GetWidth());
	};

	auto GetY = [RT]()
	{
		return rand()%(RT.GetHeight());
	};

	auto GetToX = [RT,GetX]()
	{
		return (RT.GetHeight()/3) + ((GetX()/3)*2);
	};

	auto GetToY = [RT,GetY]()
	{
		return (RT.GetHeight()/3) + ((GetY()/3)*2);
	};

	float anim = 0.0f;
	float rot = 0.0f;
	while(FB->GetKeepGoing())
	{
		rot += 0.01f;
		if( rot >= 360.0f )
		{
			rot -= 360.0f;
		}

		anim += 0.001f;
		if( anim > 1.0f )
		{
			anim -= 1.0f;
		}

		{
			int r = (rand()%50)+10;
			int x = (GetX()-r-r)+r;
			int y = (GetY()-r-r)+r;
			int c = rand()&7;

			RT.DrawCircle(x,y,r,col[c][0],col[c][1],col[c][2],(rand()&1) == 0);
		}

		{
			int FromX = GetX();
			int FromY = GetY();
			int ToX = GetToX();
			int ToY = GetToY();

			int c = rand()&7;
			int w = 1 + (rand()&7);
			
			RT.DrawLine(FromX,FromY,ToX,ToY,w,col[c][0],col[c][1],col[c][2]);
		}

		if( (rand()%15) == 0 )
		{
			int FromX = GetX();
			int FromY = GetY();
			int ToX = GetToX();
			int ToY = GetToY();

			int c = rand()&7;

			if( (rand()&1) == 0 )
				RT.DrawRectangle(FromX,FromY,ToX,ToY,col[c][0],col[c][1],col[c][2]);
			else
				RT.FillRectangle(FromX,FromY,ToX,ToY,col[c][0],col[c][1],col[c][2]);
		}

		if( (rand()%15) == 0 )
		{
			int FromX = GetX();
			int FromY = GetY();
			int ToX = GetToX();
			int ToY = GetToY();

			int c = rand()&7;
			int r = 5 + (rand()&15);

			if( (rand()&1) == 0 )
				RT.DrawRoundedRectangle(FromX,FromY,ToX,ToY,r,col[c][0],col[c][1],col[c][2]);
			else
				RT.FillRoundedRectangle(FromX,FromY,ToX,ToY,r,col[c][0],col[c][1],col[c][2]);
		}

		// Have a look to see what thick line looks like when joined up...
		{
			const int cx = (RT.GetWidth()/4)*3;
			const int cy = (RT.GetHeight()/8)*3;

			const int s = 110;
			RT.FillRoundedRectangle(cx-s,cy-s,cx+s,cy+s,17,90,100,110);
			RT.DrawRoundedRectangle(cx-s,cy-s,cx+s,cy+s,17,0,0,0);

			for( int z = 0 ; z < 2 ; z++ )
			{
				float angle = rot;
				const float angleStep = DegreeToRadian(360.0f) / 6.0f;

				int lx = cx + ((int)(std::cos(angle) * 100.0f));
				int ly = cy + ((int)(std::sin(angle) * 100.0f));

				angle += angleStep;
				for( int n = 0 ; n < 6 ; n++, angle += angleStep )
				{
					const int x = cx + ((int)(std::cos(angle) * 100.0f));
					const int y = cy + ((int)(std::sin(angle) * 100.0f));

					if( z )
						RT.DrawLine(lx,ly,x,y,5,255,255,255);
					else
						RT.DrawLine(lx,ly,x,y,9,0,0,0);
					lx = x;
					ly = y;
				}
			}

			{
				float angle = rot;
				const float angleStep = DegreeToRadian(180.0f);

				int sx = cx + ((int)(std::cos(angle) * 70.0f));
				int sy = cy + ((int)(std::sin(angle) * 70.0f));

				angle += angleStep;

				int ex = cx + ((int)(std::cos(angle) * 70.0f));
				int ey = cy + ((int)(std::sin(angle) * 70.0f));

				RT.DrawLine(sx,sy,ex,ey,11,0,0,0);
				RT.DrawLine(sx,sy,ex,ey,5,255,255,255);

			}

		}

		// Test the fonts
		{
			int cX = (int)(70.0f + ((RT.GetWidth()-130.0f) * anim));
			const int y = RT.GetHeight() - 60;
			RT.FillCircle(cX,y,50,0,0,0);
			RT.FillCircle(cX,y,40,255,0,0);
			RT.DrawCircle(cX,y,30,0,0,255);

			Font.SetPenColour(255,255,255);
			Font.Print(RT,100,y,"This is a simple pixel font!");

		}

		// Draws a test pattern so we can check colour output.
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

		// Draws some gradients, test colour output.
		{
			const int x = 20;
			const int y = (RT.GetHeight() / 2) + 80;

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
