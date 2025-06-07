#define _GNU_SOURCE // For strcasestr
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // For strdup, strlen, etc.
#include <strings.h> // For strncasecmp, strcasestr
#include <ctype.h>
#include "html_parser.h" // Contains LinkInfo, LinkList declarations

// #define DEBUG_PARSER
#ifndef DEBUG_PARSER
#define LOG_DEBUG(format, ...) // No-op when not debugging
#endif

#define INITIAL_BUFFER_SIZE 4096
#define GROWTH_FACTOR 2
#define INITIAL_LINK_CAPACITY 10

// LinkList Helper Functions ----------------------------------------------------

void init_link_list(LinkList *list, int initial_capacity) {
    if (!list) return;
    list->links = malloc(initial_capacity * sizeof(LinkInfo));
    if (!list->links) {
        list->capacity = 0;
        list->count = 0;
        // fprintf(stderr, "HTML_PARSER_ERROR: Failed to allocate memory for links in init_link_list\n");
        return; // Or handle error more robustly
    }
    list->capacity = initial_capacity;
    list->count = 0;
}

void add_link(LinkList *list, const char *url, const char *text) {
    if (!list) return;

    if (list->count >= list->capacity) {
        int new_capacity = (list->capacity == 0) ? INITIAL_LINK_CAPACITY : list->capacity * GROWTH_FACTOR;
        LinkInfo *new_links = realloc(list->links, new_capacity * sizeof(LinkInfo));
        if (!new_links) {
            // fprintf(stderr, "HTML_PARSER_ERROR: Failed to reallocate memory for links in add_link\n");
            return; // Or handle error (e.g., don't add the link)
        }
        list->links = new_links;
        list->capacity = new_capacity;
    }

    list->links[list->count].url = strdup(url ? url : "");
    list->links[list->count].text = strdup(text ? text : "");

    // Check if strdup failed
    if (!list->links[list->count].url || !list->links[list->count].text) {
        // fprintf(stderr, "HTML_PARSER_ERROR: strdup failed in add_link\n");
        free(list->links[list->count].url); // free whatever was allocated
        list->links[list->count].url = NULL;
        free(list->links[list->count].text);
        list->links[list->count].text = NULL;
        // Don't increment count if add failed partially
        return;
    }
    list->count++;
}

void free_link_list(LinkList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        free(list->links[i].url);
        free(list->links[i].text);
    }
    free(list->links);
    list->links = NULL;
    list->count = 0;
    list->capacity = 0;
}

// Text Extraction Helper Functions (append_string, append_char, trim_trailing_whitespace) - unchanged from before
// ... (These functions would be here, identical to step 59's version) ...
// Helper to append string and handle buffer reallocation
static void append_string(char **buffer, size_t *buffer_size, size_t *current_len, const char *str_to_add) {
    size_t str_len = strlen(str_to_add);
    if (*current_len + str_len + 1 > *buffer_size) {
        size_t new_size = (*buffer_size + str_len + 1) * GROWTH_FACTOR;
        char *new_buffer = realloc(*buffer, new_size);
        if (!new_buffer) {
            /* fprintf(stderr, "HTML_PARSER_ERROR: Realloc failed in append_string for string: %.20s\n", str_to_add); */
            if (*buffer_size > 0) (*buffer)[*current_len] = '\0';
            return;
        }
        *buffer = new_buffer;
        *buffer_size = new_size;
    }
    strcat(*buffer, str_to_add);
    *current_len += str_len;
}

// Helper to append char and handle buffer reallocation
static void append_char(char **buffer, size_t *buffer_size, size_t *current_len, char char_to_add) {
    if (*current_len + 1 + 1 > *buffer_size) {
        size_t new_size = (*buffer_size + 2) * GROWTH_FACTOR;
        if (new_size < *buffer_size + 2) new_size = *buffer_size + 2;
        char *new_buffer = realloc(*buffer, new_size);
        if (!new_buffer) {
            /* fprintf(stderr, "HTML_PARSER_ERROR: Realloc failed in append_char\n"); */
            if (*buffer_size > 0) (*buffer)[*current_len] = '\0';
            return;
        }
        *buffer = new_buffer;
        *buffer_size = new_size;
    }
    (*buffer)[(*current_len)++] = char_to_add;
    (*buffer)[*current_len] = '\0';
}

// Helper to trim trailing whitespace from a string (modifies the string)
static void trim_trailing_whitespace(char *str) {
    if (str == NULL) return;
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}
// Main Parsing Function -----------------------------------------------------

char *extract_text_and_links_from_html(const char *html_content, LinkList *link_list) {
    if (html_content == NULL) return NULL;

    size_t len = strlen(html_content);
    char *text_buffer_local = NULL;
    size_t buffer_size_local = 0;
    size_t current_len_local = 0;

#ifdef DEBUG_PARSER
// ... (LOG_DEBUG definition would be here if active) ...
#else
#ifndef LOG_DEBUG
#define LOG_DEBUG(format, ...)
#endif
#endif

    if (len == 0) {
        char *empty_str = malloc(1);
        if (empty_str) empty_str[0] = '\0';
        init_link_list(link_list, 0); // Initialize even if no content
        return empty_str;
    }

    buffer_size_local = INITIAL_BUFFER_SIZE;
    text_buffer_local = malloc(buffer_size_local);
    if (!text_buffer_local) {
        init_link_list(link_list, 0);
        return NULL;
    }
    text_buffer_local[0] = '\0';

    // Initialize the passed-in link_list
    init_link_list(link_list, INITIAL_LINK_CAPACITY);

    int in_tag = 0;
    int in_script = 0;
    int in_style = 0;
    // int ol_counter = 0; // Removed in step 59/60

    // For link parsing
    int in_anchor_tag = 0;
    char current_href[2048]; // Buffer for href URL
    char current_link_text[1024]; // Buffer for link text
    size_t current_link_text_len = 0;


    LOG_DEBUG("Starting parse. Initial in_tag = %d. HTML len = %zu", in_tag, len);

    for (size_t i = 0; i < len; ++i) {
        LOG_DEBUG("Loop top: i=%zu, char='%c', in_tag=%d, in_script=%d, in_style=%d, current_len=%zu", i, html_content[i], in_tag, in_script, in_style, current_len_local);

        if (html_content[i] == '<') {
            LOG_DEBUG("Found '<' at i=%zu. Old in_tag=%d. Setting in_tag=1.", i, in_tag);
            in_tag = 1;

            LOG_DEBUG("At i=%zu, checking for tag: %.*s", i, 10, &html_content[i]);

            // Handle <a> tag opening specifically for link extraction
            if (strncasecmp(&html_content[i], "<a", 2) == 0) {
                LOG_DEBUG("Potential <a> tag start at i=%zu", i);
                in_anchor_tag = 1;
                current_href[0] = '\0';
                current_link_text[0] = '\0';
                current_link_text_len = 0;

                // Simple href parsing: look for href="...", href='...', or href=...
                const char *href_ptr = strcasestr(&html_content[i], "href=");
                if (href_ptr) {
                    href_ptr += 5; // Move past "href="
                    char quote_char = 0;
                    if (*href_ptr == '"' || *href_ptr == '\'') {
                        quote_char = *href_ptr;
                        href_ptr++;
                    }
                    size_t k = 0;
                    while (*href_ptr && k < sizeof(current_href) - 1) {
                        if (quote_char) { // If quoted
                            if (*href_ptr == quote_char) break;
                        } else { // Unquoted
                            if (isspace(*href_ptr) || *href_ptr == '>') break;
                        }
                        current_href[k++] = *href_ptr;
                        href_ptr++;
                    }
                    current_href[k] = '\0';
                    LOG_DEBUG("Extracted href: '%s'", current_href);
                }
                // Fall through to generic tag processing logic which will advance i and set in_tag=0
                // The generic <a> handler will just advance i to '>', set in_tag=0, and continue
            }
            // Handle </a> closing tag
            else if (in_anchor_tag && strncasecmp(&html_content[i], "</a>", 4) == 0) {
                LOG_DEBUG("Matched </a> tag at i=%zu", i);
                current_link_text[current_link_text_len] = '\0'; // Null-terminate link text
                if (strlen(current_href) > 0 && current_link_text_len > 0) {
                    LOG_DEBUG("Adding link: URL='%s', Text='%s'", current_href, current_link_text);
                    add_link(link_list, current_href, current_link_text);
                }
                in_anchor_tag = 0;
                current_link_text_len = 0; // Reset for next link
                // Standard closing tag logic: advance i to '>', set in_tag=0, continue
                i += (4-1); in_tag = 0; LOG_DEBUG("Processed </a>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
            }


            if (i + 7 < len && strncasecmp(&html_content[i], "<script", 7) == 0) {
                LOG_DEBUG("Matched <script>"); in_script = 1; i += (7-1); in_tag=0; LOG_DEBUG("Processed <script>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
            } else if (i + 6 < len && strncasecmp(&html_content[i], "<style", 6) == 0) {
                LOG_DEBUG("Matched <style>"); in_style = 1; i += (6-1); in_tag=0; LOG_DEBUG("Processed <style>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
            }
            else if (in_script && strncasecmp(&html_content[i], "</script>", 9) == 0) {
                in_script = 0; i += (9-1); in_tag = 0; LOG_DEBUG("Processed </script>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
            } else if (in_style && strncasecmp(&html_content[i], "</style>", 8) == 0) {
                in_style = 0; i += (8-1); in_tag = 0; LOG_DEBUG("Processed </style>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
            }

            if (in_script || in_style) {
                 LOG_DEBUG("Inside script/style content. Skipping char '%c'. Continuing.", html_content[i]);
                 continue;
            }

            if (html_content[i+1] == '/') { // Closing tags (other than script/style/a)
                LOG_DEBUG("Closing tag check: '%.*s'", 10, &html_content[i]);
                if (strncasecmp(&html_content[i], "</p>", 4) == 0) {
                    trim_trailing_whitespace(text_buffer_local); append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n\n");
                    i += (4-1); in_tag = 0; LOG_DEBUG("Processed </p>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "</h1>", 5) == 0) {
                    trim_trailing_whitespace(text_buffer_local); append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n\n");
                    i += (5-1); in_tag = 0; LOG_DEBUG("Processed </h1>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "</h2>", 5) == 0) {
                    trim_trailing_whitespace(text_buffer_local); append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n\n");
                    i += (5-1); in_tag = 0; LOG_DEBUG("Processed </h2>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "</li>", 5) == 0) { // </li> already handled by <a> if it's a link item
                    trim_trailing_whitespace(text_buffer_local); append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n");
                    i += (5-1); in_tag = 0; LOG_DEBUG("Processed </li>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "</ol>", 5) == 0) {
                    trim_trailing_whitespace(text_buffer_local); append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n");
                    i += (5-1); in_tag = 0; LOG_DEBUG("Processed </ol>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "</ul>", 5) == 0) {
                    trim_trailing_whitespace(text_buffer_local); append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n");
                    i += (5-1); in_tag = 0; LOG_DEBUG("Processed </ul>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else {
                    LOG_DEBUG("Unknown closing tag: '%.*s'", 10, &html_content[i]);
                    size_t temp_j = i; while(temp_j < len && html_content[temp_j] != '>') temp_j++; i = temp_j;
                    in_tag = 0; LOG_DEBUG("Processed unknown closing tag. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                }
            } else { // Opening tags (other than script/style/a)
                LOG_DEBUG("Opening tag check: '%.*s'", 10, &html_content[i]);
                 if (strncasecmp(&html_content[i], "<h1>", 4) == 0) {
                    append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "## ");
                    i+=(4-1); in_tag = 0; LOG_DEBUG("Processed <h1>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "<h2>", 4) == 0) {
                    append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "### ");
                    i+=(4-1); in_tag = 0; LOG_DEBUG("Processed <h2>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "<li>", 4) == 0) {
                    if(current_len_local > 0 && text_buffer_local[current_len_local-1] != '\n' && text_buffer_local[current_len_local-1] != ' ') { /* Heuristic for <li> spacing */ append_string(&text_buffer_local, &buffer_size_local, &current_len_local, " ");}
                    // No prefix for <li> based on simplified list handling
                    i+=(4-1); in_tag = 0; LOG_DEBUG("Processed <li>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "<ol>", 4) == 0) {
                    if(current_len_local > 0 && text_buffer_local[current_len_local-1] != '\n') { append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n");}
                    append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n");
                    i+=(4-1); in_tag = 0; LOG_DEBUG("Processed <ol>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "<ul>", 4) == 0) {
                    if(current_len_local > 0 && text_buffer_local[current_len_local-1] != '\n') { append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n");}
                    append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n");
                    i+=(4-1); in_tag = 0; LOG_DEBUG("Processed <ul>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "<br", 3) == 0) {
                    append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n");
                    size_t temp_i = i; while(temp_i < len && html_content[temp_i] != '>') temp_i++; i = temp_i;
                    in_tag = 0; LOG_DEBUG("Processed <br>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "<p>", 3) == 0) {
                    if(current_len_local > 0 && text_buffer_local[current_len_local-1] != '\n') {
                         append_string(&text_buffer_local, &buffer_size_local, &current_len_local, "\n");}
                    i+=(3-1); in_tag = 0; LOG_DEBUG("Processed <p>. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                } else if (strncasecmp(&html_content[i], "<a", 2) == 0) { // Already handled for href, just advance i
                     // The href parsing logic is above, this just ensures <a> tag itself is skipped
                     size_t temp_j = i; while(temp_j < len && html_content[temp_j] != '>') temp_j++; i = temp_j;
                     in_tag = 0; // Content of <a> will be handled by text appending logic
                     LOG_DEBUG("Processed <a> opening tag. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                }
                 else { // Unknown opening tag
                    LOG_DEBUG("Unknown opening tag: '%.*s'", 10, &html_content[i]);
                    size_t temp_j = i; while(temp_j < len && html_content[temp_j] != '>') temp_j++; i = temp_j;
                    in_tag = 0;
                    LOG_DEBUG("Processed unknown opening tag. New i=%zu. Set in_tag=0. Continuing.",i); continue;
                }
            }
        }
        else if (html_content[i] == '>') {
            LOG_DEBUG("Found standalone '>' at i=%zu. Old in_tag=%d. Setting in_tag=0.", i, in_tag);
            in_tag = 0; // This will correctly handle the '>' after <a> tag processed by href logic
        }
        else if (!in_tag && !in_script && !in_style) {
            // Append to main text buffer
            append_char(&text_buffer_local, &buffer_size_local, &current_len_local, html_content[i]);
            LOG_DEBUG("Appended char '%c' to main text. current_len=%zu", html_content[i], current_len_local);

            // If also inside an anchor tag, append to link text buffer
            if (in_anchor_tag) {
                if (current_link_text_len < sizeof(current_link_text) - 1) {
                    current_link_text[current_link_text_len++] = html_content[i];
                    current_link_text[current_link_text_len] = '\0'; // Keep it null-terminated
                    LOG_DEBUG("Appended char '%c' to link text. current_link_text='%s'", html_content[i], current_link_text);
                }
            }
        } else {
            LOG_DEBUG("Skipping char '%c' at i=%zu because in_tag=%d, in_script=%d, in_style=%d", html_content[i], i, in_tag, in_script, in_style);
        }
    }

    LOG_DEBUG("Finished loop. Finalizing. Current length: %zu", current_len_local);
    trim_trailing_whitespace(text_buffer_local);

    char *final_buffer = realloc(text_buffer_local, strlen(text_buffer_local) + 1);
    if (!final_buffer) {
        LOG_DEBUG("Final realloc to trim buffer failed. Returning original buffer.");
        // free_link_list(link_list); // Should free if parser fails overall
        return text_buffer_local;
    }
    LOG_DEBUG("Parser returning final_buffer. Final length after trim: %zu", strlen(final_buffer));
    return final_buffer;
}

#ifdef DEBUG_PARSER
#undef LOG_DEBUG
#endif
