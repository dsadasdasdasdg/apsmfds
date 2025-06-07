# Text-Based Web Browser in C

This project is a simple text-based web browser written in C.
The goal is to fetch and display the textual content of web pages, navigate links, and maintain a browsing history.

## Features
- Fetch HTML content from a given URL using HTTP GET requests (via libcurl).
- Parse basic HTML to extract and display visible text.
    - Handles common tags: `<p>`, `<h1>`-`<h6>`, `<br>`.
    - Simplified list display: `<ul>`, `<ol>`, `<li>` items are extracted with newlines for separation.
- Extract hyperlinks (`<a>` tags with `href` attributes) from HTML.
- Command-line interface for user input:
    - Enter a full URL to navigate to a new page.
    - Select extracted links by number to navigate.
    - Use the "back" command to go to the previously visited page.
    - "exit" or "quit" to terminate the browser.
- Basic URL resolution for absolute, root-relative (e.g., `/foo.html`), and same-directory/subdirectory relative links (e.g., `bar.html`, `baz/page.html`).
- History stack for the "back" command.
- Basic build system using a Makefile.

## Prerequisites
Before building, ensure you have the following installed:
- `gcc` (GNU Compiler Collection)
- `make` (GNU Make utility)
- `libcurl` development libraries (e.g., `libcurl4-openssl-dev` on Debian/Ubuntu, `libcurl-devel` on Fedora/CentOS)

You can typically install these on a Debian-based system (like Ubuntu) using:
```bash
sudo apt-get update
sudo apt-get install -y gcc make libcurl4-openssl-dev
```

## Build Instructions
1.  Navigate to the `text_browser` directory in your terminal.
2.  Run the `make` command:
    ```bash
    make
    ```
    This will compile the source files and create an executable named `text_browser` in the current directory.

## Usage
Once built, you can run the browser from the `text_browser` directory:
```bash
./text_browser [starting_url]
```
- If `[starting_url]` is provided, the browser will attempt to load it.
- If no URL is provided, it defaults to `http://example.com`.

The browser will then display the page content and a list of extracted links.
At the prompt, you can:
- **Enter a full URL:** e.g., `http://info.cern.ch`
- **Select a link by number:** If links are displayed as `[1] Link Text ...`, type `1` and press Enter.
- **Go back:** Type `back` and press Enter to navigate to the previously visited page in your history.
- **Exit:** Type `exit` or `quit` and press Enter.

## Known Issues
-   **HTML Parsing:** The current HTML parser is very basic. It might not correctly render complex pages. Content immediately following list structures (`</ul>`, `</ol>`) might sometimes be missed or displayed incorrectly in the main text output.
-   **URL Resolution:** Relative URL resolution for `../` patterns is very basic and may not always produce the correct URL.
-   **Character Encoding:** The browser does not currently handle different character encodings; it primarily expects UTF-8 or ASCII.
-   **No CSS/JavaScript:** This is a text-only browser; CSS styling and JavaScript execution are not supported.

## Future Goals
- Advanced HTML parsing and rendering.
- More robust relative URL resolution.
- HTTPS support (already available via libcurl, just ensure it's used).
- Search functionality within fetched content.
- Bookmarks.
- More sophisticated history management (e.g., "forward" command).
- Configuration options.
