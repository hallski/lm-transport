/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "lm-channel.h"
#include "lm-resolver.h"
#include "lm-socket.h"

static gchar     *host;
static int        port;
static GMainLoop *loop;

#define MESSAGE "Hey there! Whazzup?"

static void
socket_connected (LmSocket *socket, int res, gpointer user_data)
{
        g_print ("Connect callback: %d\n", res);

        lm_channel_write (LM_CHANNEL (socket), 
                          MESSAGE, (gssize)strlen (MESSAGE), NULL, NULL);
}

int
main (int argc, char **argv)
{
        LmSocketAddress *sa;
        LmSocket        *socket;

        g_type_init ();

        if (argc < 3) {
                g_print ("Give a host and port\n");
                return 1;
        }

        host = g_strdup (argv[1]);
        port = atoi (argv[2]);

        g_print ("Connecting to %s:%d ...\n", host, port);

        sa = lm_socket_address_new (host, port);

        socket = lm_socket_new (NULL, sa);
        g_signal_connect (socket, "connect-result", 
                          G_CALLBACK (socket_connected),
                          NULL);
        lm_socket_connect (socket);

        loop = g_main_loop_new (NULL, FALSE);

        g_main_loop_run (loop);

        g_object_unref (socket);
   /*     lm_socket_address_unref (sa);*/

        return 0;
}




