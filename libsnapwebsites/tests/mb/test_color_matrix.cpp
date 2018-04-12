// Snap Websites Server -- test email.cpp/h
// Copyright (c) 2014-2018  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

//
// This test works along the test_ssl_server.cpp and implements the
// client's side. It connects to the server, sends a few messages (START,
// PAUSE x 4, STOP) and expects a HUP before quitting.
//
// To run the test, you first have to start the server otherwise the
// client won't be able to connect.
//

// snapwebsites lib
//
#include <snapwebsites/matrix.h>
//#include <snapwebsites/file_content.h>
//#include <snapwebsites/hexadecimal_string.h>
#include <snapwebsites/log.h>
//#include <snapwebsites/not_reached.h>
//#include <snapwebsites/qstring_stream.h>
//#include <snapwebsites/snapwebsites.h>

// Qt Lib
//
//#include <QFile>

// C++ lib
//
//#include <functional>

// C lib
//
//#include <unistd.h>



namespace
{

// path as defined on the compiler command line, so the tests here run
// as expected only as long as the source is available at the same place...
//
char * g_image_filename = nullptr;

double g_saturation = 0.0;
double g_hue = 0.0;
double g_brightness = 0.0;

snap::matrix<double>    g_color_matrix(4, 4);

void calculate_color_matrix()
{
    // reset back to identity
    //
    g_color_matrix.identity();

    g_color_matrix = g_color_matrix.brightness(g_brightness);
    g_color_matrix = g_color_matrix.saturation(g_saturation);
    g_color_matrix = g_color_matrix.hue(g_hue);
}



void create_color_image()
{
    QImage img(100, 100, QImage::Format_RGB32);

    uchar * s = img.bits();

    // BGRX -- is the real format here (or XRGB in little endian)
    uchar colors[16 * 4] =
    {
        255,   0,   0, 255,     // Blue
          0, 255,   0, 255,     // Green
          0,   0, 255, 255,     // Red
        255, 255, 255, 255,     // White

        255,   0, 255, 255,     // Purple
          0, 255, 255, 255,     // Yellow
        255, 255,   0, 255,     // Cyan
        128, 128, 128, 255,     // Gray

        255, 128,   0, 255,     // Azure
        255,   0, 128, 255,     // Magenta
        128, 255,   0, 255,     // SpringGreen
         64,  64,  64, 255,     // DarkGray

          0, 255, 128, 255,     // Chartreuse
          0, 128, 255, 255,     // Amber
        128,   0, 255, 255,     // Pink
          0,   0,   0, 255,     // Black
    };

    int c(0);
    for(int j(0); j < 4; ++j)
    {
        for(int i(0); i < 4; ++i)
        {
            int x_offset(i * 25);
            int y_offset(j * 25);
            for(int x(x_offset); x < x_offset+ 25; ++x)
            {
                for(int y(y_offset); y < y_offset+ 25; ++y)
                {
                    // 
                    s[x * 4 + y * 100 * 4 + 0] = colors[c * 4 + 0];
                    s[x * 4 + y * 100 * 4 + 1] = colors[c * 4 + 1];
                    s[x * 4 + y * 100 * 4 + 2] = colors[c * 4 + 2];
                    s[x * 4 + y * 100 * 4 + 3] = colors[c * 4 + 3];
                }
            }
            ++c;
        }
    }

    img.save("test-color-image-100x100.png");
}


void apply_color_matrix()
{
    QImage img(QString::fromUtf8(g_image_filename));
    img.convertToFormat(QImage::Format_RGB32);

    calculate_color_matrix();

    uchar * s(img.bits());

    int const max(img.width() * img.height());
    for(int i(0); i < max; ++i, s += 4)
    {
        // apply matrix
        //
        double const red  (static_cast<double>(s[2]));
        double const green(static_cast<double>(s[1]));
        double const blue (static_cast<double>(s[0]));

        double const r = red   * g_color_matrix[0][0]
                       + green * g_color_matrix[1][0]
                       + blue  * g_color_matrix[2][0]
                       +         g_color_matrix[3][0];
        double const g = red   * g_color_matrix[0][1]
                       + green * g_color_matrix[1][1]
                       + blue  * g_color_matrix[2][1]
                       +         g_color_matrix[3][1];
        double const b = red   * g_color_matrix[0][2]
                       + green * g_color_matrix[1][2]
                       + blue  * g_color_matrix[2][2]
                       +         g_color_matrix[3][2];

        //*tr = r*mat[0][0] + g*mat[1][0] +
		//    b*mat[2][0] + mat[3][0];
        //*tg = r*mat[0][1] + g*mat[1][1] +
		//    b*mat[2][1] + mat[3][1];
        //*tb = r*mat[0][2] + g*mat[1][2] +
		//    b*mat[2][2] + mat[3][2];

        s[2] = static_cast<char>(r);
        s[1] = static_cast<char>(g);
        s[0] = static_cast<char>(b);
    }

    img.save("test-color-matrix.png");
}



}
// no name namespace



int main(int argc, char * argv[])
{
    try
    {
        for(int i(1); i < argc; ++i)
        {
            if(strcmp(argv[i], "--create-color-image") == 0)
            {
                create_color_image();
            }
            else if(strcmp(argv[i], "--image") == 0)
            {
                ++i;
                if(i >= argc)
                {
                    std::cerr << "error: --image expects a filename." << std::endl;
                    return 1;
                }
                g_image_filename = argv[i];
            }
            else if(strcmp(argv[i], "--saturation") == 0)
            {
                ++i;
                if(i >= argc)
                {
                    std::cerr << "error: --saturation expects a number." << std::endl;
                    return 1;
                }
                g_saturation = std::stod(argv[i]);
            }
            else if(strcmp(argv[i], "--hue") == 0)
            {
                ++i;
                if(i >= argc)
                {
                    std::cerr << "error: --hue expects a number." << std::endl;
                    return 1;
                }
                g_hue = std::stod(argv[i]);
            }
            else if(strcmp(argv[i], "--brightness") == 0)
            {
                ++i;
                if(i >= argc)
                {
                    std::cerr << "error: --brightness expects a number." << std::endl;
                    return 1;
                }
                g_brightness = std::stod(argv[i]);
            }
            else if(strcmp(argv[i], "--apply-color-matrix") == 0)
            {
                apply_color_matrix();
            }
        }

        return 0;
    }
    catch( std::exception const & e )
    {
        SNAP_LOG_FATAL("Caught standard exception [")(e.what())("].");
    }

    return 1;
}

// vim: ts=4 sw=4 et
