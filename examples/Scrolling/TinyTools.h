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

#ifndef TINY_TOOLS_H
#define TINY_TOOLS_H

#include <vector>
#include <string>
#include <functional>

#include <assert.h>
#include <time.h>

namespace tinytools{	// Using a namespace to try to prevent name clashes as my class name is kind of obvious. :)
///////////////////////////////////////////////////////////////////////////////////////////////////////////

class MillisecondTicker
{
public:
	MillisecondTicker() = default;
    MillisecondTicker(int pMilliseconds)
	{
		SetTimeout(pMilliseconds);
	}


	/**
	 * @brief Sets the new timeout interval, resets internal counter.
	 * 
	 * @param pMilliseconds 
	 */
	void SetTimeout(int pMilliseconds)
	{
		assert(pMilliseconds > 0 );
		mTimeout = (CLOCKS_PER_SEC * pMilliseconds) / 1000;
		mTrigger = clock() + mTimeout;
	}

	/**
	 * @brief Returns true if trigger ticks is less than now
	 */
    bool Tick(){return Tick(clock());}
    bool Tick(const clock_t pNow)
	{
		if( mTrigger < pNow )
		{
			mTrigger += mTimeout;
			return true;
		}
		return false;
	}

	/**
	 * @brief Calls the function if trigger ticks is less than now. 
	 */
    void Tick(std::function<void()> pCallback){Tick(clock(),pCallback);}
    void Tick(const clock_t pNow,std::function<void()> pCallback )
	{
		assert( pCallback != nullptr );
		if( mTrigger < pNow )
		{
			mTrigger += mTimeout;
			pCallback();
		}
	}


private:
    clock_t mTimeout;
    clock_t mTrigger;

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
};//namespace tiny2d
	
#endif //TINY_2D_H
