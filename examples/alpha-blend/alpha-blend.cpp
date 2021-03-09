
#include <iostream>
#include "Tiny2D.h"

const void Fill(std::vector<uint8_t>& pTestPattern,uint8_t pRed,uint8_t pGreen,uint8_t pBlue)
{
    pTestPattern.resize(256*256*4);

    uint8_t* dst = pTestPattern.data();

    for(int y = 0 ; y < 256 ; y++ )
    {
        for(int x = 0 ; x < 256 ; x++, dst += 4 )
        {
            dst[0] = pRed;
            dst[1] = pGreen;
            dst[2] = pBlue;
            dst[3] = (y+x)/2;
        }
    }
}

int main(int argc, char *argv[])
{
    tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open();
    if( !FB )
    {
        return EXIT_FAILURE;
    }

	tiny2d::DrawBuffer RT(FB);
    RT.Clear(0,0,0);

    tiny2d::PixelFont TheFont(2);
    TheFont.SetBorder(true);

    // Make six boxes of the three colours with graduated alpha, one with pre multiplied alpha one without.
    // All start out as normal alpha. Is source data was already premultiplied then you would set the param in the constructor.
    // You can also change it later if you update the pixels yourself.
    tiny2d::DrawBuffer R1(256,256,true);
    tiny2d::DrawBuffer G1(256,256,true);
    tiny2d::DrawBuffer B1(256,256,true);
    tiny2d::DrawBuffer R2(256,256,true);
    tiny2d::DrawBuffer G2(256,256,true);
    tiny2d::DrawBuffer B2(256,256,true);

    Fill(R1.mPixels,255,0,0);
    Fill(G1.mPixels,0,255,0);
    Fill(B1.mPixels,0,0,255);

    R2.mPixels = R1.mPixels;
    G2.mPixels = G1.mPixels;
    B2.mPixels = B1.mPixels;

    // Now make the pre multiplied alpha version.
    R2.PreMultiplyAlpha();
    G2.PreMultiplyAlpha();
    B2.PreMultiplyAlpha();

    int n = 0;
    while(FB->GetKeepGoing())
    {
        RT.Clear(150,150,150);

        // Draw a background to alpha against
        RT.DrawGradient(0,0,RT.GetWidth(),RT.GetHeight(),255,128,64,64,0,192);

        // Draws the two checkerboards.
        for( int y = 0 ; y < 30 ; y++ )
        {
           for( int x = 0 ; x < 40 ; x++ )
            {
                {
                    const int rX = (x * 10) + (RT.GetWidth()/2) - 420;
                    const int rY = (y * 10) + (RT.GetHeight()/2) - 160;

                    if( (x&1) == (y&1) )
                        RT.FillRectangle(rX,rY,rX+10,rY+10,255,255,255);
                    else
                        RT.FillRectangle(rX,rY,rX+10,rY+10,0,0,0);
                }

                {
                    const int rX = (x * 10) + (RT.GetWidth()/2) + 20;
                    const int rY = (y * 10) + (RT.GetHeight()/2) - 160;

                    if( (x&1) == (y&1) )
                        RT.FillRectangle(rX,rY,rX+10,rY+10,255,255,255);
                    else
                        RT.FillRectangle(rX,rY,rX+10,rY+10,0,0,0);
                }
            }
        }

        // Now draw the alpha tests.
        RT.Blit(R1,50,100);
        RT.Blit(G1,150,250);
        RT.Blit(B1,200,80);
        TheFont.Print(RT,60,110,"Normal Alpha blending");

        RT.Blit(R2,450,100);
        RT.Blit(G2,550,250);
        RT.Blit(B2,600,80);
        TheFont.Print(RT,540,140,"Pre multiplied Alpha blending");

        TheFont.Printf(RT,0,0,"Counting %d",n++);
        FB->Present(RT);
    };

    delete FB;

    return EXIT_SUCCESS;
}
