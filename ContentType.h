#pragma once
#include <WString.h>

namespace ContentType {
	const unsigned char numberOfTypes = 10;
	const String extensions[numberOfTypes] = {
		"txt",
		"html",
		"css",
		"js",
		"ico",
		"bmp",
		"jpg"
		"jpeg",
		"png"
		"gif"
	};

	const String types[numberOfTypes] = {
		"text/plain",
		"text/html",
		"text/css",
		"text/javascript",
		"image/x-icon",
		"image/bmp",
		"image/jpeg",
		"image/jpeg",
		"image/png",
		"image/gif"
	};

	String getTypeFromExtension(String extension);
}
