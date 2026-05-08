#ifndef OBJFILE_H
#define OBJFILE_H

#include "common.h"

/* Object file magic and version */
#define OBJ_MAGIC "RV32OBJ\0"
#define OBJ_MAGIC_LEN 8
#define OBJ_VERSION 1

/* Initialize / free an ObjectFile */
void object_file_init(ObjectFile *obj);
void object_file_free(ObjectFile *obj);

/* Relocation list helpers */
void reloc_list_init(RelocationList *rl);
void reloc_list_push(RelocationList *rl, const RelocationEntry *entry);
void reloc_list_free(RelocationList *rl);

/* Write an object file to disk in custom binary format */
bool write_object_file(const char *path, const ObjectFile *obj);

/* Read an object file from disk */
bool read_object_file(const char *path, ObjectFile *obj);

/* Dump object file contents to stdout (for debugging) */
void dump_object_file(const ObjectFile *obj);

#endif
