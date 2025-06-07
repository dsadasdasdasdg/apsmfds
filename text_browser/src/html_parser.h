#ifndef HTML_PARSER_H
#define HTML_PARSER_H

// Structure for a single link
typedef struct {
    char *url;        // URL from href attribute
    char *text;       // Visible text of the link
} LinkInfo;

// Structure for a list of links
typedef struct {
    LinkInfo *links;
    int count;
    int capacity;
} LinkList;

// Function prototype for extracting text and links from HTML content
// Returns the main page text (caller must free)
// Populates the link_list (caller must initialize and later free using free_link_list)
char *extract_text_and_links_from_html(const char *html_content, LinkList *link_list);

// Helper functions for LinkList management
void init_link_list(LinkList *list, int initial_capacity);
void add_link(LinkList *list, const char *url, const char *text);
void free_link_list(LinkList *list);

#endif // HTML_PARSER_H
