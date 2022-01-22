#include <iostream>
#include "Tiny2D.h"

int main(int argc, char *argv[])
{
    tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open();
    if( !FB )
        return EXIT_FAILURE;

    tiny2d::DrawBuffer RT(FB);
    RT.Clear(0,0,0);
    for( int n = 0 ; n < RT.GetWidth() ; n+=2 )
    {
        RT.DrawLineV(n,0,RT.GetHeight(),255,255,255);
    }

	tiny2d::PixelFont TheFont(10);
	TheFont.SetPenColour(255,255,255);
	TheFont.SetBorder(true);
    TheFont.Printf(RT,0,RT.GetHeight()-TheFont.GetCharHeight(),"%dx%d",RT.GetWidth(),RT.GetHeight());


	while(FB->GetKeepGoing())
	{
        FB->Present(RT);
    }
    delete FB;

    return EXIT_SUCCESS;
}
