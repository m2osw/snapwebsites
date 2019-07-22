// Snap Websites Server -- test matrix.cpp/h
// Copyright (c) 2014-2019  Made to Order Software Corp.  All Rights Reserved
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


// TODO: move this test to the snapdev project where matrix.h is defined
//

// snapwebsites lib
//
#include <snapwebsites/log.h>


// snapdev lib
//
#include <snapdev/matrix.h>


// Qt lib
//
#include <QImage>


// last include
//
#include <snapdev/poison.h>






namespace
{

// path as defined on the compiler command line, so the tests here run
// as expected only as long as the source is available at the same place...
//
char * g_image_filename = nullptr;

double g_saturation = 1.0;
double g_hue = 0.0;
double g_brightness = 1.0;

snap::matrix<double>    g_color_matrix(4, 4);

void calculate_color_matrix()
{
    // reset back to identity
    //
    g_color_matrix.identity();
std::cerr << "identity? = " << g_color_matrix << "\n";

std::cerr << "brightness = " << g_brightness << "\n";
    g_color_matrix = g_color_matrix.brightness(g_brightness);
std::cerr << "matrix = " << g_color_matrix << "\n";
    g_color_matrix = g_color_matrix.saturation(g_saturation);
std::cerr << "saturation " << g_saturation << " -> matrix = " << g_color_matrix << "\n";
    g_color_matrix = g_color_matrix.hue(g_hue *  M_PI / 180.0);
std::cerr << "hue " << (g_hue *  M_PI / 180.0) << " -> matrix = " << g_color_matrix << "\n";
}


uchar clamp(double c)
{
    int v(static_cast<int>(c + 0.5));
    if(v <= 0)
    {
        return 0;
    }
    if(v >= 255)
    {
        return 255;
    }

    return v;
}



void create_color_image()
{
    QImage img(100, 100, QImage::Format_RGB32);

    uchar * s = img.bits();

    // BGRX -- is the real format here (or XRGB in little endian using uint32_t)
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
            for(int x(x_offset); x < x_offset + 25; ++x)
            {
                for(int y(y_offset); y < y_offset + 25; ++y)
                {
                    // copy the colors in small blocks
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


//
// Note:
// First, decide which order to apply the rotations (say X then Y then Z).
// Then, it also depends on your convention of whether your points are row
// vectors or column vectors. For row vectors, you have ((r*X)*Y)*Z = r*(XYZ)
// -- vs. column vectors, you have Z*(Y*(X*c))=(ZYX)*c 
//
void apply_color_matrix()
{
    QImage img(QString::fromUtf8(g_image_filename));
    img = img.convertToFormat(QImage::Format_RGB32);

// 17 is pretty much the best precision we can get here
//
std::cerr.precision(17);
std::cout.precision(17);

    calculate_color_matrix();

snap::matrix<double> p(4,4);
p[0][0] = 0.81649658092772615;
p[0][1] = 0.0;
p[0][2] = 0.097737296040753485;
p[0][3] = 0.0;
p[1][0] = -0.40824829046386302;
p[1][1] = 0.70710678118654746;
p[1][2] = 0.32831470544894448;
p[1][3] = 0.0;
p[2][0] = -0.40824829046386302;
p[2][1] = -0.70710678118654746;
p[2][2] = 1.3059988060791796;
p[2][3] = 0.0;
p[3][0] = 0.0;
p[3][1] = 0.0;
p[3][2] = 0.0;
p[3][3] = 1.0;

snap::matrix<double> r_b(4,4);
double const rot_cos = cos((g_hue * M_PI / 180.0));
double const rot_sin = sin((g_hue * M_PI / 180.0));
r_b[0][0] =  rot_cos;
r_b[0][1] =  rot_sin;
r_b[1][0] = -rot_sin;
r_b[1][1] =  rot_cos;

snap::matrix<double> m(4,4);
m = p * r_b / p;

std::cerr << "quick matrix = " << m << "\n";


// Microsoft simplified matrix for +45 deg hue changes
//g_color_matrix[0][0] =  0.6188793;
//g_color_matrix[0][1] =  0.1635025;
//g_color_matrix[0][2] = -0.4941068;
//g_color_matrix[0][3] =  0.0;
//g_color_matrix[1][0] = -0.2961627;
//g_color_matrix[1][1] =  1.0155204;
//g_color_matrix[1][2] =  0.715;
//g_color_matrix[1][3] =  0.0;
//g_color_matrix[2][0] =  0.6772834;
//g_color_matrix[2][1] = -0.1790229;
//g_color_matrix[2][2] =  0.7791068;
//g_color_matrix[2][3] =  0.0;
//g_color_matrix[3][0] =  0.0 ;
//g_color_matrix[3][1] =  0.0;
//g_color_matrix[3][2] =  0.0;
//g_color_matrix[3][3] =  1.0;

std::cerr << "color matrix = " << g_color_matrix << "\n";
std::cerr << "c0 = " << (g_color_matrix[0][0] + g_color_matrix[1][0] + g_color_matrix[2][0])
          << ", c1 = " << (g_color_matrix[0][1] + g_color_matrix[1][1] + g_color_matrix[2][1])
          << ", c2 = " << (g_color_matrix[0][2] + g_color_matrix[1][2] + g_color_matrix[2][2])
          << "\n";

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

        s[2] = clamp(r);
        s[1] = clamp(g);
        s[0] = clamp(b);
    }

    img.save("test-color-matrix.png");
}


void show_hue_matrix(int luma_select)
{
    snap::matrix<double> a(4, 4);

    // setup the the luma and then calculate a few matrices that are
    // used to calculate the factors
    //
    switch(luma_select)
    {
    case 0:
        std::cerr << "HDTV luma" << std::endl << std::endl;
        a.set_luma_vector(a.HDTV_LUMA_RED, a.HDTV_LUMA_GREEN, a.HDTV_LUMA_BLUE);
        break;

    case 1:
        std::cerr << "LED luma" << std::endl << std::endl;
        a.set_luma_vector(a.LED_LUMA_RED, a.LED_LUMA_GREEN, a.LED_LUMA_BLUE);
        break;

    case 2:
        std::cerr << "CRT luma" << std::endl << std::endl;
        a.set_luma_vector(a.CRT_LUMA_RED, a.CRT_LUMA_GREEN, a.CRT_LUMA_BLUE);
        break;

    case 3:
        std::cerr << "NTSC luma" << std::endl << std::endl;
        a.set_luma_vector(a.NTSC_LUMA_RED, a.NTSC_LUMA_GREEN, a.NTSC_LUMA_BLUE);
        break;

    case 4:
        std::cerr << "AVERAGE luma" << std::endl << std::endl;
        a.set_luma_vector(a.AVERAGE_LUMA_RED, a.AVERAGE_LUMA_GREEN, a.AVERAGE_LUMA_BLUE);
        break;

    default:
        std::cerr << "Unknown luma number " << luma_select << ", try a number between 0 and 3 instead." << std::endl;
        return;

    }

    // $R_r$ -- rotation around red axis (inverse rotation around X axis)
    //
    snap::matrix<double> r_r(4, 4);
    double const inv_sqrt_2 = 1.0 / sqrt(2.0);
    r_r[1][1] =  inv_sqrt_2;
    r_r[1][2] =  inv_sqrt_2;
    r_r[2][1] = -inv_sqrt_2;
    r_r[2][2] =  inv_sqrt_2;

//std::cerr << "R_r = " << r_r << "\n";

    // $R_g$ -- rotation around green axis (inverse rotation around Y axis)
    //
    snap::matrix<double> r_g(4, 4);
    double const inv_sqrt_3 = 1.0 / sqrt(3.0);
    double const sqrt_2_over_sqrt_3 = sqrt(2.0) / sqrt(3.0);
    r_g[0][0] =  sqrt_2_over_sqrt_3;
    r_g[0][2] =  inv_sqrt_3;
    r_g[2][0] = -inv_sqrt_3;
    r_g[2][2] =  sqrt_2_over_sqrt_3;

//std::cerr << "R_g = " << r_g << "\n";

    // $R_{rg}$ -- the product or $R_r$ and $R_g$
    //
    snap::matrix<double> r_rg(r_r * r_g);

//std::cerr << "R_rg = " << r_rg << "\n";

    // Luminance Vector
    //
    snap::matrix<double> w(a.get_luma_vector());

//std::cerr << "w = " << w << "\n";

    snap::matrix<double> l(r_rg * w);

//std::cerr << "l = " << l << "\n";

    snap::matrix<double> s(4, 4);
    s[0][2] = l[0][0] / l[2][0];
    s[1][2] = l[1][0] / l[2][0];

//std::cerr << "s = " << s << "\n";

//std::cerr << "s / s = " << (s/s) << "\n";

//std::cerr << "M_rg * s = " << (r_rg * s) << "\n";

    snap::matrix<double> p(r_rg);
    p *= s;

//std::cerr << "p = " << p << "\n";

    std::cout << "    $m = array(";
    for(size_t j(0); j < 4; ++j)
    {
        std::cout << std::endl << "            array(";
        std::cout << p[j][0];
        for(size_t i(1); i < 4; ++i)
        {
            std::cout << ", " << p[j][i];
        }
        std::cout << "),";
    }
    std::cout << std::endl << "        );" << std::endl;

// 'a' is an identity, so we can use it to compute the inverse
    snap::matrix<double> p_inv(a/p);

//std::cerr << "p^-1 = " << p_inv << "\n";

    std::cout << "    $m_inv = array(";
    for(size_t j(0); j < 4; ++j)
    {
        std::cout << std::endl << "            array(";
        std::cout << p_inv[j][0];
        for(size_t i(1); i < 4; ++i)
        {
            std::cout << ", " << p_inv[j][i];
        }
        std::cout << "),";
    }
    std::cout << std::endl << "        );" << std::endl;

}



}
// no name namespace



int main(int argc, char * argv[])
{
    // 17 is pretty much the best precision we can get here
    //
    std::cerr.precision(17);
    std::cout.precision(17);

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
            else if(strcmp(argv[i], "--hue-matrix") == 0)
            {
                ++i;
                if(i >= argc)
                {
                    std::cerr << "error: --hue expects a number used to select the luma." << std::endl;
                    return 1;
                }
                show_hue_matrix(std::stol(argv[i]));
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
