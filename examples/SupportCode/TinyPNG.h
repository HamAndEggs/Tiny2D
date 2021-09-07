/*
   Copyright (C) 2021, Richard e Collins.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
   
   Original code base is at https://github.com/HamAndEggs/TinyPNG
   
   Resources used,
   https://en.wikipedia.org/wiki/Portable_Network_Graphics
   https://www.w3.org/TR/PNG/#5Chunk-layout

   */

#ifndef TINY_PNG_H
#define TINY_PNG_H

#include <vector>
#include <string>

namespace tinypng{ // Using a namespace to try to prevent name clashes as my class names are kind of obvious :)
///////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PNGChunk; //!< Forwad decleration of internal structure.

/**
 * @brief 
 * 
 */
class Loader
{
public:

    enum PNGColourType
    {
        CT_GREY_SCALE = 0,
        CT_TRUE_COLOUR = 2,
        CT_INDEX_COLOUR = 3,
        CT_GREYSCALE_WITH_ALPHA = 4,
        CT_TRUE_COLOUR_WITH_ALPHA = 6,

        CT_INVALID = 255
    };

    /**
     * @brief Construct a new PNG Loader
     * 
     * @param pVerbose If true then debug information will be sent to std::log
     */
    Loader(bool pVerbose = false);

    /**
     * @brief Loads the PNG from file
     * 
     * @param pFilename The file to load.
     * @return true If the PNG was loaded ok.
     * @return false If file was not found our the file was corrupt.
     */
    bool LoadFromFile(const std::string& pFilename);

    /**
     * @brief Decodes the PNG that is held in memory.
     * 
     * @param pMemory The PNG as loaded from a file in memory.
     * @return true  If the PNG was loaded ok.
     * @return false If PNG was corrupt / invalid.
     */
    bool LoadFromMemory(const std::vector<uint8_t>& pMemory);

    uint32_t GetWidth()const{return mWidth;}
    uint32_t GetHeight()const{return mHeight;}

    /**
     * @brief Will return a 24bit image in RGB order. Will convert the original data to correct bit depth. Alpha is ignored.
     * 
     * @param rRGB 
     * @return true 
     * @return false 
     */
    bool GetRGB(std::vector<uint8_t>& rRGB)const;

    /**
     * @brief Will return a 32bit image in RGBA order. Will convert the original data to correct bit depth.
     * Alpha is set to 255 if was not already in source image.
     * 
     * @param rRGBA 
     * @return true 
     * @return false 
     */
    bool GetRGBA(std::vector<uint8_t>& rRGBA)const;

    /**
     * @brief Gets the alpha status of the PNG file.
     * 
     * @return true 
     * @return false 
     */
    bool GetHasAlpha()const{return mHasAlpha;}

    /**
     * @brief Makes the loaded image go away.
     * 
     */
    void Clear();

private:
    const bool mVerbose;
    uint32_t mWidth;
    uint32_t mHeight;
    int mBitDepth;
    int mBytesPerPixel;
    bool mHasAlpha;
    PNGColourType mType;
    int mCompressionMethod;
    int mFilterMethod;
    int mInterlaceMethod;

    // When I load I split out the channels like this to help avoid endian issues.
    // There will be supporting functions to return the data in the most popular arrangements.
    std::vector<uint8_t> mRed,mGreen,mBlue,mAlpha;

    bool ReadImageHeader(const PNGChunk& pChunk);
    bool BuildImage(const std::vector<uint8_t>& pCompressionData);    

    void FillColourPlanes(const std::vector<uint8_t>pDecompressedImageData,std::vector<uint8_t>& rRowFilters);

    void PushGreyscalePixels(const std::vector<uint8_t>& pImageData,const std::vector<uint8_t>pRowFilters);
    void PushTrueColour(const std::vector<uint8_t>& pImageData,const std::vector<uint8_t>pRowFilters);
    void PushIndexPixels(const std::vector<uint8_t>& pImageData,const std::vector<uint8_t>pRowFilters);
    void PushGreyscaleAlphaPixels(const std::vector<uint8_t>& pImageData,const std::vector<uint8_t>pRowFilters);
    void PushTrueColourAlphaPixels(const std::vector<uint8_t>& pImageData,const std::vector<uint8_t>pRowFilters);

};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
};// namespace tinypng

#endif //TINY_PNG_H