/*
 * Copyright (C) 2008 Mikael Hallendal <micke@imendio.com>
 */

#include <glib.h>

#include "lm-resolver.h"

static gchar     *domain;
static GMainLoop *loop;

static void
resolver_finished_cb (LmResolver       *resolver, 
                      LmResolverResult  result,
                      LmSocketAddress  *sa,
                      gpointer          user_data)
{
        g_print ("Looked up service 'xmpp-client' for domain '%s'\n", domain);
        g_print ("Found it point to server '%s' on port '%d'\n",
                 lm_socket_address_get_host (sa),
                 lm_socket_address_get_port (sa));

        g_free (domain);

        g_main_loop_quit (loop);
}

int
main (int argc, char **argv)
{
        LmResolver *resolver;

        g_type_init ();

        if (argc < 2) {
                g_print ("Give a domain\n");
                return 1;
        }

        domain = g_strdup (argv[1]);

        g_print ("Looking up %s ...\n", domain);

        resolver = lm_resolver_lookup_service (NULL,
                                               domain,
                                               LM_RESOLVER_SRV_XMPP_CLIENT);
        g_signal_connect (resolver, "finished", G_CALLBACK (resolver_finished_cb),
                          NULL);

        loop = g_main_loop_new (NULL, FALSE);

        g_main_loop_run (loop);

        return 0;
}



