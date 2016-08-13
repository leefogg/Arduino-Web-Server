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
	byte Method;
	String File;
	boolean KeepAlive;
	//TODO: Accepted types
} Request;
struct HTTPResponse {
	byte StatusCode;
	boolean KeepAlive;
	uint32_t ContentLength;
	String ContentType;
	boolean noContent;
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

void readHTTPRequest(EthernetClient client) {
	Request.File = "";
	Request.Method = HTTPMethod::Get; // Assume get
	Request.KeepAlive = false; // Close connection after by default

	String line = readLine(client);
	Serial.println(line);

	if (line.indexOf("GET") != -1)
		Request.Method = HTTPMethod::Get;
	else if (line.indexOf("HEAD") != -1)
		Request.Method = HTTPMethod::Head;
	else if (line.indexOf("POST") != -1)
		Request.Method = HTTPMethod::Post;
	else if (line.indexOf("PUT") != -1)
		Request.Method = HTTPMethod::Put;
	else if (line.indexOf("DELETE") != -1)
		Request.Method = HTTPMethod::Delete;
	else if (line.indexOf("CONNECT") != -1)
		Request.Method = HTTPMethod::Connect;
	else if (line.indexOf("OPTIONS") != -1)
		Request.Method = HTTPMethod::Options;
	else if (line.indexOf("TRACE") != -1)
		Request.Method = HTTPMethod::Trace;

	Request.File = line.substring(line.indexOf(' ')+1, line.lastIndexOf(' '));

	while ((line = readLine(client)).length() != 1) {
		Serial.println(line);

		if (line.indexOf("Connection") != -1)
			Request.KeepAlive = line.indexOf("keep-alive") != -1;
	}
}

void writeHTTPResponse(EthernetClient client) {
	Serial.print("HTTP/1.0 ");
	Serial.println(String(Response.StatusCode));
	Serial.print("Content-Type: ");
	Serial.println(Response.ContentType);
	Serial.print("Connection: ");
	Serial.println(Response.KeepAlive ? "keep-alive" : "close");
	Serial.print("Content-Length: ");
	Serial.println(String(Response.ContentLength));
	Serial.println();
	
	client.print("HTTP/1.0 ");
	client.println(String(Response.StatusCode));
	client.print("Content-Type: ");
	client.println(Response.ContentType);
	client.print("Connection: ");
	client.println(Response.KeepAlive ? "keep-alive" : "close");
	client.print("Content-Length: ");
	client.println(String(Response.ContentLength));
	client.println();
}

void loop() {
	Ethernet.maintain();

	// listen for incoming clients
	EthernetClient client = server.available();
	if (client) {
		while (client.connected()) {
			if (client.available()) {
				readHTTPRequest(client);

				Serial.println();
				Serial.print("Connection: ");
				Serial.println(String(Request.KeepAlive));
				Serial.print("Method: ");
				Serial.println(String(Request.Method));
				Serial.print("File: ");
				Serial.println(Request.File);
				Serial.println();

				Response.KeepAlive = Request.KeepAlive;
				//TODO:
				Response.ContentType = "text/html";

				if (Request.Method == HTTPMethod::Get) {
					//TODO: Support directory browsing
					if (Request.File.indexOf('.') == -1) {// No file specified and directory browsing not supported
						Response.StatusCode = HTTPStatusCode::ClientError::Unauthorized;
						Response.noContent = true;
					} else {
						enableSD();
						if (!SD.exists(Request.File)) {
							Response.StatusCode = HTTPStatusCode::ClientError::NotFound;
							Response.noContent = true;
						} else {
							Response.noContent = false;
							Response.StatusCode = HTTPStatusCode::Success::OK;

							File file = SD.open(Request.File);
							if (file)
								Response.ContentLength = file.size();

							file.close();
						}
					}

					enableEthernet();
					writeHTTPResponse(client);

					enableSD();
					if (!Response.noContent) {
						File file = SD.open(Request.File);
						// if the file is available, write to it:
						if (file) {
							while (file.available()) {
								// Buffer the data
								int const buffersize = 100; //TODO fill RAM
								char buffer[buffersize];
								unsigned int i = 0;
								while (i < buffersize && file.available()) {
									buffer[i++] = file.read();
								}

								// Send buffered data
								enableEthernet();
								unsigned int size = i;
								for (i = 0; i < size; i++)
									client.write(buffer[i]);

								enableSD();
							}
							file.close();
						} else {
							Serial.println("File doesn't exist.");
						}
					}
				} else {
					Response.noContent = true;
					Response.StatusCode = HTTPStatusCode::ClientError::MethodNotAllowed;
					Response.ContentLength = 0;

					writeHTTPResponse(client);
				}

				enableEthernet();

				break;
			}
		}

		client.flush();
		// give the web browser time to receive the data
		delay(1);
		// close the connection:
		client.stop();
		Serial.println("Client disconnected.");
	}
}
