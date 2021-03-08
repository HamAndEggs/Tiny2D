
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

	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(true);
	if( !FB )
		return EXIT_FAILURE;

    uint32_t rRGBBlendTable[256];
    uint32_t rHSVBlendTable[256];

    tiny2d::FrameBuffer::TweenColoursRGB(255,0,0,0,255,0,rRGBBlendTable);
    tiny2d::FrameBuffer::TweenColoursHSV(255,0,0,0,255,0,rHSVBlendTable);


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

    	FB->ClearScreen(0,0,0);

        FB->DrawGradient(0,0,FB->GetWidth()/2,FB->GetHeight(),0,0,100,0,180,50);
        FB->DrawGradient(FB->GetWidth()/2,FB->GetHeight(),FB->GetWidth(),0,0,0,100,0,180,50);

        FB->DrawGradient(FB->GetWidth()*3/8,100,FB->GetWidth()*6/8,355,255,c,0,0,255,c);


		TheFont.Print(FB,FB->GetWidth() * 5 / 19,50,"HSV blend");

        bigFont.Printf(FB,300,400,"%d",i);

        FB->Present();
    };

    delete FB;

// And quit";
    return EXIT_SUCCESS;
}
