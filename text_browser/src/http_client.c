#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "http_client.h"

// Struct to hold the response data
struct MemoryStruct {
  char *memory;
  size_t size;
};

// Callback function to write data received by libcurl
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
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

  chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
  chunk.size = 0;            /* no data at this point */

  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle = curl_easy_init();

  if(curl_handle) {
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0"); // Optional: set a user agent

    // Follow redirects
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);


    res = curl_easy_perform(curl_handle);

    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      free(chunk.memory);
      curl_easy_cleanup(curl_handle);
      curl_global_cleanup();
      return NULL;
    }

    /* always cleanup */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    return chunk.memory; // Caller must free this
  }

  free(chunk.memory);
  curl_global_cleanup();
  return NULL; // Should not happen if curl_easy_init worked
}
