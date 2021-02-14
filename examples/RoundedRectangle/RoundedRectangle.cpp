#include <iostream>
#include <cmath>

#include "framebuffer.h"
const float pi = std::acos(-1);
const float radian = pi*2;
int main(int argc, char *argv[])
{
// Display the constants defined by app build. \n";
    std::cout << "Application Version " << APP_VERSION << '\n';
    std::cout << "Build date and time " << APP_BUILD_DATE_TIME << '\n';
    std::cout << "Build date " << APP_BUILD_DATE << '\n';
    std::cout << "Build time " << APP_BUILD_TIME << '\n';

	FBIO::FrameBuffer* FB = FBIO::FrameBuffer::Open(true);
	if( !FB )
		return EXIT_FAILURE;

	uint8_t col[8][3] = {{0,0,0},{255,0,0},{0,255,0},{0,0,255},{255,255,255},{255,0,255},{255,255,0},{0,255,255}};
	
	FBIO::PixelFont db(3);
    float t = 0.0f;
	while(FB->GetKeepGoing())
	{
    	FB->ClearScreen(0,0,0);
        
        int r = (int)((std::sin(t) * 120.0f)+80.0f);// Test negative radius and radius*2 > than width / height...

        FB->DrawRoundedRectangle(900,100,1000,400,r,255,0,255,true);
        FB->DrawRoundedRectangle(100,450,900,550,r,255,0,255,true);

        FB->DrawRoundedRectangle(920,120,980,380,r,255,255,255,false);
        FB->DrawRoundedRectangle(120,470,880,530,r,255,255,255,false);

        FB->DrawRectangle(100-4,100-4,400+4,400+4,55,55,55,true);
        FB->DrawRectangle(500-4,100-4,800+4,400+4,55,55,55,true);

        FB->DrawRoundedRectangle(100,100,400,400,r,255,255,255,true);
        FB->DrawRoundedRectangle(500,100,800,400,r,255,255,255,false);

		FB->DrawRoundedRectangle(-40,50,50,400,30,-1,-1,-1,true);

		FB->DrawRoundedRectangle(0,560,1024,700,30,255,100,30,true);
        
        if(false)
		{
			int FromX = rand()%FB->GetWidth();
			int FromY = rand()%FB->GetHeight();
			int ToX = rand()%FB->GetWidth();
			int ToY = rand()%FB->GetHeight();

			int c = rand()&7;

			FB->DrawRectangle(FromX,FromY,ToX,ToY,col[c][0],col[c][1],col[c][2],(rand()&1) == 0);
		}

        t += 0.01f;
        if( t > radian ) t-= radian;

		db.Printf(FB,0,0,"%f",t);

		FB->Present();
	}

	// Stop monitor burn in...
	FB->ClearScreen(0,0,0);	

	delete FB;

// And quit";
    return EXIT_SUCCESS;
}
