#ifndef __ARGP_H
#define __ARGP_H

#include <stdio.h>
#include <errno.h>

struct argp_option
{
  const char *name;
  int key;
  const char *arg;
  int flags;
  const char *doc;
  int group;
};

typedef int error_t;
struct argp_state
{
  __const struct argp *root_argp;
  int argc;
  char **argv;
  int next;
  unsigned flags;
  unsigned arg_num;
  int quoted;
  void *input;
  void **child_inputs;
  void *hook;
  char *name;
  FILE *err_stream;		/* For errors; initialized to stderr. */
  FILE *out_stream;		/* For information; initialized to stdout. */
  void *pstate;			/* Private, for use by argp.  */
};
typedef error_t (*argp_parser_t) (int key, char *arg,
				  struct argp_state *state);
struct argp_child;
struct argp
{
  const struct argp_option *options;
  argp_parser_t parser;
  const char *args_doc;
  const char *doc;
  const struct argp_child *children;
  char *(*help_filter) (int __key, const char *__text, void *__input);
  const char *argp_domain;
};

#include <getopt.h>
#define ARGP_ERR_UNKNOWN	E2BIG
#define ARGP_KEY_ARGS		0x1000006
#define OPTION_DOC		0x8
#define ARGP_HELP_LONG		0x08 /* a long help message. */


error_t argp_parse (const struct argp *__restrict __argp,
			   int __argc, char **__restrict __argv,
			   unsigned __flags, int *__restrict __arg_index,
			   void *__restrict __input);
void argp_state_help(const struct argp_state *state, 
		FILE *stream, unsigned flags);


#endif	/* __ARGP_H */

