#include "argp.h"

/* Poor man's argp_parse() */

error_t argp_parse (const struct argp *__restrict __argp,
			   int __argc, char **__restrict __argv,
			   unsigned __flags, int *__restrict __arg_index,
			   void *__restrict __input)
{
	char *o, opts[20];
	const struct argp_option *ao;
	int opt;
	struct argp_state state = { 0 };

	for (o = &opts[0], ao = __argp->options; 
		(ao != NULL) && (ao->name != NULL); o++, ao++) {
		*o = (char)(ao->key);
		if (ao->arg != NULL)
			*++o = ':';
	}
	*++o = '0';
	state.root_argp = __argp;
	state.name = __argv[0];
	state.argv = __argv;
	state.argc = __argc;
	state.next = 0;
	while ((opt = getopt(__argc, __argv, opts)) != EOF) {
		if (__argp->parser(opt, optarg, &state) == ARGP_ERR_UNKNOWN) {
			fprintf(stderr, "Unknown option %c.\n", 
				(char)optopt);
			argp_state_help(&state, stderr, ARGP_HELP_LONG);
			return ARGP_ERR_UNKNOWN;
		}
	}
	state.next = optind;
	__argp->parser(ARGP_KEY_ARGS, optarg, &state);
	return 0;
}

void argp_state_help(const struct argp_state *state, 
		FILE *stream, unsigned flags)
{
	struct argp_option *option = 
		(struct argp_option *) state->root_argp->options;
	fprintf(stream, "Usage: %s [OPTION...]\n", state->name);
	fprintf(stream, "%s\n", state->root_argp->doc);
	while (option->name != NULL) {
		if (option->flags & OPTION_DOC) {
			fprintf(stream, "\t%s\t\t%s\n", 
				option->name, option->doc);
		} else if (option->arg != NULL) {
			fprintf(stream, "\t-%c, --%s=%s\t%s\n",
				(char)(option->key), option->name,
				option->arg, option->doc);
		} else {
			fprintf(stream, "\t-%c, --%s\t%s\n",
				(char)(option->key), option->name,
				option->doc);
		}
		option++;
	}
}
