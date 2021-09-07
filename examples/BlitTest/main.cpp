#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <string>

#include "Tiny2D.h"
#include "../SupportCode/TinyPNG.h"

static void LoadPNG(tiny2d::DrawBuffer& pImage,const std::string& pFilename)
{
    static tinypng::Loader pngLoader(false);

    if( pngLoader.LoadFromFile(pFilename) == false )
    {
        throw std::runtime_error("Failed to load image " + pFilename);
    }

    std::vector<uint8_t> pixels;

    if( pngLoader.GetHasAlpha() )
    {
        pngLoader.GetRGBA(pixels);
        pImage.Resize(pngLoader.GetWidth(),pngLoader.GetHeight(),4,true,false);
        pImage.BlitRGBA(pixels.data(),0,0,pngLoader.GetWidth(),pngLoader.GetHeight());
    }
    else
    {
        pngLoader.GetRGB(pixels);
        pImage.Resize(pngLoader.GetWidth(),pngLoader.GetHeight(),3,false,false);
        pImage.BlitRGB(pixels.data(),0,0,pngLoader.GetWidth(),pngLoader.GetHeight());
    }
}

int main(int argc, char *argv[])
{	
	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(true);
	if( !FB )
		return 1;

	tiny2d::DrawBuffer RT(FB);
	tiny2d::PixelFont Font(3);
	Font.SetBorder(true);

	srand(time(NULL));

    // Load in a test texture
    tiny2d::DrawBuffer Bird_by_Magnus;
    tiny2d::DrawBuffer create;
    tiny2d::DrawBuffer plant;
    tiny2d::DrawBuffer debug1;
    tiny2d::DrawBuffer debug2;
    tiny2d::DrawBuffer ball;

    LoadPNG(Bird_by_Magnus,"../data/Bird_by_Magnus.png");
    LoadPNG(create,"../data/crate.png");
    LoadPNG(plant,"../data/plant.png");
    LoadPNG(debug1,"../data/debug.png");
    LoadPNG(debug2,"../data/debug2.png");
    LoadPNG(ball,"../data/foot-ball.png");


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

        RT.Blit(Bird_by_Magnus,RT.GetWidth()-Bird_by_Magnus.GetWidth(),RT.GetHeight() - Bird_by_Magnus.GetHeight());

		{
			int cX = (int)(-40.0f + ((RT.GetWidth()+80.0f) * anim));
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

        RT.Blend(create,500,60);
        RT.Blend(plant,RT.GetWidth() - plant.GetWidth(),RT.GetHeight() - plant.GetHeight());

		FB->Present(RT);
	}

	delete FB;
	return 0;
}
