#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "html_parser.h"

// Helper to replace a substring
static char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance from beginning of source to rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // ...
    // next time through the loop, orig points to the end of the string
    // and ins points to the start of the new string ...
    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}


char *extract_text_from_html(const char *html_content) {
    if (html_content == NULL) {
        return NULL;
    }

    size_t len = strlen(html_content);
    if (len == 0) {
        char *empty_str = malloc(1);
        if (empty_str) empty_str[0] = '\0';
        return empty_str;
    }

    char *text_buffer = malloc(len + 1); // Allocate enough space, likely more than needed
    if (!text_buffer) {
        return NULL;
    }

    int in_tag = 0;
    int text_idx = 0;
    for (size_t i = 0; i < len; ++i) {
        if (html_content[i] == '<') {
            in_tag = 1;
            continue;
        }
        if (html_content[i] == '>') {
            in_tag = 0;
            continue;
        }
        if (!in_tag) {
            // Handle simple entities
            if (html_content[i] == '&' && i + 1 < len) {
                if (strncmp(&html_content[i], "&amp;", 5) == 0) {
                    text_buffer[text_idx++] = '&';
                    i += 4; continue;
                } else if (strncmp(&html_content[i], "&lt;", 4) == 0) {
                    text_buffer[text_idx++] = '<';
                    i += 3; continue;
                } else if (strncmp(&html_content[i], "&gt;", 4) == 0) {
                    text_buffer[text_idx++] = '>';
                    i += 3; continue;
                } else if (strncmp(&html_content[i], "&quot;", 6) == 0) {
                    text_buffer[text_idx++] = '"';
                    i += 5; continue;
                } else if (strncmp(&html_content[i], "&nbsp;", 6) == 0) {
                    text_buffer[text_idx++] = ' ';
                    i += 5; continue;
                }
                 // Add more entities if needed
            }
            text_buffer[text_idx++] = html_content[i];
        }
    }
    text_buffer[text_idx] = '\0';

    // Post-process to replace script and style tags content if any slipped through (basic)
    // This is a very naive removal and can be improved.
    char *temp;
    char *no_script_start = text_buffer;
    while ((temp = strstr(no_script_start, "<script")) != NULL) {
        char *end_script = strstr(temp, "</script>");
        if (end_script) {
            memmove(temp, end_script + strlen("</script>"), strlen(end_script + strlen("</script>")) + 1);
        } else {
            // No closing script tag, remove the rest of the string from here
            *temp = '\0';
            break;
        }
    }

    char *no_style_start = text_buffer;
    while ((temp = strstr(no_style_start, "<style")) != NULL) {
        char *end_style = strstr(temp, "</style>");
        if (end_style) {
            memmove(temp, end_style + strlen("</style>"), strlen(end_style + strlen("</style>")) + 1);
        } else {
            // No closing style tag, remove the rest of the string from here
            *temp = '\0';
            break;
        }
    }


    // The str_replace helper isn't used in this version of extract_text_from_html
    // but was part of the original thought process.
    // For a more robust entity decoding or tag stripping, it could be useful.

    // Trim the buffer to the actual size
    char *trimmed_buffer = realloc(text_buffer, text_idx + 1);
    if (!trimmed_buffer) {
        free(text_buffer); // If realloc fails, original buffer is still valid
        return NULL;       // Or return text_buffer if partial success is acceptable
    }

    return trimmed_buffer;
}
