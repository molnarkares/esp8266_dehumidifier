# Pages

This folder contains the work files of the web server pages. These files can be changed then compressed in order to save memory and improve response time of ESP8266 WebServer.

## Architecture

The page connsists of a single main HTML file (**index.html**) that refers to
* list.json : the json response of the web server including the sensor readings and the status of the switches
* main.css is the CSS file
* main.js contains the javascript control file that handles the periodic reading of list.json and controls the switches
* control.php - this was used only during testing. You shall replace *control.php* to *control.cgi* in main.js file when you use it in esp8266.

## Moving pages to esp8266

### General

1. Convert the file using a compressor tool. Either use the recommended ones below or another that you like better.
2. Paste the compressed file in a text editor and replace the quotation mark " to \"
3. It is recommended to break down the compressed content to multiple lines for better readability. 
4. Add '    message += F("   ' at the beginning of each line and '   ");   ' at the end of the  lines.
5. Copy-paste the formatted message to the Arduino sketch.

### index.html
You can use http://www.miniwebtool.com/html-compressor/

### main.css
You may use http://cssminifier.com/

### main.js 
You can use http://jscompress.com/
