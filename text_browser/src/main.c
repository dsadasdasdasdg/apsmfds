#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_client.h"
#include "html_parser.h"

#define MAX_URL_LENGTH 2048

int main(int argc, char *argv[]) {
    char url_buffer[MAX_URL_LENGTH];

    // If a URL is provided as a command-line argument, process it directly
    if (argc > 1) {
        printf("Processing URL from command line: %s\n", argv[1]);
        char *html_content = fetch_url(argv[1]);
        if (html_content) {
            char *plain_text = extract_text_from_html(html_content);
            if (plain_text) {
                printf("\n--- Page Content ---\n%s\n--- End of Content ---\n\n", plain_text);
                free(plain_text);
            } else {
                fprintf(stderr, "Could not parse HTML from %s.\n", argv[1]);
            }
            free(html_content);
        } else {
            fprintf(stderr, "Could not fetch URL: %s.\n", argv[1]);
        }
        // If started with a command line argument, exit after processing.
        // For interactive mode, the user should run without arguments.
        return 0;
    }

    // Interactive mode
    while (1) {
        printf("Enter a URL (or 'exit'/'quit' to stop): ");
        if (fgets(url_buffer, sizeof(url_buffer), stdin) == NULL) {
            // Handle EOF or read error
            printf("\nExiting due to input error or EOF.\n");
            break;
        }

        // Remove trailing newline character from fgets
        url_buffer[strcspn(url_buffer, "\n")] = 0;

        if (strcmp(url_buffer, "exit") == 0 || strcmp(url_buffer, "quit") == 0) {
            printf("Exiting browser.\n");
            break;
        }

        if (strlen(url_buffer) == 0) {
            printf("Please enter a valid URL.\n");
            continue;
        }

        // Basic check for http or https prefix, not exhaustive
        if (strncmp(url_buffer, "http://", 7) != 0 && strncmp(url_buffer, "https://", 8) != 0) {
            printf("Warning: URL does not start with http:// or https://. Proceeding anyway.\n");
            // Optionally, prepend http:// if missing, or reject. For now, proceed.
        }


        printf("Fetching URL: %s\n", url_buffer);
        char *html_content = fetch_url(url_buffer);

        if (html_content) {
            // printf("Fetched RAW HTML (length: %zu):\n%s\n", strlen(html_content), html_content); // For debugging
            char *plain_text = extract_text_from_html(html_content);
            if (plain_text) {
                printf("\n--- Page Content ---\n%s\n--- End of Content ---\n\n", plain_text);
                free(plain_text);
            } else {
                fprintf(stderr, "Could not parse HTML from %s.\n", url_buffer);
            }
            free(html_content);
        } else {
            fprintf(stderr, "Could not fetch URL: %s.\n", url_buffer);
        }
    }

    return 0;
}
