/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#include <glib.h>

#include "lm-socket.h"

int
main (int argc, char **argv)
{
        LmSocket        *socket;
        LmSocketAddress *sa;
        GMainLoop       *loop;

        g_type_init ();

        if (argc < 2) {
                g_print ("Give a host\n");
                return 1;
        }

        g_print ("Connecting to %s ...\n", argv[1]);
        
        sa = lm_socket_address_new (argv[1], 5222);
        socket = lm_socket_new (sa, NULL);
        
        loop = g_main_loop_new (NULL, FALSE);

        lm_socket_connect (socket);

        g_main_loop_run (loop);

        return 0;
}



