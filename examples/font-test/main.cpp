
#include <iostream>
#include <string.h>

#include "Tiny2D.h"

int main(int argc, char *argv[])
{	
    std::cout << "Application Version " << APP_VERSION << '\n';
    std::cout << "Build date and time " << APP_BUILD_DATE_TIME << '\n';
    std::cout << "Build date " << APP_BUILD_DATE << '\n';
    std::cout << "Build time " << APP_BUILD_TIME << '\n';

	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(true);
	if( !FB )
		return 1;

    tiny2d::DrawBuffer RT(FB);
	RT.Clear(150,150,150);

	srand(time(NULL));

	uint8_t col[8][3] = {{0,0,0},{255,0,0},{0,255,0},{0,0,255},{255,255,255},{255,0,255},{255,255,0},{0,255,255}};

	tiny2d::PixelFont TheFont;
	TheFont.SetPenColour(0,0,0);
	TheFont.SetBorder(true);

	int n = 149;
	timespec SleepTime = {0,10000000};
	while(FB->GetKeepGoing())
	{
		if( ((n)>>4) != ((n-1)>>4) )
		{
			RT.FillRectangle(9*8*5,0,19*8*5,13*5,150,150,150);
			TheFont.SetBorder(false);
			TheFont.SetPixelSize(5);
			TheFont.Printf(RT,0,0,"Counting %d",n>>4);
		}

		for( int x = 0 ; x < 5 ; x++ )
		{
			const uint8_t* c = col[(x+(n>>2))&7];

			TheFont.SetBorder(true);
			TheFont.SetPixelSize(1 + (x*2));
			TheFont.SetPenColour(c[0],c[1],c[2]);
			TheFont.Print(RT,x * 10,100 + x*70,"The fox jumped over something...");
		}

		TheFont.SetPenColour(0,0,0);
		TheFont.SetBorder(false);
		TheFont.SetPixelSize(1);
		const int y = RT.GetHeight() - 40;
		RT.FillRoundedRectangle(10,y,400,y + 30,5,255,255,255);
		TheFont.Print(RT,60,y+10,"This demo uses the simple pixel font");
		
		FB->Present(RT);
		n++;
		nanosleep(&SleepTime,NULL);
	};

	delete FB;
	return 0;
}
