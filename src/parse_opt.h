#ifndef PARSE_OPT_H
# define PARSE_OPT_H

struct opt {
	char **argv;
	int argc;
};

struct opt parse_cmd
	(const char *s);

#endif /* PARSE_OPT_H */
