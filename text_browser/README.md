# Text-Based Web Browser in C

This project is a simple text-based web browser written in C.
The goal is to fetch and display the textual content of web pages.

## Phase 1 Goals
- Fetch HTML content from a given URL using HTTP GET requests.
- Parse basic HTML to extract and display visible text.
- Provide a command-line interface for user input (entering URLs).
- Basic build system using a Makefile.

## Future Goals
- Advanced HTML parsing and rendering.
- Hyperlink navigation.
- Image rendering using text symbols.
- History and cookie management.

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
./text_browser
```
The browser will then prompt you to enter a URL. Type a full URL (e.g., `http://example.com`) and press Enter.
The fetched and parsed text content of the page will be displayed.
To exit the browser, type `exit` or `quit` at the prompt.

You can also pass a URL as a command-line argument:
```bash
./text_browser http://example.com
```
In this case, it will process the given URL and then exit.

## Cleaning the Build
To remove the compiled executable and object files, run:
```bash
make clean
```
