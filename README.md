# Arduino Webserver
This project was to create a webserver with all the common features we are used to but on an Arduino platform.
#### Why Arduino?
Installing webservers today can be a complicated process, whether you use a simulator such as XAMPP or professional applications such as Microsoft IIS - you must know the basics of webservers and networking, what technologies you want to use, where to find them and how to install them etc.<br/>
Writing a webserver from scratch provides insight into how they work and creates a platform to learn and tinker with.
+ **_Easy to setup_** - 7 steps that any Arduino user should be familiar with
+ **_Educational_** - See the steps a modern webserver takes or learn basic web development if you'd rather read code than documentation.
+ **_Configurable_** - Its open-source!

## Installation
1. Format SD card to FAT-32 or FAT-16 (FAT-16 recommended for performance)
2. Copy website to root of SD card
3. Copy all files under the SD folder to root of SD card
4. Insert SD card into Ethernet shield
5. Attach Arduino Ethernet Shield
6. Attach Ethernet cable from shield into router
7. Upload the code

It's now running, check the serial port at baud 250000 for local IP to browse to.<br/>
If running from a private network you may need to port-forward the local IP on port 80 via your router.<br/>

## Requirements
1. Arduino Due or similar
2. SD card
3. A PC (for setup or debugging)
4. Arduino Ethernet Shield
5. Ethernet cable (duh)

## Supported Features
+ MIME types
+ Directory listing
+ 404 (File not found) page
+ 415 (unsupported media type) page
+ Logging via serial port
+ Correct HTTP Status codes

## Limitations
+ 58 KB/s upload speed
+ [8:3 file name format](https://en.wikipedia.org/wiki/8.3_filename)

## Future Features
+ Config.h for easier configuration
+ Enable/Disable directory browsing
+ More error pages

Duration: 1 month
