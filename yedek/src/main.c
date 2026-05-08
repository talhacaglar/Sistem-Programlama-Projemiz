#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "assembler.h"
#include "hex_writer.h"
#include "utils.h"
#include "opcodes.h"
#include "objfile.h"

static void usage(const char *prog)
{
    fprintf(stderr,
            "PicoRV32 RV32I Assembler\n"
            "Usage:\n"
            "  %s <input.s> <output.hex> [listing.lst]   Assemble to Intel HEX\n"
            "  %s -c <input.s> -o <output.o>             Assemble to object file\n"
            "  %s --dump <file.o>                        Dump object file contents\n"
            "  %s --opcodes                              List supported opcodes\n",
            prog, prog, prog, prog);
}

/* Mode: assemble to object file */
static int mode_object(const char *input_path, const char *output_path)
{
    LineVector lines;
    SymbolTable symbols;
    ErrorList errors = {0};

    line_vector_init(&lines);
    symbol_table_init(&symbols);

    if (!parse_source_file(input_path, &lines, &errors)) goto fail;

    ObjectFile obj;
    object_file_init(&obj);

    /* Extract filename for source_name */
    const char *basename = strrchr(input_path, '/');
    basename = basename ? basename + 1 : input_path;
    snprintf(obj.source_name, sizeof(obj.source_name), "%s", basename);

    if (!assemble_to_object(&lines, &symbols, &obj, &errors)) {
        object_file_free(&obj);
        goto fail;
    }

    if (!write_object_file(output_path, &obj)) {
        object_file_free(&obj);
        errors_add(&errors, 0, "Failed to write object file: %s", output_path);
        goto fail;
    }

    printf("Object file created: %s\n", output_path);
    printf("  .text : %u bytes\n", obj.text.size);
    printf("  .data : %u bytes\n", obj.data.size);

    object_file_free(&obj);
    symbol_table_free(&symbols);
    line_vector_free(&lines);
    return 0;

fail:
    for (size_t i = 0; i < errors.count; ++i)
        fprintf(stderr, "%s\n", errors.messages[i]);
    symbol_table_free(&symbols);
    line_vector_free(&lines);
    return 1;
}

/* Mode: dump object file */
static int mode_dump(const char *path)
{
    ObjectFile obj;
    if (!read_object_file(path, &obj)) {
        fprintf(stderr, "Failed to read object file: %s\n", path);
        return 1;
    }
    dump_object_file(&obj);
    object_file_free(&obj);
    return 0;
}

/* Mode: traditional assemble to HEX */
static int mode_hex(const char *input_path, const char *hex_path, const char *listing_path)
{
    LineVector lines;
    SymbolTable symbols;
    ErrorList errors = {0};
    uint32_t program_end = 0, entry = 0, min_used = 0, max_used = 0;

    line_vector_init(&lines);
    symbol_table_init(&symbols);

    if (!parse_source_file(input_path, &lines, &errors)) goto fail;
    if (!run_pass1(&lines, &symbols, &errors, &program_end)) goto fail;

    MemoryImage image;
    size_t image_size = (program_end + 0x1000u > 0x100000u) ? (size_t)(program_end + 0x1000u) : 0x100000u;
    memory_image_init(&image, image_size);

    if (!run_pass2(&lines, &symbols, &image, &errors, &entry, &min_used, &max_used)) {
        memory_image_free(&image); goto fail;
    }
    if (!write_intel_hex(hex_path, &image, min_used, max_used)) {
        memory_image_free(&image);
        errors_add(&errors, 0, "Failed to write HEX: %s", hex_path); goto fail;
    }
    if (listing_path && !write_listing(listing_path, &lines)) {
        memory_image_free(&image);
        errors_add(&errors, 0, "Failed to write listing: %s", listing_path); goto fail;
    }

    printf("Assembly successful.\n");
    printf("Entry point : 0x%08X\n", entry);
    printf("Used range  : 0x%08X - 0x%08X\n", min_used, max_used);
    printf("HEX output  : %s\n", hex_path);
    if (listing_path) printf("Listing     : %s\n", listing_path);

    memory_image_free(&image);
    symbol_table_free(&symbols);
    line_vector_free(&lines);
    return 0;

fail:
    for (size_t i = 0; i < errors.count; ++i)
        fprintf(stderr, "%s\n", errors.messages[i]);
    symbol_table_free(&symbols);
    line_vector_free(&lines);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--opcodes") == 0) {
        print_supported_opcodes();
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "--dump") == 0) {
        return mode_dump(argv[2]);
    }

    /* -c mode: picorv32asm -c input.s -o output.o */
    if (argc == 5 && strcmp(argv[1], "-c") == 0 && strcmp(argv[3], "-o") == 0) {
        return mode_object(argv[2], argv[4]);
    }

    /* Traditional mode: picorv32asm input.s output.hex [listing.lst] */
    if (argc >= 3 && argc <= 4) {
        const char *listing = (argc == 4) ? argv[3] : NULL;
        return mode_hex(argv[1], argv[2], listing);
    }

    usage(argv[0]);
    return 1;
}
