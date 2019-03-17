//===============================================================================
// Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
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
//===============================================================================

#include "MainWindow.h"
#include "SettingsDialog.h"

// libQtCassandra lib
//
#include <libdbproxy/qstring_stream.h>

using namespace casswrapper;

int main( int argc, char * argv[] )
{
    QApplication app(argc, argv);
    app.setApplicationName    ( "cassview"           );
    app.setApplicationVersion ( CASSVIEW_VERSION     );
    app.setOrganizationDomain ( "snapwebsites.org"   );
    app.setOrganizationName   ( "M2OSW"              );
    app.setWindowIcon         ( QIcon(":icons/icon") );

    bool show_settings = true;
    do
    {
        SettingsDialog dlg( nullptr, true /*first_time*/ );
        QSettings settings;
        if( settings.contains( "cassandra_host") )
        {
            show_settings = !dlg.tryConnection();
        }

        if( show_settings )
        {
            if( dlg.exec() != QDialog::Accepted )
            {
                qDebug() << "User abort!";
                exit( 1 );
            }
        }
    }
    while( show_settings );

    try
    {
        MainWindow win;
        win.show();

        return app.exec();
    }
    catch(casswrapper::cassandra_exception_t const & e)
    {
        std::cerr << "cassview: A casswrapper exception occurred: " << e.what() << std::endl;
        std::cerr << "cassview: Stack trace: " << std::endl;
        for( auto const & stack_line : e.get_stack_trace() )
        {
            std::cerr << "cassview: " << stack_line << std::endl;
        }
        std::cerr << "cassview: End stack trace!" << std::endl;
        exit(1);
    }
    catch(libexcept::exception_t const & e)
    {
        std::cerr << "cassview: A library exception occurred: " << e.what() << std::endl;
        std::cerr << "cassview: Stack trace: " << std::endl;
        for( auto const & stack_line : e.get_stack_trace() )
        {
            std::cerr << "cassview: " << stack_line << std::endl;
        }
        std::cerr << "cassview: End stack trace!" << std::endl;
        exit(1);
    }

    return 0;
}

// vim: ts=4 sw=4 et
