
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <unistd.h>
#include <cstdarg>
#include <string.h>

#include "Tiny2D.h"

int main(int argc, char *argv[])
{
// Say hello to the world!
    std::cout << "Free Type Font.\n";

// Display the constants defined by app build. \n";
    std::cout << "Application Version " << APP_VERSION << '\n';
    std::cout << "Build date and time " << APP_BUILD_DATE_TIME << '\n';
    std::cout << "Build date " << APP_BUILD_DATE << '\n';
    std::cout << "Build time " << APP_BUILD_TIME << '\n';

    tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(true);
	if( !FB )
    {
		return EXIT_FAILURE;
    }

    tiny2d::DrawBuffer RT(FB);

	srand(time(NULL));

    const uint8_t BG_R = 255;
    const uint8_t BG_G = 0;
    const uint8_t BG_B = 0;


    tiny2d::FreeTypeFont FTFont("../data/Blenda Script.otf",60,true);
    FTFont.SetBackgroundColour(BG_R,BG_G,BG_B);
    FTFont.SetPenColour(0,255,255);

    tiny2d::FreeTypeFont FTFont2("../data/MachineScript.ttf",45,true);
    FTFont2.SetBackgroundColour(BG_R,BG_G,BG_B);
    FTFont2.SetPenColour(0,255,0);

    // Grab something the compiler can't optimise out.
    char buf[32];
    gethostname(buf,31);
    const std::string host = buf;

    while( FB->GetKeepGoing() )
    {
	    RT.Clear(BG_R,BG_G,BG_B);

        FTFont.SetPenColour(0,0,0);
        FTFont.Printf(RT,0,80,"Blenda Script 0123456789 :)");

        FTFont.SetPenColour(0,255,255);
        FTFont.Print(RT,0,180,"Spacing Test iAlBjXvIoiP X l");

        FTFont2.Print(RT,10,300,"Test Number 0123456789");
        FTFont2.Printf(RT,10,400,"Random Number %d",rand());

        std::string something = "Hostname: " + host;
        FTFont.Print(RT,10,500,something);

        FB->Present(RT);
        sleep(1);
    }

	delete FB;

// And quit";
    return EXIT_SUCCESS;
}
