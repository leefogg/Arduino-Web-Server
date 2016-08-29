#define EthernetEnablePin 10
#define SDEnablePin 4

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

#include "HTTPMethod.h"
#include "HTTPStatusCode.h"
#include "ContentType.h"
#include "Path.h"

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

// Initialize the Ethernet server
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
	Ethernet.begin(mac);
	server.begin();
	server.available();
	Serial.println(Ethernet.localIP());
}

String readLine(EthernetClient client) {
	return client.readStringUntil('\n');
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

void writeResponseHeader(EthernetClient client) {
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

void writeToEthernet(String text, EthernetClient client) {
	enableEthernet();
	for (unsigned int i = 0; i < text.length(); i++)
		client.write(text[i]);
}
void writeToEthernet(byte* data, unsigned int size, EthernetClient client) {
	enableEthernet();

	unsigned int dataremaining = size;
	while (dataremaining > 0) {
		unsigned int datatosend = (dataremaining < 2048) ? dataremaining : 2048;

		client.write(data, datatosend);
		data += datatosend;
		dataremaining -= datatosend;
	}
}

void dumpFile(String filepath, EthernetClient client) {
	enableSD();

	File file = SD.open(filepath);
	if (!file) {
		Serial.println("File doesn't exist!");
		return;
	}
	Serial.print("Sending file ");
	Serial.println(filepath);
	dumpFile(file, client);
	file.close();
}
void dumpFile(File file, EthernetClient client) {
	enableSD();

	Response.ContentLength = file.size();
	writeResponseHeader(client);

	Serial.println("Reading file..");
	// Buffer the data
	uint16_t const buffersize = 1024*2;
	char buffer[buffersize];
	while (file.available()) {
		Serial.print("Reading ");
		Serial.print(buffersize, DEC);
		Serial.print(" bytes...");

		int bytesread = file.read(buffer, buffersize);
		Serial.print(" Read ");
		Serial.print(bytesread, DEC);
		Serial.println(" bytes.");

		// Send buffered data
		enableEthernet();
		client.write(buffer, bytesread);

		enableSD();
	}
}

bool sendFile(String filepath, EthernetClient client) {
	enableSD();

	String extension = Path::getFileExtension(filepath);
	extension.toLowerCase();
	String contenttype = ContentType::getTypeFromExtension(extension);

	if (contenttype.length() == 0) {
		Response.StatusCode = HTTPStatusCode::ClientError::UnsupportedMediaType;
		Response.ContentType = "text/html";
		dumpFile("415.htm", client);
		return false;
	}

	File file = SD.open(filepath);
	if (!file) {
		Response.StatusCode = HTTPStatusCode::ClientError::NotFound;
		Response.ContentType = "text/html";
		dumpFile("404.htm", client);
		return false;
	}

	// Passed checks
	Response.StatusCode = HTTPStatusCode::Success::OK;
	Response.ContentType = contenttype;
	dumpFile(file, client);
	file.close();

	return true;
}

void showDirectoryListing(String path, EthernetClient client) { // TODO: Fix file listing changing bug
	enableSD();

	File folder = SD.open(path);
	if (!folder) {
		Response.StatusCode = HTTPStatusCode::ClientError::NotFound;
		Response.ContentType = "text/html";
		dumpFile("404.htm", client);
		return;
	}
	if (!folder.isDirectory()) {
		//TODO: Send appropriate response code
	}

	Response.StatusCode = HTTPStatusCode::Success::OK;
	Response.ContentType = "text/html";
	String content = "<HTML><HEAD><TITLE>" + Request.File + "</TITLE></HEAD><BODY><h1>Index of "+path+"</H1><TABLE><TR><TH>Name</TD><TH>Type</TD><TH>Size</TD></TR>";

	File file;
	while (file = folder.openNextFile()) {
		content += "<TR>";
		content += "<TD><A href = \"";
		String filename = file.name();
		content += Path::combinePaths(path, filename);
		content += "\">";
		content += filename;
		content += "</A></TD>";
		String fileextension = Path::getFileExtension(filename);
		fileextension.toLowerCase();
		String contenttype = ContentType::getTypeFromExtension(fileextension);
		content += "<TD>";
		if (contenttype.length() != 0) {
			content += contenttype;
		} else {
			content += "unknown";
		}
		content += "</TD>";
		content += "<TD>";
		content += file.size();
		content += "</TD>";
		content += "<TR>";
		file.close();
	}
	folder.close();
	
	content += "</TABLE></BODY></HTML>";
	Response.ContentLength = content.length();
	
	writeResponseHeader(client);
	writeToEthernet(content, client);
}

void loop() {
	Ethernet.maintain();

	// listen for incoming clients
	EthernetClient client = server.available();
	if (!client)
		return;

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
				if (!Path::hasFile(Request.File)) { // Requested a folder or file without extension
					showDirectoryListing(Request.File, client);
				} else {
					sendFile(Request.File, client);
				}
			} else {
				Response.StatusCode = HTTPStatusCode::ClientError::MethodNotAllowed;

				writeResponseHeader(client);
			}

			Serial.println();
			Serial.print("Content-length: ");
			Serial.println(String(Response.ContentLength));
			Serial.print("Content-type: ");
			Serial.println(String(Response.ContentType));
			Serial.print("Keep-Alive: ");
			Serial.println(Response.KeepAlive);
			Serial.print("StatusCode: ");
			Serial.println(Response.StatusCode);
			Serial.println();

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
