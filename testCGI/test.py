#!/usr/bin/env python3

import os
import sys
import urllib.parse

print("<html><body>")
print("<h1>CGI Work Successfully!</h1>")

content_length = os.environ.get("CONTENT_LENGTH", "0")
if content_length.isdigit():
    content_length = int(content_length)
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        post_params = urllib.parse.parse_qs(post_data)

        print("<h2>Received POST Data:</h2><ul>")
        for key, value in post_params.items():
            print(f"<li><b>{key.capitalize()}:</b> {value[0]}</li>")
        print("</ul>")
    else:
        print("<p>No POST data received.</p>")
else:
    print("<p>Error: Invalid CONTENT_LENGTH.</p>")

# Fine HTML
print("</body></html>")

