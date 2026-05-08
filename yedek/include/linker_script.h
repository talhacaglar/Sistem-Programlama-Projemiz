#ifndef LINKER_SCRIPT_H
#define LINKER_SCRIPT_H

#include "linker.h"

/* Parse a simple linker script file into LinkerConfig */
bool parse_linker_script(const char *path, LinkerConfig *config, ErrorList *errors);

#endif
