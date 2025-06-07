#define _GNU_SOURCE // For strcasestr if needed, and other POSIX extensions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // For strdup, strlen, strcpy, strcat, strrchr, etc.
#include <strings.h> // For strncasecmp, strcasecmp
#include <curl/curl.h>
#include "http_client.h" // MAX_URL_LENGTH should be defined in here

// Struct to hold the response data (for fetch_url)
struct MemoryStruct {
  char *memory;
  size_t size;
};

// Callback function to write data received by libcurl (for fetch_url)
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    fprintf(stderr, "HTTP_CLIENT_ERROR: not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

char *fetch_url(const char *url) {
  CURL *curl_handle;
  CURLcode res;
  struct MemoryStruct chunk;

  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle = curl_easy_init();

  if(curl_handle) {
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "text_browser/0.1");
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 20L);

    res = curl_easy_perform(curl_handle);

    if(res != CURLE_OK) {
      fprintf(stderr, "HTTP_CLIENT_ERROR: curl_easy_perform() failed for %s: %s\n", url, curl_easy_strerror(res));
      free(chunk.memory);
      curl_easy_cleanup(curl_handle);
      curl_global_cleanup();
      return NULL;
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return chunk.memory;
  }

  free(chunk.memory);
  curl_global_cleanup();
  return NULL;
}

// URL Resolution Logic
char *resolve_url(const char *base_url, const char *relative_or_absolute_url) {
    if (!base_url || !relative_or_absolute_url) {
        return NULL;
    }
    if (strlen(base_url) == 0) { // Cannot resolve against an empty base
        char* rel_url_copy = strdup(relative_or_absolute_url);
        if (!rel_url_copy) {fprintf(stderr, "HTTP_CLIENT_ERROR: strdup failed in resolve_url\n");}
        return rel_url_copy;
    }


    // 1. Check if relative_or_absolute_url is already absolute
    if (strncmp(relative_or_absolute_url, "http://", 7) == 0 || strncmp(relative_or_absolute_url, "https://", 8) == 0) {
        char* abs_url = strdup(relative_or_absolute_url);
        if (!abs_url) {fprintf(stderr, "HTTP_CLIENT_ERROR: strdup failed in resolve_url for absolute\n");}
        return abs_url;
    }

    char scheme[10] = "";
    char authority[1024] = "";
    char base_dir_path[MAX_URL_LENGTH] = ""; // To store "http://example.com/path/to/" (path ends with /)

    const char *scheme_end = strstr(base_url, "://");
    if (!scheme_end) {
        fprintf(stderr, "HTTP_CLIENT_ERROR: Invalid base_url (no ://): %s\n", base_url);
        // Attempt to treat relative_or_absolute_url as is, if it looks like a full URL (less likely now)
        // or just return it as a relative path to be handled by caller or fail.
        // For now, if base is invalid, we can't do much.
        return strdup(relative_or_absolute_url);
    }
    strncpy(scheme, base_url, scheme_end - base_url);
    scheme[scheme_end - base_url] = '\0'; // scheme is "http" or "https"

    const char *auth_start = scheme_end + 3; // Skip "://"
    const char *path_start_after_auth = strchr(auth_start, '/');

    if (path_start_after_auth) { // If there's a path component like /path/to/file.html
        strncpy(authority, auth_start, path_start_after_auth - auth_start);
        authority[path_start_after_auth - auth_start] = '\0'; // authority is "example.com"

        const char *last_slash = strrchr(base_url, '/');
        // Ensure last_slash is after the "://" part, otherwise it's like http://example.com (no path)
        if (last_slash > scheme_end + 2) {
             strncpy(base_dir_path, base_url, last_slash - base_url + 1);
             base_dir_path[last_slash - base_url + 1] = '\0'; // path is "http://example.com/path/to/"
        } else {
            snprintf(base_dir_path, MAX_URL_LENGTH, "%s://%s/", scheme, authority); // path is "http://example.com/"
        }
    } else { // No path, e.g., "http://example.com"
        strcpy(authority, auth_start);
        snprintf(base_dir_path, MAX_URL_LENGTH, "%s://%s/", scheme, authority); // path is "http://example.com/"
    }

    char *resolved_url_buf = malloc(MAX_URL_LENGTH);
    if (!resolved_url_buf) {
        fprintf(stderr, "HTTP_CLIENT_ERROR: malloc failed for resolved_url_buf in resolve_url\n");
        return NULL;
    }

    // 2. Handle root-relative URLs (e.g., "/path/to/page.html")
    if (relative_or_absolute_url[0] == '/') {
        snprintf(resolved_url_buf, MAX_URL_LENGTH, "%s://%s%s", scheme, authority, relative_or_absolute_url);
        return resolved_url_buf;
    }

    // 3. Handle other relative URLs (e.g., "page.html", "subdir/page.html")
    if (strstr(relative_or_absolute_url, "../") != NULL) {
         // For this simplified version, we don't correctly handle "../"
         // We'll just append to base_dir_path, which will be wrong for ../
         fprintf(stderr, "Warning: Navigation using '../' in relative URLs might be incorrect: %s\n", relative_or_absolute_url);
    }
    // This handles "page.html" and "subdir/page.html" correctly by appending to base_dir_path
    snprintf(resolved_url_buf, MAX_URL_LENGTH, "%s%s", base_dir_path, relative_or_absolute_url);
    return resolved_url_buf;
}
