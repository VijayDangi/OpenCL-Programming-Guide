/**
 * @author : Vijaykumar Dangi
 * @date   : 18-Aug-2023
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include "../../Common/FreeImage/x64/FreeImage.h"

const int OUT_IMAGE_WIDTH = 256;
const int OUT_IMAGE_HEIGHT = 1024;
const int BIN_COUNT = 256;
const int COLOR_RANGE = 256;

/**
 * @brief main() : Entry-Point function
 */
int main( int argc, char **argv)
{
    // function declaration
    bool SaveHistogramGraphImage( uint32_t *red_channel_data, uint32_t *green_channel_data, uint32_t *blue_channel_data, bool b_filled_graph, bool b_sepated_output);
    uint8_t* LoadImage( const char *file_name, int *image_width, int *image_height);

    // variable declaration
    uint8_t *image_bits = nullptr;
    int image_width = 0;
    int image_height = 0;

    uint32_t *ref_histogram_results = nullptr;

    bool b_save_filled_graph = true;
    bool b_save_separate_channel_graph = false;

    std::string input_image;

    // code
    for( int i = 1; i < argc; ++i)
    {
        std::string input( argv[i]);
        if( !input.compare( "--input"))
        {
            input_image = std::string( argv[++i]);
        }
        else if( !input.compare( "-d"))
        {
            b_save_filled_graph = false;
        }
        else if( !input.compare( "-s"))
        {
            b_save_separate_channel_graph = true;
        }
    }

    if( input_image.empty())
    {
        std::cerr << "usage: " << argv[0] << " --input <input_image_name>\n";
        std::cerr << "options: " << "\n"
                  << "   -d: show dotted graph output\n"
                  << "   -s: separate output for each color channel"
                  << std::endl;

        return EXIT_SUCCESS;
    }

        /******** IMAGE LOADING ***********/
    image_bits = LoadImage( input_image.c_str(), &image_width, &image_height);
    if( image_bits == nullptr)
    {
        std::cerr << "Cannot open image \"" << input_image << "\"" << std::endl;
        return EXIT_FAILURE;
    }

        /********* CREATE HISTOGRAM ***********/
            // 256-bins for each color in R-G-B
    //ref_histogram_results = new uint32_t[ COLOR_RANGE * 3]();   // zero-initialize memory

        /**
          *      |<--- red bins -->|  |<--- green bins --->|  |<--- blue bins --->|
          *
          *      ------------------------------------------------------------------
          *      [ | | | . . . | | |  | | | | |. . . | | | |  | | | | |. . .| | | ]
          *      ------------------------------------------------------------------
          * 
          */
    ref_histogram_results = new uint32_t[ COLOR_RANGE * 3];
    memset( ref_histogram_results, 0x0, COLOR_RANGE * 3 * sizeof( uint32_t));

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    
    int index;
    for( int i = 0; i < image_width * image_height; ++i)
    {
            // compute histogram for R channel
        index = image_bits[ 4 * i + 2];
        ref_histogram_results[ index]++;

            // compute histogram for G channel
        index = image_bits[ 4 * i + 1];
        ref_histogram_results[ 256 + index]++;

            // compute histogram for B channel
        index = image_bits[ 4 * i + 0];
        ref_histogram_results[ 512 + index]++;
    }

        /******** SAVE HISTOGRAM *******************/
    SaveHistogramGraphImage( ref_histogram_results, ref_histogram_results + 256, ref_histogram_results + 512, b_save_filled_graph, b_save_separate_channel_graph);


    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double>  elapsed_seconds = end - start;
    std::cout << "Time Required for Histogram by CPU is: " << elapsed_seconds.count() << "s" << std::endl;

    delete ref_histogram_results;
    delete image_bits;
    return EXIT_SUCCESS;
}


/**
 * @brief LoadImage() : Load Image and returns image width, image height and image data in 32-bit format. Delete image data when work is done.
 */
uint8_t* LoadImage( const char *file_name, int *image_width, int *image_height)
{
    // code
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType( file_name, 0);
    FIBITMAP *image = FreeImage_Load( format, file_name);
    if( image == nullptr)
    {
        *image_width = 0;
        *image_height = 0;
        return nullptr;
    }

        // convert to 32-bit image
    FIBITMAP *temp = image;
    image = FreeImage_ConvertTo32Bits( image);
    FreeImage_Unload( temp);

    *image_width = FreeImage_GetWidth( image);
    *image_height = FreeImage_GetHeight( image);

    uint8_t *image_bits = FreeImage_GetBits( image);

    uint8_t *ret_image_bits = new uint8_t[ (*image_width) * (*image_height) * 4];
    memcpy( ret_image_bits, image_bits, (*image_width) * (*image_height) * 4 * sizeof( uint8_t));

    FreeImage_Unload( image);

    return ret_image_bits;
}

/**
 * @brief SaveHistogramGraphImage()
 */
bool SaveHistogramGraphImage( uint32_t *red_channel_data, uint32_t *green_channel_data, uint32_t *blue_channel_data, bool b_filled_graph, bool b_sepated_output)
{
    // function declaration
    bool SaveHistogramImage( uint32_t *red_data, uint32_t *green_data, uint32_t *blue_data, bool b_filled_graph);
    bool SaveSeparateHistogramImage( uint32_t *red_data, uint32_t *green_data, uint32_t *blue_data, bool b_filled_graph);

    // variable declaration
    int max_red = 0;
    int max_green = 0;
    int max_blue = 0;

    bool result = true;

    // code
        // normalize image [0 - 256]
    for( int i = 0; i < BIN_COUNT; ++i)
    {
        if( max_red < red_channel_data[i])
        {
            max_red = red_channel_data[i];
        }

        if( max_green < green_channel_data[i])
        {
            max_green = green_channel_data[i];
        }

        if( max_blue < blue_channel_data[i])
        {
            max_blue = blue_channel_data[i];
        }
    }

    for( int i = 0; i < BIN_COUNT; ++i)
    {
        red_channel_data[i]   = ( (float)red_channel_data[i] / (float)max_red) * (OUT_IMAGE_HEIGHT - 1);
        green_channel_data[i] = ( (float)green_channel_data[i] / (float)max_green) * (OUT_IMAGE_HEIGHT - 1);
        blue_channel_data[i]  = ( (float)blue_channel_data[i] / (float)max_blue) * (OUT_IMAGE_HEIGHT - 1);
    }


    if( b_sepated_output)
    {
        result = SaveSeparateHistogramImage( red_channel_data, green_channel_data, blue_channel_data, b_filled_graph);
    }
    else
    {
        result = SaveHistogramImage( red_channel_data, green_channel_data, blue_channel_data, b_filled_graph);
    }


    return result;
}

/**
 * @brief SaveHistogramImage()
 */
bool SaveHistogramImage( uint32_t *red_data, uint32_t *green_data, uint32_t *blue_data, bool b_filled_graph)
{
    // variable declaration
    uint8_t *image_buffer = nullptr;

    // code
    image_buffer = new uint8_t[ OUT_IMAGE_WIDTH * OUT_IMAGE_HEIGHT * 3]();

    for( int i = 0; i < BIN_COUNT; ++i)
    {
        int j;

        if( b_filled_graph)
        {
            // red
            for( j = 0; j < red_data[i]; ++j)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 2] = 0xFF;
            }

            // green
            for( j = 0; j < green_data[i]; ++j)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 1] = 0xFF;
            }

            // blue
            for( j = 0; j < blue_data[i]; ++j)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 0] = 0xFF;
            }
        }
        else
        {
            // red
            j = red_data[i] - 1;
            if( j >= 0)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 2] = 0xFF;
            }

            // green
            j = green_data[i] - 1;
            if( j >= 0)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 1] = 0xFF;
            }

            // blue
            j = blue_data[i] - 1;
            if( j >= 0)
            {
                image_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 0] = 0xFF;
            }
        }
    }

    // save image
    std::string out_image = "out.png";
    FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename( out_image.c_str());
    int row_pitch = 3 * OUT_IMAGE_WIDTH;
    FIBITMAP *image = FreeImage_ConvertFromRawBits( image_buffer, OUT_IMAGE_WIDTH, OUT_IMAGE_HEIGHT, row_pitch, 24, 0xFF000000, 0x00FF0000, 0x0000FF00);
    FreeImage_Save( format, image, out_image.c_str());
    FreeImage_Unload( image);

    delete image_buffer;
    image_buffer = nullptr;

    return true;
}

/**
 * @brief SaveSeparateHistogramImage()
 */
bool SaveSeparateHistogramImage( uint32_t *red_data, uint32_t *green_data, uint32_t *blue_data, bool b_filled_graph)
{
    // variable declaration
    uint8_t *image_red_buffer = nullptr;
    uint8_t *image_green_buffer = nullptr;
    uint8_t *image_blue_buffer = nullptr;

    // code
    image_red_buffer   = new uint8_t[ OUT_IMAGE_WIDTH * OUT_IMAGE_HEIGHT * 3]();
    image_green_buffer = new uint8_t[ OUT_IMAGE_WIDTH * OUT_IMAGE_HEIGHT * 3]();
    image_blue_buffer  = new uint8_t[ OUT_IMAGE_WIDTH * OUT_IMAGE_HEIGHT * 3]();

    for( int i = 0; i < BIN_COUNT; ++i)
    {
        int j;

        if( b_filled_graph)
        {
            // red
            for( j = 0; j < red_data[i]; ++j)
            {
                image_red_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 2] = 0xFF;
            }

            // green
            for( j = 0; j < green_data[i]; ++j)
            {
                image_green_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 1] = 0xFF;
            }

            // blue
            for( j = 0; j < blue_data[i]; ++j)
            {
                image_blue_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 0] = 0xFF;
            }
        }
        else
        {
            // red
            j = red_data[i] - 1;
            if( j >= 0)
            {
                image_red_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 2] = 0xFF;
            }

            // green
            j = green_data[i] - 1;
            if( j >= 0)
            {
                image_green_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 1] = 0xFF;
            }

            // blue
            j = blue_data[i] - 1;
            if( j >= 0)
            {
                image_blue_buffer[ 3 * (j * OUT_IMAGE_WIDTH + i) + 0] = 0xFF;
            }
        }
    }

    // save images
    {
        std::string out_image = "out_red.png";
        FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename( out_image.c_str());
        int row_pitch = 3 * OUT_IMAGE_WIDTH;
        FIBITMAP *image = FreeImage_ConvertFromRawBits( image_red_buffer, OUT_IMAGE_WIDTH, OUT_IMAGE_HEIGHT, row_pitch, 24, 0xFF000000, 0x00FF0000, 0x0000FF00);
        FreeImage_Save( format, image, out_image.c_str());
        FreeImage_Unload( image);
    }

    {
        std::string out_image = "out_green.png";
        FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename( out_image.c_str());
        int row_pitch = 3 * OUT_IMAGE_WIDTH;
        FIBITMAP *image = FreeImage_ConvertFromRawBits( image_green_buffer, OUT_IMAGE_WIDTH, OUT_IMAGE_HEIGHT, row_pitch, 24, 0xFF000000, 0x00FF0000, 0x0000FF00);
        FreeImage_Save( format, image, out_image.c_str());
        FreeImage_Unload( image);
    }

    {
        std::string out_image = "out_blue.png";
        FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename( out_image.c_str());
        int row_pitch = 3 * OUT_IMAGE_WIDTH;
        FIBITMAP *image = FreeImage_ConvertFromRawBits( image_blue_buffer, OUT_IMAGE_WIDTH, OUT_IMAGE_HEIGHT, row_pitch, 24, 0xFF000000, 0x00FF0000, 0x0000FF00);
        FreeImage_Save( format, image, out_image.c_str());
        FreeImage_Unload( image);
    }

    delete image_red_buffer;
    image_red_buffer = nullptr;

    delete image_green_buffer;
    image_green_buffer = nullptr;

    delete image_blue_buffer;
    image_blue_buffer = nullptr;

    return true;
}
