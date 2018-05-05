<?php

function product($a, $b)
{
    $sum = "";

    if(strpos($a, ' ') !== FALSE)
    {
        $sum .= "(" . $a . ")";
    }
    else
    {
        if(is_numeric($a))
        {
            $sum .= number_format($a, 17, '.', '');
        }
        else
        {
            $sum .= $a;
        }
    }
    $sum .= " * ";
    if(strpos($b, ' ') !== FALSE)
    {
        $sum .= "(" . $b . ")";
    }
    else
    {
        if(is_numeric($b))
        {
            $sum .= number_format($b, 17, '.', '');
        }
        else
        {
            $sum .= $b;
        }
    }

    return $sum;
}


function mult($a, $b, $optimze)
{
    $c = array(
            array( 1, 0, 0, 0),
            array( 0, 1, 0, 0),
            array( 0, 0, 1, 0),
            array( 0, 0, 0, 1)
        );

    for($j = 0; $j < 4; ++$j)
    {
        $joffset = $j * 4;
        for($i = 0; $i < 4; ++$i)
        {
            $sum = 0;
            for($k = 0; $k < 4; ++$k)
            {
//                    sum += f_vector[k + joffset]
//                       * m.f_vector[i + k * m.f_columns];
                if(!$optimze)
                {
                    if($sum)
                    {
                        $sum .= " + ";
                    }
                    else if($sum == 0)
                    {
                        $sum = "";
                    }
                    $sum .= product($a[$j][$k], $b[$k][$i]);
                }
                else
                {
                    if(is_numeric($a[$j][$k])
                    && is_numeric($b[$k][$i]))
                    {
                        $product = $a[$j][$k] * $b[$k][$i];
//echo "products = ", $product, " a * b = ", $a[$j][$k], " * ", $b[$k][$i], "\n";
                        if($product != 0)
                        {
                            if(is_numeric($sum))
                            {
                                $sum += $product;
                            }
                            else
                            {
                                if($sum)
                                {
                                    if($product < 0)
                                    {
                                        $sum .= " - ";
                                        $product = -$product;
                                    }
                                    else
                                    {
                                        $sum .= " + ";
                                    }
                                }
                                $sum .= number_format($product, 17, '.', '');
                            }
                        }
                    }
                    else
                    {
                        if((is_numeric($a[$j][$k]) && $a[$j][$k] != 0)
                        || (is_numeric($b[$k][$i]) && $b[$k][$i] != 0))
                        {
                            if($sum)
                            {
                                $sum .= " + ";
                            }
                            else if($sum == 0)
                            {
                                $sum = "";
                            }
                            $sum .= product($a[$j][$k], $b[$k][$i]);
                        }
                    }
                }
            }
            $c[$j][$i] = $sum;
        }
    }

    return $c;
}



//        for(size_type j(0); j < f_rows; ++j)
//        {
//            size_type const joffset(j * f_columns);
//            for(size_type i(0); i < m.f_columns; ++i)
//            {
//                value_type sum = value_type();
//
//                // k goes from 0 to (f_columns == m.f_rows)
//                //
//                for(size_type k(0); k < m.f_rows; ++k)     // sometimes it's X and sometimes it's Y
//                {
//                    sum += f_vector[k + joffset]
//                       * m.f_vector[i + k * m.f_columns];
//                }
//                t.f_vector[i + j * t.f_columns] = sum;
//            }
//        }

// to get the $m and $m_inv matrices run the following:
//
//     ../../BUILD/snapwebsites/libsnapwebsites/tests/mb/test_color_matrix --hue-matrix 0..4
//
// The number 0 to 4 defines which luma factors you want to use.
//   0 -- HDTV
//   1 -- LED
//   2 -- CRT
//   3 -- NTSC
//   4 -- AVERAGE
//
// The first I used was the CRT factors.
//

// HDTV (0)
function hdtv_matrices(&$m, &$m_inv)
{
    $m = array(
            array( 0.81649658092772615,  0,                   0.25825325596814908, 0),
            array(-0.40824829046386302,  0.70710678118654746, 0.14560526009227781, 0),
            array(-0.40824829046386302, -0.70710678118654746, 1.3281922915084508,  0),
            array( 0,                    0,                   0,                   1),
        );
    $m_inv = array(
            array(1.0421322428330055,  -0.18261262855858337, -0.18261262855858337, 0),
            array(0.48278913390038725,  1.1898959150869348,  -0.2243176472861603,  0),
            array(0.57735026918962562,  0.57735026918962562,  0.57735026918962562, 0),
            array(0,                    0,                    0,                   1),
        );
}


// LED (1)
function led_matrices(&$m, &$m_inv)
{
    $m = array(
            array( 0.81649658092772615,  0,                   0.23459617558603596, 0),
            array(-0.40824829046386302,  0.70710678118654746, 0.13816744507720524, 0),
            array(-0.40824829046386302, -0.70710678118654746, 1.3592871869056362,  0),
            array( 0,                    0,                   0,                   1),
        );
    $m_inv = array(
            array(1.0588603247942729,  -0.16588454659731602, -0.16588454659731602, 0),
            array(0.49852004705313063,  1.2056268282396783,  -0.20858673413341691, 0),
            array(0.57735026918962573,  0.57735026918962584,  0.57735026918962584, 0),
            array(0,                    0,                    0,                   1),
        );
}


// CRT (2)
function crt_matrices(&$m, &$m_inv)
{
    $m = array(
            array( 0.81649658092772615,  0,                   0.097737296040753485, 0),
            array(-0.40824829046386302,  0.70710678118654746, 0.32831470544894448,  0),
            array(-0.40824829046386302, -0.70710678118654746, 1.3059988060791796,   0),
            array( 0,                    0,                   0,                    1)
        );
    $m_inv = array(
            array( 1.1556341665863352, -0.069110704805253872, -0.069110704805253872, 0),
            array( 0.399137862695993,   1.1062446438825406,   -0.30796891849055463,  0),
            array( 0.57735026918962562, 0.57735026918962562,   0.57735026918962562,  0),
            array( 0,                   0,                     0,                    1)
        );
}


// NTSC (3)
function ntsc_matrices(&$m, &$m_inv)
{
    $m = array(
            array( 0.81649658092772615,  0,                   0.040404129069193218, 0),
            array(-0.40824829046386302,  0.70710678118654746, 0.30749174724713957,  0),
            array(-0.40824829046386302, -0.70710678118654746, 1.3841549312525445,   0),
            array( 0,                    0,                   0,                    1),
        );
    $m_inv = array(
            array(1.1961748377388259,  -0.028570033652763036, -0.028570033652763033, 0),
            array(0.4395459042755861,   1.1466526854621337,   -0.26756087691096142,  0),
            array(0.57735026918962573,  0.57735026918962584,   0.57735026918962584,  0),
            array(0,                    0,                     0,                    1),
        );
}


// AVERAGE (4)
function average_matrices(&$m, &$m_inv)
{
    $m = array(
            array(0.81649658092772615, 0, -1.5380048024607857, 0),
            array(-0.40824829046386302, 0.70710678118654746, 0.48341511859422825, 0),
            array(-0.40824829046386302, -0.70710678118654746, 2.7866404914354348, 0),
            array(0, 0, 0, 1),
        );
    $m_inv = array(
            array(2.3122784967090859, 1.0875336253174974, 1.0875336253174974, 0),
            array(0.94028782101541564, 1.6473946022019634, 0.23318103982886837, 0),
            array(0.57735026918962518, 0.57735026918962551, 0.57735026918962551, 0),
            array(0, 0, 0, 1),
        );
}


function compute($optimze, $weights)
{
    $m = array();
    $m_inv = array();

    switch($weights)
    {
    case "hdtv":
        hdtv_matrices($m, $m_inv);
        break;

    case "led":
        led_matrices($m, $m_inv);
        break;

    case "crt":
        crt_matrices($m, $m_inv);
        break;

    case "ntsc":
        ntsc_matrices($m, $m_inv);
        break;

    case "average":
        average_matrices($m, $m_inv);
        break;

    default:
        echo "error: unknown luma weights $weights\n";
        exit(1);

    }

    $r = array(
            array( "rot_cos",  "rot_sin", 0, 0),
            array( "-rot_sin", "rot_cos", 0, 0),
            array( 0,              0,             1, 0),
            array( 0,              0,             0, 1)
        );

    $temp = mult($m, $r, $optimze);

    $h = mult($temp, $m_inv, $optimze);

    //$a = array(
    //        array( "0.81649658092772615 cos(%theta)", "0.81649658092772615 sin(%theta)",                                                                       0.097737296040753485, 0),
    //        array( "-0.40824829046386302 cos(%theta) - 0.70710678118654746 sin(%theta)", "-0.40824829046386302 sin(%theta) + 0.70710678118654746 cos(%theta)", 0.32831470544894448,  0),
    //        array( "-0.40824829046386302 cos(%theta) + 0.70710678118654746 sin(%theta)", "-0.40824829046386302 sin(%theta) - 0.70710678118654746 cos(%theta)", 1.3059988060791796,   0),
    //        array( 0,                    0,                   0,                    1)
    //    );
    //
    //$b = array(
    //    );

    //$a = array(
    //        array(
    //            "0.94357134582097624 cos(%theta) + 0.32589470021007605 sin(%theta) + 0.056428654178995",
    //            "-0.056428654178995265 cos(%theta) + 0.90324496939967125 sin(%theta) + 0.056428654178995",
    //            "-0.056428654178995265 cos(%theta) - 0.25145556897954369 sin(%theta) + 0.056428654178995",
    //            0),
    //        array(
    //            "-0.1260152011232234 cos(%theta) - 1.2254050462279 sin(%theta) - 0.40824829046386302 sin(%theta) + 0.18955258356986",
    //            "0.37398479887675007 cos(%theta) - 0.45711693848423984 sin(%theta) + 0.18955258356986",
    //            "-0.62601520112321807 cos(%theta) - 0.35937964244348619 sin(%theta) + 0.18955258356986",
    //            0),
    //        array( "-0.40824829046386302 cos(%theta) + 0.70710678118654746 sin(%theta)", "-0.40824829046386302 sin(%theta) - 0.70710678118654746 cos(%theta)", 1.3059988060791796,   0),
    //        array( 0,                    0,                   0,                    1)
    //    );
    //
    //$b = array(
    //        array( 1.1556341665863352, -0.069110704805253872, -0.069110704805253872, 0),
    //        array( 0.399137862695993,   1.1062446438825406,   -0.30796891849055463,  0),
    //        array( 0.57735026918962562, 0.57735026918962562,   0.57735026918962562,  0),
    //        array( 0,                   0,                     0,                    1)
    //    );






    //echo "---------------------------------------- Temp (R_rg x S x R_b)\n";
    //var_dump($temp);

    echo "---------------------------------------- Hue Matrix\n";
    //var_dump($h);

    echo "            hue_matrix[0][0] = ", $h[0][0], ";\n";
    echo "            hue_matrix[0][1] = ", $h[0][1], ";\n";
    echo "            hue_matrix[0][2] = ", $h[0][2], ";\n";
    echo "\n";
    echo "            hue_matrix[1][0] = ", $h[1][0], ";\n";
    echo "            hue_matrix[1][1] = ", $h[1][1], ";\n";
    echo "            hue_matrix[1][2] = ", $h[1][2], ";\n";
    echo "\n";
    echo "            hue_matrix[2][0] = ", $h[2][0], ";\n";
    echo "            hue_matrix[2][1] = ", $h[2][1], ";\n";
    echo "            hue_matrix[2][2] = ", $h[2][2], ";\n";
    echo "\n";

            //hue_matrix[0][0] =  0.86455583487457 * c + 0.32589470021007605 * s + 0.056428654178995;
            //hue_matrix[0][1] = -0.056428654178995265 * c + 0.90324496939967125 * s + 0.056428654178995;
            //hue_matrix[0][2] = -0.056428654178995265 * c - 0.25145556897954369 * s + 0.056428654178995;

            //hue_matrix[1][0] = -0.189552583569840000 * c - 0.98010410586906000 * s + 0.189552583569860;
            //hue_matrix[1][1] =  0.810447416430107000 * c - 0.40275383667945400 * s + 0.189552583569860;
            //hue_matrix[1][2] = -0.189552583569853000 * c + 0.17459643251014600 * s + 0.189552583569860;

            //hue_matrix[2][0] = -0.754018762251120000 * c + 0.65420940565900000 * s + 0.754018762251160;
            //hue_matrix[2][1] = -0.754018762251113000 * c - 0.50049113272020600 * s + 0.754018762251160;
            //hue_matrix[2][2] =  0.245981237748847000 * c + 0.07685913646939400 * s + 0.754018762251160;


}

if($argc != 2)
{
    echo "error: which weights need to be used must be specified, one of:\n";
    echo "  hdtv\n";
    echo "  led\n";
    echo "  crt\n";
    echo "  ntsc\n";
    echo "  average\n";
    exit(1);
}

compute(false, $argv[1]);
compute(true,  $argv[1]);


// vim: ts=4 sw=4 et
