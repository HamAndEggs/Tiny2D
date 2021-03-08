
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

    FB->ClearScreen(0,0,0);

    tiny2d::PixelFont TheFont(2);
    TheFont.SetBorder(true);

    // Make three boxes of the tree colours with graduated alpha.
    std::vector<uint8_t> R1;
    std::vector<uint8_t> G1;
    std::vector<uint8_t> B1;
    std::vector<uint8_t> R2;
    std::vector<uint8_t> G2;
    std::vector<uint8_t> B2;

    Fill(R1,255,0,0);
    Fill(G1,0,255,0);
    Fill(B1,0,0,255);

    Fill(R2,255,0,0);
    Fill(G2,0,255,0);
    Fill(B2,0,0,255);

    FB->PreMultiplyAlphaChannel(R2);
    FB->PreMultiplyAlphaChannel(G2);
    FB->PreMultiplyAlphaChannel(B2);

    int n = 0;
    while(FB->GetKeepGoing())
    {
        FB->ClearScreen(150,150,150);

        // Draw a background to alpha against
        FB->DrawGradient(0,0,FB->GetWidth(),FB->GetHeight(),255,128,64,64,0,192);

        for( int y = 0 ; y < 40 ; y++ )
        {
           for( int x = 0 ; x < 40 ; x++ )
            {
                {
                    const int rX = (x * 10) + (FB->GetWidth()/2) - 420;
                    const int rY = (y * 10) + (FB->GetHeight()/2) - 200;

                    if( (x&1) == (y&1) )
                        FB->DrawRectangle(rX,rY,rX+10,rY+10,255,255,255,true);
                    else
                        FB->DrawRectangle(rX,rY,rX+10,rY+10,0,0,0,true);
                }

                {
                    const int rX = (x * 10) + (FB->GetWidth()/2) + 20;
                    const int rY = (y * 10) + (FB->GetHeight()/2) - 200;

                    if( (x&1) == (y&1) )
                        FB->DrawRectangle(rX,rY,rX+10,rY+10,255,255,255,true);
                    else
                        FB->DrawRectangle(rX,rY,rX+10,rY+10,0,0,0,true);
                }
            }
        }

        // Now draw the alpha tests.
        FB->BlitRGBA(R1.data(),50,100,256,256);
        FB->BlitRGBA(G1.data(),150,250,256,256);
        FB->BlitRGBA(B1.data(),200,80,256,256);
        TheFont.Print(FB,60,110,"Normal Alpha blending");


        FB->BlitRGBA(R2.data(),450,100,256,256,true);
        FB->BlitRGBA(G2.data(),550,250,256,256,true);
        FB->BlitRGBA(B2.data(),600,80,256,256,true);
        TheFont.Print(FB,540,140,"Pre multiplied Alpha blending");


        TheFont.Printf(FB,0,0,"Counting %d",n++);
        FB->Present();
    };

    delete FB;

    return EXIT_SUCCESS;
}
