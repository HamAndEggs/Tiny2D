/*
   Copyright (C) 2017, Richard e Collins.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <iostream>
#include <cstdlib>
#include <time.h>
#include <unistd.h>
#include <cstdarg>
#include <string.h>

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
	RT.Clear(150,150,150);

	tiny2d::PixelFont TheFont(3);

    int n = 0;
	while(FB->GetKeepGoing())
	{
        RT.Clear(150,150,150);

        RT.DrawRectangle(100,50,200,150,255,255,255);
        RT.DrawGradient(0,200,RT.GetWidth(),500,255,128,64,0,255,128);

        RT.FillRectangle(300,300,400,400,255,0,0);
        RT.FillRectangle(500,300,600,400,0,255,0);
        RT.FillRectangle(700,300,800,400,0,0,255);

		TheFont.Printf(RT,0,0,"Counting %d",n++);
        FB->Present(RT);
    };

    delete FB;

    return EXIT_SUCCESS;
}
