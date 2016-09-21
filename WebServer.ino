#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

#include "HTTPMethod.h"
#include "HTTPStatusCode.h"
#include "ContentType.h"
#include "Path.h"

// Chip Select pins for SD card and Ethernet driver on Arduino's Ethernet Shield.
#define EthernetEnablePin 10
#define SDEnablePin 4
#define EthernetBufferSize 1024*2

// (Hidden) folder used for settings and other system files
const String systemFolder = "_system/";

// Initialize the Ethernet server
// port 80 is default for HTTP
EthernetServer server(80);

/// <summary>
/// Both SD and Ethernet use SPI. Enable the Ethernet driver and disable the SD card.
/// </summary>
void enableEthernet() {
	digitalWrite(SDEnablePin, HIGH);
	digitalWrite(EthernetEnablePin, LOW);
}
/// <summary>
/// Both SD and Ethernet use SPI. Enable the SD card and disable the Ethernet driver.
/// </summary>
void enableSD() {
	digitalWrite(SDEnablePin, LOW);
	digitalWrite(EthernetEnablePin, HIGH);
}

/// <summary>
/// Struct to store data requested by client
/// </summary>
struct HTTPRequest {
	byte Method;
	String File;
	boolean KeepAlive;
	//TODO: Accepted types
} Request;
/// <summary>
/// Struct to store data to be sent to client
/// </summary>
struct HTTPResponse {
	unsigned int StatusCode;
	boolean KeepAlive;
	size_t ContentLength;
	String ContentType;
} Response;

void DebugRequest() {
	Serial.println();
	Serial.print("Keep Alive: ");
	Serial.println(Request.KeepAlive ? "Yes" : "No");
	Serial.print("Method: ");
	Serial.println(String(Request.Method));
	Serial.print("File: ");
	Serial.println(Request.File);
	Serial.println();
}

void DebugResponse() {
	Serial.print("HTTP/1.0 ");
	Serial.println(String(Response.StatusCode));
	Serial.print("Content-Type: ");
	Serial.println(Response.ContentType);
	Serial.print("Connection: ");
	Serial.println(Response.KeepAlive ? "keep-alive" : "close");
	Serial.print("Content-Length: ");
	Serial.println(String(Response.ContentLength));
	Serial.println();
}

/// <summary>
/// Clears data from the last HTTP request.
/// </summary>
void clearRequest() {
	Request.File = "";
	Request.Method = HTTPMethod::Get; // Assume get
	Request.KeepAlive = false; // Close connection after by default
}

/// <summary>
/// Clears data from the last HTTP response.
/// </summary>
void clearResponse() {
	Response.ContentLength = 0;
	Response.ContentType = "";
	Response.KeepAlive = false;
	Response.StatusCode = HTTPStatusCode::ClientError::BadRequest;
}

void setup() {
	// Open serial communications and wait for port to open:
	Serial.begin(250000);
	while (!Serial) { } // Wait until serial port is ready
	
	Serial.println("Starting up...");

	Serial.print("Loading File System...");
	enableSD();
	if (SD.begin(SDEnablePin)) {
		Serial.println("Success.");
	} else {
		Serial.println("Failed.");
		return; // Cant continue if cant serve files
	}

	Serial.print("Loading Ethernet driver...");
	enableEthernet();
	// Enter a unique MAC address below. This only has to be unique on the network.
	byte macaddress[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
	// Start up the ethernet driver with MAC address and find the first available IP on network.
	if (Ethernet.begin(macaddress)) {
		Serial.println("Success.");

		Serial.print("Starting Network service...");
		server.begin();
		Serial.println("Success.");

		// Display the IP the web server is accessable on via the serial port.
		Serial.print("IP address is: ");
		Serial.println(Ethernet.localIP());
	} else {
		Serial.println("Fail.");
		return;
	}
}

/// <summary>
/// Interprets the incoming HTTP request header and fills the request struct.
/// </summary>
void readHTTPRequest(EthernetClient client) {
	clearRequest();

	String line = client.readStringUntil('\n');

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

	while ((line = client.readStringUntil('\n')).length() != 1) {
		if (line.indexOf("Connection") != -1)
			Request.KeepAlive = line.indexOf("keep-alive") != -1;
	}

	// Show read request information

	DebugRequest();
}

/// <summary>
/// Uses the HTTP response struct to write a response header to the client.
/// </summary>
void writeResponseHeader(EthernetClient client) {
	enableEthernet();
	
	client.print("HTTP/1.0 ");
	client.println(String(Response.StatusCode));
	client.print("Content-Type: ");
	client.println(Response.ContentType);
	client.print("Connection: ");
	client.println(Response.KeepAlive ? "keep-alive" : "close");
	client.print("Content-Length: ");
	client.println(String(Response.ContentLength));
	client.println();

	// Confirm what was written to the client by echoing in serial port
	// TODO: Dont duplicate above, write exact string that was written to client to serial.
	DebugResponse();
}

/// <summary>
/// Writes a string of unknown length to the client.
/// Use array version for performance reasons.
/// Currently benchmarked at 1282 bytes per second
/// </summary>
void writeToEthernet(String text, EthernetClient client) {
	enableEthernet();

	// No nice way to work with unknow array lengths in C. Can only trust the String library here
	// Much slower sending because sending byte at a time instead of passing an array
	for (unsigned int i = 0; i < text.length(); i++)
		client.write(text[i]);
}

/// <summary>
/// Writes an array of bytes to the client.
/// Currently benchmarked at 58KB per second.
/// </summary>
void writeToEthernet(byte* data, unsigned int size, EthernetClient client) {
	enableEthernet();

	unsigned int dataremaining = size;
	while (dataremaining > 0) {
		// Limit sent data to ethernet controllers buffer size
		unsigned int datatosend = (dataremaining < EthernetBufferSize) ? dataremaining : EthernetBufferSize;

		client.write(data, datatosend);

		// Move pointer to point to next data in list
		data += datatosend;
		// Count how much data is remaining to be sent
		dataremaining -= datatosend;

		client.flush();
	}
}

/// <summary>
/// Send a file from the SD card to the client.
/// </summary>
/// <param name="filepath">Absolute file path of file on SD card.</param>
void dumpFile(String filepath, EthernetClient client) {
	//TODO: Return if file was sent
	enableSD();

	Serial.print("Sending file: ");
	Serial.println(filepath);
	File file = SD.open(filepath);
	if (!file) {
		Serial.println("File doesn't exist!");
		return;
	}
	dumpFile(file, client);
	file.close();
}
/// <summary>
/// Sends a file from the SD card to the client.
/// </summary>
/// <param name="file">An opened file from the SD card.</param>
void dumpFile(File file, EthernetClient client) {
	// Finish writing header and send to client
	// TODO: Move to caller
	Response.ContentLength = file.size();
	writeResponseHeader(client);

	Serial.println("Reading file..");
	// Ethernet controller has an internal buffer
	char buffer[EthernetBufferSize];
	enableSD();
	while (file.available()) {
		Serial.print("Reading ");
		Serial.print(EthernetBufferSize, DEC);
		Serial.print(" bytes...");

		// Read buffer into RAM while ethernet controller is sending
		int bytesread = file.read(buffer, EthernetBufferSize);
		Serial.print(" Read ");
		Serial.print(bytesread, DEC);
		Serial.println(" bytes.");

		// Ensure ethernet buffer is empty
		client.flush();

		// Send buffered data to controller
		enableEthernet();
		client.write(buffer, bytesread);

		// Switch back to SD for further reading
		enableSD();
	}
}

/// <summary>
/// Requests a file to be sent to the client.
/// Sends a 404 error if the file does not exist.
/// Sends 415 error if the file has unsupported extension.
/// Constructs and sends appropriate HTTP header then file contents.
/// </summary>
/// <param name="filepath">The absolute file path of a file on the SD card.</param>
/// <returns>True if the content of the requested file was sent to the client, otherwise, false.</returns>
bool sendFile(String filepath, EthernetClient client) {
	enableSD();

	Serial.print("Requested File: ");
	Serial.println(filepath);
	// Show Unsupported Media Type page if unknown file extension
	String extension = Path::getFileExtension(filepath);
	extension.toLowerCase();
	String contenttype = ContentType::getTypeFromExtension(extension);
	if (contenttype.length() == 0) { // No media type found on look-up
		Serial.println("Unknown MIME Type");
		Response.StatusCode = HTTPStatusCode::ClientError::UnsupportedMediaType;
		Response.ContentType = "text/html";
		dumpFile(systemFolder + "415.htm", client);
		return false;
	}

	// Show 404 page if file not found.
	if (!SD.exists(filepath)) {
		Serial.println("File not found.");
		Response.StatusCode = HTTPStatusCode::ClientError::NotFound;
		Response.ContentType = "text/html";
		dumpFile(systemFolder + "404.htm", client);
		return false;
	}

	// Passed checks.
	Response.StatusCode = HTTPStatusCode::Success::OK;
	Response.ContentType = contenttype;

	File file = SD.open(filepath);
	dumpFile(file, client);	// Write HTTP header and file contents
	file.close();

	return true;
}

/// <summary>
/// Constructs and sends HTML code to client with classic directory listing of folder contents
/// </summary>
/// <param name="path">The absolute path of a folder on the SD card.</param>
void showDirectoryListing(String path, EthernetClient client) { // TODO: Fix file listing changing bug
	enableSD();

	File folder = SD.open(path);
	if (!folder) { // Open returns null if file could not be found
		Serial.println("File not found.");
		Response.StatusCode = HTTPStatusCode::ClientError::NotFound;
		Response.ContentType = "text/html";
		dumpFile(systemFolder + "404.htm", client);
		return;
	}
	if (!folder.isDirectory()) {
		//TODO: Check is directory before calling
	}

	// Passed validation checks
	Serial.println("Showing directory listing.");
	Response.StatusCode = HTTPStatusCode::Success::OK;
	Response.ContentType = "text/html";

	// Start constructing HTML code 
	String content = "<HTML><HEAD><TITLE>" + Request.File + "</TITLE></HEAD><BODY><h1>Index of "+path+"</H1><TABLE><TR><TH>Name</TD><TH>Type</TD><TH>Size</TD></TR>";

	File file;
	while (file = folder.openNextFile()) { // Open each file in folder
		content += "<TR>";
		content += "<TD><A href = \"";
		String filename = file.name();
		String filepath = Path::makeAbsolute(Path::combinePaths(path, filename));
		content += filepath;
		content += "\">";
		content += filename;
		content += "</A></TD>";
		String fileextension = Path::getFileExtension(filepath);
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
		file.close(); // File must be closed!
	}
	folder.close(); // Close folder for reading.
	
	content += "</TABLE></BODY></HTML>";
	Response.ContentLength = content.length();
	
	// Output constructed header and page
	writeResponseHeader(client);
	writeToEthernet(content, client);
}

/// <summary>
/// Main loop which maintains Ethernet services and listens for incoming requests.
/// </summary>
void loop() {
	// IP address is on lease, notify IP is still in use.
	Ethernet.maintain();

	// Listen for incoming clients
	EthernetClient client = server.available(); // Get any avaiable clients
	if (!client) // If null, no clients waiting.
		return; // Loop again

	while (client.connected()) {
		if (client.available()) {
			readHTTPRequest(client);
			clearResponse();

			// Tell client connection is still alive if requested to be
			Response.KeepAlive = Request.KeepAlive;
			// If file was requested
			if (Request.Method == HTTPMethod::Get) { // Server only supports Get requests currently
				if (!Path::hasFile(Request.File)) { // Requested a folder or file without extension
					// TODO: Enable directory browsing config setting
					showDirectoryListing(Request.File, client);
				} else {
					sendFile(Request.File, client);
				}
			} else {
				Response.StatusCode = HTTPStatusCode::ClientError::MethodNotAllowed;

				writeResponseHeader(client);
			}

			enableEthernet();
			break;
		}
	}

	// Wait for all sent bytes to be confirmed sent.
	Serial.println("Flushing data stream...");
	client.flush();

	// Close the connection to the client
	client.stop(); // TODO: Investigate
	Serial.println("Client disconnected.");
}
