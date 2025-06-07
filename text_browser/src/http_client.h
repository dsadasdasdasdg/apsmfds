#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

// Define MAX_URL_LENGTH here, ensuring it's effective.
// This value should be accessible by any .c file that includes this header.
#define MAX_URL_LENGTH 2048

// Function prototype for fetching a URL
// Returns a dynamically allocated string with the content, or NULL on error.
// Caller must free the returned string.
char *fetch_url(const char *url);

// Function prototype for resolving a URL
// Takes a base URL (current page's URL) and a potentially relative URL.
// Returns a dynamically allocated string with the absolute URL, or NULL on error/unresolvable.
// Caller must free the returned string.
char *resolve_url(const char *base_url, const char *relative_or_absolute_url);

#endif // HTTP_CLIENT_H
