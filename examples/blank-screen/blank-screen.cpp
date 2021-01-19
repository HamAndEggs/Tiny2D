
#include <iostream>
#include "framebuffer.h"

int main(int argc, char *argv[])
{
	FBIO::FrameBuffer* FB = FBIO::FrameBuffer::Open();
	if( !FB )
		return EXIT_FAILURE;

    FB->ClearScreen(0,0,0);

    delete FB;

    return EXIT_SUCCESS;
}
