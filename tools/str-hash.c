#include <stdlib.h>
#include <glib.h>
#include "irc.h"

int
main (int argc, char **argv)
{
	if (argc != 2)
	{
		g_print ("Usage: %s <string>\n", argv[0]);
		return EXIT_FAILURE;
	}

	g_print("0x%xu\n", irc_str_hash (argv[1]));//g_str_hash(argv[1]));
	return EXIT_SUCCESS;
}
