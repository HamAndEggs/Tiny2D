#include <iostream>
#include <cmath>

#include "Tiny2D.h"
const float pi = std::acos(-1);
const float radian = pi*2;
int main(int argc, char *argv[])
{
// Display the constants defined by app build. \n";
    std::cout << "Application Version " << APP_VERSION << '\n';
    std::cout << "Build date and time " << APP_BUILD_DATE_TIME << '\n';
    std::cout << "Build date " << APP_BUILD_DATE << '\n';
    std::cout << "Build time " << APP_BUILD_TIME << '\n';

	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(true);
	if( !FB )
		return EXIT_FAILURE;

    tiny2d::DrawBuffer RT(FB);

	tiny2d::PixelFont db(3);
    float t = 0.0f;
	while(FB->GetKeepGoing())
	{
    	RT.Clear(0,0,0);
        
        int r = (int)((std::sin(t) * 120.0f)+80.0f);// Test negative radius and radius*2 > than width / height...

        RT.FillRoundedRectangle(900,100,1000,400,r,255,0,255);
        RT.FillRoundedRectangle(100,450,900,550,r,255,0,255);

        RT.DrawRoundedRectangle(920,120,980,380,r,255,255,255);
        RT.DrawRoundedRectangle(120,470,880,530,r,255,255,255);

        RT.FillRectangle(100-4,100-4,400+4,400+4,55,55,55);
        RT.FillRectangle(500-4,100-4,800+4,400+4,55,55,55);

        RT.FillRoundedRectangle(100,100,400,400,r,255,255,255);
        RT.DrawRoundedRectangle(500,100,800,400,r,255,255,255);

		RT.FillRoundedRectangle(-40,50,50,400,30,-1,-1,-1);

		RT.FillRoundedRectangle(0,560,1024,700,30,255,100,30);
        

        t += 0.01f;
        if( t > radian ) t-= radian;

		db.Printf(RT,0,0,"%f",t);

		FB->Present(RT);
	}

	delete FB;

// And quit";
    return EXIT_SUCCESS;
}
