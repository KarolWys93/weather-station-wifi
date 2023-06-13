# HTML static files

The html directory contains all the necessary static files for the device's web server. The api_mock directory contains mock APIs for testing the server on a local machine. These mocks are examples of server responses to various requests.
The device stores all website static files in the *'html"* directory on the SD card.


## Static files compression 
The http server always returns static files compressed with gzip (Content-Encoding set as gzip). The web server does not have a compression function implemented, so static files must be compressed beforehand.
To prepare the files for the server, run the ***compress.sh*** script.
The script creates a new directory *"compressed"*, copies the *"html"* directory there and compresses all files to the desired format. The *"html"* directory from the *"compressed"* directory can be copied to the root directory of the SD card.

## Test server
The test server can be used to test a website locally without the weather station device.
To enable test server, run the ***server_test.bat*** script. The script copies the static and mock files to the "test_server" directory and then starts a simple Python HTTP server. The server is listening for a connection on port **8000**.
The server works on a copy of the files, so any changes will be visible after restarting the server.
