#!/usr/bin/env python3
import sys
import os

# Get the content length from the environment
content_length = int(os.environ.get("CONTENT_LENGTH", 0))

# Read POST data from stdin (only if content_length > 0)
post_data = sys.stdin.read(content_length) if content_length > 0 else "(No POST data)"

# Debug: Log POST data
with open("/tmp/cgi_debug.log", "w") as debug_log:
    debug_log.write(f"Received POST data ({content_length} bytes): {post_data}\n")

# Print HTTP Response
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>CGI DEBUG</h1>")
print("<h2>POST Data:</h2><pre>")
print(post_data)
print("</pre>")
print("</body></html>")

