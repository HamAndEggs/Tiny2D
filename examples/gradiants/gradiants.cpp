
#include <iostream>
#include <cmath>
#include "Tiny2D.h"

int main(int argc, char *argv[])
{
// Display the constants defined by app build. \n";
    std::cout << "Application Version " << APP_VERSION << '\n';
    std::cout << "Build date and time " << APP_BUILD_DATE_TIME << '\n';
    std::cout << "Build date " << APP_BUILD_DATE << '\n';
    std::cout << "Build time " << APP_BUILD_TIME << '\n';   

	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(tiny2d::FrameBuffer::VERBOSE_MESSAGES);
	if( !FB )
		return EXIT_FAILURE;

    tiny2d::DrawBuffer RT(FB);

	tiny2d::PixelFont TheFont(3);
	tiny2d::PixelFont bigFont(10);

    TheFont.SetBorder(true);
    bigFont.SetBorder(true);

    int n = 0;
    float a = 0.0f;
	while(FB->GetKeepGoing())
	{
        const int i = (n++&255);
        const int c = (int)((std::sin(a)*127)+127);
        a += 0.01f;

    	RT.Clear(0,0,0);

        RT.DrawGradient(0,0,RT.GetWidth()/2,RT.GetHeight(),0,0,100,0,180,50);
        RT.DrawGradient(RT.GetWidth()/2,RT.GetHeight(),RT.GetWidth(),0,0,0,100,0,180,50);
        RT.DrawGradient(RT.GetWidth()*3/8,100,RT.GetWidth()*6/8,355,255,c,0,0,255,c);


		TheFont.Print(RT,RT.GetWidth() * 5 / 19,50,"HSV blend");

        bigFont.Printf(RT,300,400,"%d",i);

        FB->Present(RT);
    };

    delete FB;

// And quit";
    return EXIT_SUCCESS;
}
