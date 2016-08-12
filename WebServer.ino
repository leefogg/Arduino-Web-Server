#define EthernetEnablePin 10
#define SDEnablePin 4
 

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

#include "HTTPMethod.h"
#include "HTTPStatusCode.h"

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 16);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

void enableEthernet() {
	digitalWrite(SDEnablePin, HIGH);
	digitalWrite(EthernetEnablePin, LOW);
}

void enableSD() {
	digitalWrite(SDEnablePin, LOW);
	digitalWrite(EthernetEnablePin, HIGH);
}

struct HTTPRequest {
	byte HTTPMethod;
	String File;
	boolean KeepAlive;
	//TODO: Accepted types
} Request;
struct HTTPResponse {
	byte StatusCode;
	boolean KeepAlive;
	int ContentLength;
	String ContentType;
} Response;



void setup() {
	// Open serial communications and wait for port to open:
	Serial.begin(250000);
	while (!Serial) {
		; // wait for serial port to connect. Needed for native USB port only
	}

	enableSD();
	if (SD.begin(SDEnablePin)) {
		Serial.println("Found SD card");
	} else {
		Serial.println("Card failed, or not present");
		return; // Cant continue if cant serve files
		//TODO: Return appropriate response
	}


	enableEthernet();
	// start the Ethernet connection and the server:
	Ethernet.begin(mac, ip);
	server.begin();
	server.available();
	Serial.print("server is at ");
	Serial.println(Ethernet.localIP());
}

void readHTTPRequest(EthernetClient client) {
	Request.File = "";
	Request.HTTPMethod = HTTPMethod::Get; // Assume get
	Request.KeepAlive = false; // Close connection after by default

	String line = readLine(client);
	Serial.println(line);

	if (line.indexOf("CONNECT"))
		Request.HTTPMethod = HTTPMethod::Connect;
	if (line.indexOf("DELETE"))
		Request.HTTPMethod = HTTPMethod::Delete;
	if (line.indexOf("GET"))
		Request.HTTPMethod = HTTPMethod::Get;
	if (line.indexOf("HEAD"))
		Request.HTTPMethod = HTTPMethod::Head;
	if (line.indexOf("OPTIONS"))
		Request.HTTPMethod = HTTPMethod::Options;
	if (line.indexOf("POST"))
		Request.HTTPMethod = HTTPMethod::Post;
	if (line.indexOf("PUT"))
		Request.HTTPMethod = HTTPMethod::Put;
	if (line.indexOf("TRACE"))
		Request.HTTPMethod = HTTPMethod::Trace;
	Request.File = line.substring(line.indexOf(' '), line.lastIndexOf(' '));

	while ((line = readLine(client)).length() != 1) {
		Serial.println(line);

		if (line.indexOf("Connection: "))
			Request.KeepAlive = line.substring(line.indexOf(' ')) == "keep-alive";
	}
}

String readLine(EthernetClient client) {
	String out;
	char in;
	while (client.available())
		if ((in = client.read()) != '\n')
			out += in;
		else
			break;

	return out;
}

void loop() {
	Ethernet.maintain();

	// listen for incoming clients
	EthernetClient client = server.available();
	if (client) {
		while (client.connected()) {
			if (client.available()) {
				readHTTPRequest(client);

				// send a standard http response header
				client.println("HTTP/1.1 200 OK");
				client.println("Content-Type: text/html");
				client.println("Connection: Closed");// the connection will be closed after completion of the response
				//client.println("Refresh: 5");// refresh the page automatically every 5 sec
				client.println();
		
				enableSD();
				File dataFile = SD.open("page.txt");
				// if the file is available, write to it:
				if (dataFile) {
					while (dataFile.available()) {
						// Buffer the data
						int const buffersize = 100; //TODO fill RAM
						char buffer[buffersize];
						unsigned int i = 0;
						while (i < buffersize && dataFile.available()) {
							buffer[i++] = dataFile.read();
						}

						// Send buffered data
						enableEthernet();
						unsigned int size = i;
						for (i = 0; i < size; i++)
							client.write(buffer[i]);

						enableSD();
					}
					dataFile.close();
				} else {
					Serial.println("File doesn't exist");
				}

				enableEthernet();

				break;
			}
		}
		// give the web browser time to receive the data
		delay(1);
		// close the connection:
		client.stop();
		Serial.println("client disconnected");
	}
}
