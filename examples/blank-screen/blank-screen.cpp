
#include <iostream>
#include "Tiny2D.h"

int main(int argc, char *argv[])
{
	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open();
	if( !FB )
		return EXIT_FAILURE;

    FB->ClearScreen(0,0,0);

    delete FB;

    return EXIT_SUCCESS;
}
