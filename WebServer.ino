#define EthernetEnablePin 10
#define SDEnablePin 4

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

#include "HTTPMethod.h"
#include "HTTPStatusCode.h"
#include "ContentType.h"

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
	unsigned int StatusCode;
	boolean KeepAlive;
	size_t ContentLength;
	String ContentType;
} Response;



void setup() {
	// Open serial communications and wait for port to open:
	Serial.begin(250000);

	enableSD();
	if (!SD.begin(SDEnablePin)) {
		Serial.println("SD failed.");
		return; // Cant continue if cant serve files
		//TODO: Return appropriate response
	}


	enableEthernet();
	// start the Ethernet connection and the server:
	Ethernet.begin(mac, ip);
	server.begin();
	server.available();
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

void clearRequest() {
	Request.File = "";
	Request.Method = HTTPMethod::Get; // Assume get
	Request.KeepAlive = false; // Close connection after by default
}

void clearResponse() {
	Response.ContentLength = 0;
	Response.ContentType = "";
	Response.KeepAlive = false;
	Response.StatusCode = HTTPStatusCode::ClientError::BadRequest;
}

void readHTTPRequest(EthernetClient client) {
	clearRequest();

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
	enableEthernet();

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

void dumpFile(String filepath, EthernetClient client) {
	enableSD();

	File file = SD.open(filepath);
	// if the file is available, write to it:
	if (!file) {
		Serial.println("File doesn't exist.");
		return;
	}

	Serial.println("Reading file..");
	// Buffer the data
	uint16_t const buffersize = 1024*2;
	char buffer[buffersize];
	while (file.available()) {
		Serial.print("Reading ");
		Serial.println(buffersize, DEC);

		int bytesread = file.read(buffer, buffersize);

		// Send buffered data
		enableEthernet();
		client.write(buffer, bytesread);

		enableSD();
	}
	file.close();
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

				clearResponse();

				Response.KeepAlive = Request.KeepAlive;

				if (Request.Method == HTTPMethod::Get) {
					bool sendcontent = false;
					//TODO: Support directory browsing
					if (Request.File.indexOf('.') == -1) {// No file specified and directory browsing not supported
						Response.StatusCode = HTTPStatusCode::ClientError::Unauthorized;
						Response.KeepAlive = false;
					} else {
						enableSD();
							
						File file;
						if (SD.exists(Request.File) && (file = SD.open(Request.File))) {
							Response.ContentLength = file.size();
							Response.StatusCode = HTTPStatusCode::Success::OK;

							String extension = Request.File.substring(Request.File.lastIndexOf('.') + 1);
							extension.toLowerCase();
							Response.ContentType = ContentType::getTypeFromExtension(extension);
							sendcontent = true;
							file.close();
						} else {
							Response.StatusCode = HTTPStatusCode::ClientError::NotFound;
							Response.KeepAlive = false;
						}
					}
					
					writeHTTPResponse(client);

					if (sendcontent) {
						dumpFile(Request.File, client);
					}
				} else {
					Response.StatusCode = HTTPStatusCode::ClientError::MethodNotAllowed;

					writeHTTPResponse(client);
				}

				enableEthernet();
				break;
			}
		}

		Serial.println("Flushing data stream...");
		client.flush();

		// Close the connection
		client.stop();
		Serial.println("Client disconnected.");
	}
}
