#define _GNU_SOURCE // For strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "http_client.h"
#include "html_parser.h"

#define MAX_URL_LENGTH 2048
#define INITIAL_LINK_CAPACITY 10

// History Stack Implementation
#define MAX_HISTORY_SIZE 20
static char *history_stack[MAX_HISTORY_SIZE];
static int history_top = -1;

// Debug print utility
// #define MAIN_DEBUG
#ifdef MAIN_DEBUG
#define printf_debug(fmt, ...) fprintf(stderr, "MAIN_DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define printf_debug(fmt, ...)
#endif

void init_history() {
    history_top = -1;
    for (int i = 0; i < MAX_HISTORY_SIZE; ++i) {
        if (history_stack[i] != NULL) {
            free(history_stack[i]);
            history_stack[i] = NULL;
        }
    }
    printf_debug("History initialized.");
}

int push_to_history(const char *url) {
    if (!url || strlen(url) == 0) {
        printf_debug("Attempted to push empty or NULL URL to history.");
        return 0;
    }
    // Avoid pushing the same URL if it's already at the top
    if (history_top >= 0 && history_stack[history_top] != NULL && strcmp(history_stack[history_top], url) == 0) {
        printf_debug("URL %s is already at top of history. Not pushing.", url);
        return 1;
    }
    if (history_top >= MAX_HISTORY_SIZE - 1) {
        fprintf(stderr, "History stack full. Cannot add: %s\n", url);
        return 0;
    }
    history_top++;
    history_stack[history_top] = strdup(url);
    if (history_stack[history_top] == NULL) {
        fprintf(stderr, "MAIN_ERROR: Failed to strdup URL for history.\n");
        history_top--;
        return 0;
    }
    printf_debug("Pushed to history: %s (top: %d)", url, history_top);
    return 1;
}

char *pop_from_history() {
    if (history_top < 0) {
        printf_debug("Pop from history: Stack is empty.");
        return NULL;
    }
    char *url = history_stack[history_top];
    history_stack[history_top] = NULL;
    history_top--;
    printf_debug("Popped from history: %s (new top: %d)", url ? url : "NULL", history_top);
    return url;
}

void free_history() {
    printf_debug("Freeing history stack (top: %d):", history_top);
    for (int i = 0; i < MAX_HISTORY_SIZE; ++i) { // Iterate through the whole array
        if (history_stack[i]) {
            printf_debug("  Freeing history[%d]: %s", i, history_stack[i]);
            free(history_stack[i]);
            history_stack[i] = NULL;
        }
    }
    history_top = -1;
}

int is_numeric(const char *str) {
    if (str == NULL || *str == '\0') return 0;
    const char* p = str;
    while (*p) {
        if (!isdigit(*p)) return 0;
        p++;
    }
    return (p != str);
}

void process_page_and_update_current(const char* url_to_process,
                                     char* current_page_url_dest_buffer,
                                     LinkList* page_links_ptr)
{
    printf_debug("Processing page: %s", url_to_process);

    strncpy(current_page_url_dest_buffer, url_to_process, MAX_URL_LENGTH - 1);
    current_page_url_dest_buffer[MAX_URL_LENGTH - 1] = '\0';

    printf("\nFetching URL: %s\n", current_page_url_dest_buffer);
    char *html_content = fetch_url(current_page_url_dest_buffer);

    if (page_links_ptr->links) {
        free_link_list(page_links_ptr); // Free links from the previous page
    }
    init_link_list(page_links_ptr, INITIAL_LINK_CAPACITY);

    if (html_content) {
        char *plain_text = extract_text_and_links_from_html(html_content, page_links_ptr);

        printf("\n--- Page Content (%s) ---\n", current_page_url_dest_buffer);
        if (plain_text) {
            printf("%s", plain_text);
            free(plain_text);
        } else {
            fprintf(stderr, "Could not parse HTML from %s.\n", current_page_url_dest_buffer);
        }
        printf("\n--- End of Content ---\n");

        if (page_links_ptr->count > 0) {
            printf("\n--- Extracted Links (%s) ---\n", current_page_url_dest_buffer);
            for (int i = 0; i < page_links_ptr->count; i++) {
                printf("[%d] %s (URL: %s)\n", i + 1, page_links_ptr->links[i].text, page_links_ptr->links[i].url);
            }
        } else {
            printf("\n--- No Links Extracted (%s) ---\n", current_page_url_dest_buffer);
        }
        free(html_content);
    } else {
        fprintf(stderr, "Could not fetch URL: %s.\n", current_page_url_dest_buffer);
        // If fetch fails, current_page_url_dest_buffer still holds the failed URL.
        // The history should ideally not include this failed URL as a "successfully visited" page.
        // The current logic pushes to history *before* attempting fetch. This might need refinement.
        // For now, we accept that a failed fetch might be in history.
    }
}

typedef enum { ACTION_NAVIGATE, ACTION_SELECT_LINK, ACTION_BACK, ACTION_EXIT } UserActionType;
typedef struct {
    UserActionType type;
    char* value;
} SimulatedAction;

int main(int argc, char *argv[]) {
    char current_page_url[MAX_URL_LENGTH] = "";
    char *url_to_fetch_next = NULL;
    LinkList page_links;

    init_history();
    page_links.links = NULL; page_links.count = 0; page_links.capacity = 0;

    // Initial URL
    if (argc > 1 && strncmp(argv[1], "--", 2) != 0) {
        url_to_fetch_next = strdup(argv[1]);
    } else {
        url_to_fetch_next = strdup("http://example.com"); // Default starting page
    }
    if (!url_to_fetch_next) {
        fprintf(stderr, "MAIN_ERROR: Failed to allocate initial URL.\n");
        return 1;
    }

    SimulatedAction actions[] = {
        // Initial page is already in url_to_fetch_next
        {ACTION_SELECT_LINK, "1"},      // Select link 1 from example.com (to iana.org) -> Page B
        {ACTION_NAVIGATE, "http://info.cern.ch"},     // Page C
        {ACTION_BACK, NULL},            // Should go to Page B (iana.org)
        {ACTION_BACK, NULL},            // Should go to Page A (example.com)
        {ACTION_BACK, NULL},            // Should say "No history"
        {ACTION_NAVIGATE, "http://example.com/pageD"}, // Page D
        {ACTION_BACK, NULL},            // Should go to Page A (example.com)
        {ACTION_NAVIGATE, "http://example.com/pageE"}, // Page E
        {ACTION_BACK, NULL},            // Should go to Page D (pageD was visited before A was restored by back)
                                        // Corrected expectation: back from E should go to D.
        {ACTION_EXIT, NULL}
    };
    int num_actions = sizeof(actions) / sizeof(actions[0]);
    int current_action_idx = 0;

    while(1) {
        if (url_to_fetch_next) {
            process_page_and_update_current(url_to_fetch_next, current_page_url, &page_links);
            if (url_to_fetch_next) free(url_to_fetch_next); // Free the strdup'd URL
            url_to_fetch_next = NULL;
        }

        if (current_action_idx >= num_actions) break; // All scripted actions done

        SimulatedAction current_action = actions[current_action_idx++];
        printf_debug("Next action: type=%d, value=%s", current_action.type, current_action.value ? current_action.value : "N/A");

        switch (current_action.type) {
            case ACTION_NAVIGATE:
                printf("\nSimulated action: navigate to %s\n", current_action.value);
                if (strlen(current_page_url) > 0 && strcmp(current_page_url, current_action.value) != 0) {
                    push_to_history(current_page_url);
                }
                url_to_fetch_next = strdup(current_action.value);
                if (!url_to_fetch_next) { fprintf(stderr, "MAIN_ERROR: strdup failed for navigate URL.\n"); goto cleanup_and_exit; }
                break;

            case ACTION_SELECT_LINK:
                printf("\nSimulated action: select link number %s\n", current_action.value);
                if (is_numeric(current_action.value)) {
                    int link_num = atoi(current_action.value);
                    if (link_num > 0 && link_num <= page_links.count) {
                        char* chosen_rel_url = page_links.links[link_num - 1].url;
                        char* resolved_url = resolve_url(current_page_url, chosen_rel_url);
                        if (resolved_url) {
                            if (strlen(current_page_url) > 0 && strcmp(current_page_url, resolved_url) != 0) {
                                push_to_history(current_page_url);
                            }
                            url_to_fetch_next = resolved_url; // resolve_url already strdup's
                            printf("Navigating to link %d: %s (from %s)\n", link_num, url_to_fetch_next, chosen_rel_url);
                        } else {
                            printf("Could not resolve link: %s\n", chosen_rel_url);
                        }
                    } else {
                        printf("Invalid link number %d (max %d)\n", link_num, page_links.count);
                    }
                } else {
                    printf("Invalid link selection value: %s\n", current_action.value);
                }
                break;

            case ACTION_BACK:
                printf("\nSimulated action: back\n");
                char *prev_url = pop_from_history();
                if (prev_url) {
                    url_to_fetch_next = prev_url; // Already strdup'd
                } else {
                    printf("No history to go back to.\n");
                }
                break;

            case ACTION_EXIT:
                printf("\nSimulated action: exit\n");
                goto cleanup_and_exit; // Use goto to break from outer loop
        }
    }

cleanup_and_exit:
    free_history();
    if (page_links.links) {
        free_link_list(&page_links);
    }
    if (url_to_fetch_next) { // If loop broken while url_to_fetch_next was set
        free(url_to_fetch_next);
    }

    printf("\n--- End of Scripted Session ---\n");
    return 0;
}
