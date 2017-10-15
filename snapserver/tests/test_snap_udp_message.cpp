// Snap Websites Server -- test the UDP messaging system
// Copyright (C) 2011-2017  Made to Order Software Corp.
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
// This test forks another process which becomes the UDP client. The
// client is expected to send a PING, a RSET and then a STOP message
// to the server. The server quits when it receives the STOP message.
//

/////////////////// Retired since we have snap_communicator now and that has its own tests

#include "snapwebsites.h"

int main(int argc, char *argv[])
{
    pid_t pid(fork());

    if(argc == 1)
    {
        fprintf(stderr, "Usage: test_snap_udp_message -c <config>\n");
        fprintf(stderr, "  where <config> is the same as what you'd use for your server but with the sendmail setup for this computer.\n");
        exit(1);
    }

    if(pid == 0)
    {
        // I'm the child, sleep a bit and then create a client
        sleep(3);
        snap::server::pointer_t s( snap::server::instance() );
        s->config(argc, argv);
        snap::snap_child *c(new snap::snap_child(s));
        c->udp_ping("sendmail_udp_signal"); // "PING" is the default
        sleep(1);
        c->udp_ping("sendmail_udp_signal", "RSET");
        sleep(1);
        c->udp_ping("sendmail_udp_signal", "STOP");
    }
    else
    {
        snap::server::pointer_t s( snap::server::instance() );
        s->config(argc, argv);
        snap::snap_child *c(new snap::snap_child(s));
        QSharedPointer<udp_client_server::udp_server> u(c->udp_get_server("sendmail_udp_signal"));
        bool got_ping(false);
        bool got_rset(false);
        for(;;)
        {
            char buf[5];
            buf[4] = '\0';
            int r(u->recv(buf, sizeof(buf)));
            if(r != 4)
            {
                fprintf(stderr, "error: received a message with size %d\n", r);
                exit(1);
            }
            if(strcmp(buf, "PING") == 0)
            {
                printf("server received PING\n");
                got_ping = true;
            }
            else if(strcmp(buf, "RSET") == 0)
            {
                printf("server received RSET\n");
                got_rset = true;
            }
            else if(strcmp(buf, "STOP") == 0)
            {
                printf("server received STOP\n");
                // we got the STOP message!
                break;
            }
        }
        if(!got_ping)
        {
            fprintf(stderr, "error: PING not received!\n");
            exit(1);
        }
        if(!got_rset)
        {
            fprintf(stderr, "error: RSET not received!\n");
            exit(1);
        }
    }

    exit(0);
}

// vim: ts=4 sw=4 et

