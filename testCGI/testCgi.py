#!/usr/bin/env python3

import os
import sys

# Print the necessary HTTP header
print("Content-Type: text/html\n")

# Print basic HTML output
print("<html><body>")
print("<h1>CGI Test</h1>")

# Handle GET request
query_string = os.environ.get("QUERY_STRING", "")
if query_string:
    print(f"<p><b>GET Data:</b> {query_string}</p>")

# Handle POST request
content_length = os.environ.get("CONTENT_LENGTH", "0")
if content_length.isdigit():
    post_data = sys.stdin.read(int(content_length))
    print(f"<p><b>POST Data:</b> {post_data}</p>")

print("</body></html>")

