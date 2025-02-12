#!/usr/bin/env python3

import cgi
import os

# Print HTTP headers
print("Content-Type: text/plain\n")

try:
    # Parse the form data
    form = cgi.FieldStorage()
    file_item = form['file']  # 'file' is the name attribute in the HTML form

    if file_item.filename:
        # Create the uploads directory if it doesn't exist
        upload_dir = "uploads"
        os.makedirs(upload_dir, exist_ok=True)

        # Save the uploaded file
        file_path = os.path.join(upload_dir, os.path.basename(file_item.filename))
        with open(file_path, 'wb') as f:
            f.write(file_item.file.read())
        
        # Response for success
        print(f"File '{file_item.filename}' uploaded successfully.")
    else:
        print("No file was uploaded.")
except Exception as e:
    print(f"Error during upload: {str(e)}")
import logging

logging.basicConfig(filename='/tmp/upload_debug.log', level=logging.DEBUG)

logging.debug("CGI script started.")
try:
    form = cgi.FieldStorage()
    file_item = form['file']
    logging.debug(f"File received: {file_item.filename}")

    if file_item.filename:
        upload_dir = "uploads"
        os.makedirs(upload_dir, exist_ok=True)
        file_path = os.path.join(upload_dir, os.path.basename(file_item.filename))
        logging.debug(f"Saving file to {file_path}")
        
        with open(file_path, 'wb') as f:
            f.write(file_item.file.read())
        logging.debug("File saved successfully.")
        print(f"File '{file_item.filename}' uploaded successfully.")
    else:
        logging.debug("No file uploaded.")
        print("No file was uploaded.")
except Exception as e:
    logging.error(f"Error: {str(e)}")
    print(f"Error during upload: {str(e)}")

