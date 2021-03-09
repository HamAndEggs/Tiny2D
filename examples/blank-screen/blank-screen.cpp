#include <iostream>
#include "Tiny2D.h"

int main(int argc, char *argv[])
{
    tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open();
    if( !FB )
        return EXIT_FAILURE;

    tiny2d::DrawBuffer RT(FB);
    RT.Clear(0,0,0);

    FB->Present(RT);
    delete FB;

    return EXIT_SUCCESS;
}
