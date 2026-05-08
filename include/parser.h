#ifndef PARSER_H
#define PARSER_H

#include "common.h"

bool parse_source_file(const char *path, LineVector *lines, ErrorList *errors);

#endif
