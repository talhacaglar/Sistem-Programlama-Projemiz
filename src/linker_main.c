#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linker.h"
#include "linker_script.h"
#include "hex_writer.h"
#include "utils.h"

static void usage(const char *prog)
{
    fprintf(stderr,
        "PicoRV32 RV32I Linker\n"
        "Usage: %s [-T script.ld] -o output.hex input1.o input2.o ...\n"
        "Options:\n"
        "  -T script.ld    Linker script (memory layout)\n"
        "  -o output.hex   Output file (Intel HEX format)\n"
        "  --bin            Also output raw binary (.bin)\n",
        prog);
}

int main(int argc, char **argv)
{
    const char *script_path = NULL;
    const char *output_path = NULL;
    bool output_bin = false;
    const char *obj_paths[64];
    int obj_count = 0;

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-T") == 0 && i+1 < argc) {
            script_path = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i+1 < argc) {
            output_path = argv[++i];
        } else if (strcmp(argv[i], "--bin") == 0) {
            output_bin = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]); return 1;
        } else {
            if (obj_count >= 64) {
                fprintf(stderr, "Too many input files.\n"); return 1;
            }
            obj_paths[obj_count++] = argv[i];
        }
    }

    if (!output_path || obj_count == 0) {
        usage(argv[0]); return 1;
    }

    ErrorList errors = {0};
    LinkerConfig config;
    linker_config_init(&config);

    /* Parse linker script if provided */
    if (script_path) {
        if (!parse_linker_script(script_path, &config, &errors)) {
            for (size_t i = 0; i < errors.count; i++)
                fprintf(stderr, "%s\n", errors.messages[i]);
            return 1;
        }
    }

    /* Link */
    MemoryImage image;
    uint32_t entry_point, min_used, max_used;

    if (!linker_link(obj_count, obj_paths, &config, &image,
                     &entry_point, &min_used, &max_used, &errors)) {
        for (size_t i = 0; i < errors.count; i++)
            fprintf(stderr, "%s\n", errors.messages[i]);
        return 1;
    }

    /* Write Intel HEX */
    if (!write_intel_hex(output_path, &image, min_used, max_used)) {
        fprintf(stderr, "Failed to write HEX: %s\n", output_path);
        memory_image_free(&image);
        return 1;
    }
    printf("Output HEX  : %s\n", output_path);

    /* Optionally write raw binary */
    if (output_bin) {
        char bin_path[512];
        snprintf(bin_path, sizeof(bin_path), "%s", output_path);
        char *dot = strrchr(bin_path, '.');
        if (dot) strcpy(dot, ".bin");
        else snprintf(bin_path + strlen(bin_path), sizeof(bin_path) - strlen(bin_path), ".bin");

        FILE *fp = fopen(bin_path, "wb");
        if (fp) {
            fwrite(image.data + min_used, 1, max_used - min_used + 1, fp);
            fclose(fp);
            printf("Output BIN  : %s\n", bin_path);
        }
    }

    memory_image_free(&image);
    return 0;
}
