#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"
#include "utils.h"

static void strip_comment(char *line)
{
    bool in_quote = false;
    for (size_t i = 0; line[i] != '\0'; ++i) {
        if (line[i] == '\'') {
            in_quote = !in_quote;
        }
        if (!in_quote && (line[i] == '#' || line[i] == ';')) {
            line[i] = '\0';
            break;
        }
    }
}

static bool parse_single_line(const char *input, int line_no, ParsedLine *out, ErrorList *errors)
{
    memset(out, 0, sizeof(*out));
    out->line_no = line_no;
    snprintf(out->original, sizeof(out->original), "%s", input);

    char buf[MAX_LINE_TEXT + 1];
    strncpy(buf, input, MAX_LINE_TEXT);
    buf[MAX_LINE_TEXT] = '\0';
    strip_comment(buf);
    trim(buf);

    if (buf[0] == '\0') {
        out->is_empty = true;
        return true;
    }

    char *rest = buf;
    char *colon = strchr(rest, ':');
    if (colon) {
        /* Check if colon is inside parentheses - if so, it's not a label separator */
        bool inside_paren = false;
        for (char *p = rest; p < colon; p++) {
            if (*p == '(') inside_paren = true;
            if (*p == ')') inside_paren = false;
        }
        if (!inside_paren) {
            *colon = '\0';
            trim(rest);
            if (strlen(rest) > MAX_LABEL_LEN) {
                errors_add(errors, line_no, "Label is too long.");
                return false;
            }
            snprintf(out->label, sizeof(out->label), "%s", rest);
            rest = colon + 1;
        }
    }

    trim(rest);
    if (*rest == '\0') {
        /* label-only line */
        return true;
    }

    char mnemonic[MAX_MNEMONIC_LEN + 1] = {0};
    size_t pos = 0;
    while (*rest && !isspace((unsigned char)*rest) && pos < MAX_MNEMONIC_LEN) {
        mnemonic[pos++] = *rest++;
    }
    mnemonic[pos] = '\0';
    trim(rest);
    to_lowercase(mnemonic);
    snprintf(out->mnemonic, sizeof(out->mnemonic), "%s", mnemonic);
    snprintf(out->operands, sizeof(out->operands), "%s", rest);
    out->is_directive = (mnemonic[0] == '.');
    return true;
}

bool parse_source_file(const char *path, LineVector *lines, ErrorList *errors)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        errors_add(errors, 0, "Unable to open source file: %s", path);
        return false;
    }

    char raw[MAX_LINE_TEXT + 1];
    int line_no = 1;
    while (fgets(raw, sizeof(raw), fp)) {
        size_t len = strlen(raw);
        while (len > 0 && (raw[len - 1] == '\n' || raw[len - 1] == '\r')) {
            raw[--len] = '\0';
        }
        ParsedLine parsed;
        if (!parse_single_line(raw, line_no, &parsed, errors)) {
            fclose(fp);
            return false;
        }
        line_vector_push(lines, &parsed);
        line_no++;
    }

    fclose(fp);
    return true;
}
