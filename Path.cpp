#pragma once
#include <WString.h>

#include "Path.h"

/// <summary>
/// Appends path2 to path1, ensuring a valid path format
/// </summary>
String Path::combinePaths(String path1, String path2) {
	// TODO: replace \ with /
	// TODO: normalise both paths
	if (path1.endsWith("/"))
		return path1 + path2;
	else
		return path1 + "/" + path2;
}

/// <summary>
/// Appends a / to a folder path to ensure a format
/// </summary>
String Path::normalisePath(String path) {
	if (Path::hasFile(path))
		return path;

	if (path.endsWith("/"))
		return path;
	else
		return path + "/";
}

/// <summary>
/// Checks whether path mentions a file
/// </summary>
/// <returns>True if a file exists in the given path</returns>
bool Path::hasFile(String path) {
	// TODO: check if content after the .
	return path.indexOf('.') != -1;
}

/// <summary>
/// Extracts file name from a path
/// </summary>
/// <return>File and extension from given path or path if no file exists</returns>
String Path::getFileName(String path) {
	Path::normalisePath(path);
	// TODO: Check if file exists
	return path.substring(path.lastIndexOf("\\") + 1);
}

/// <summary>
/// Extracts a file's extension
/// </summary>
/// <returns>The file's extension or an empty string if no file is found in string</return>
String Path::getFileExtension(String path) {
	if (!Path::hasFile(path))
		return "";

	return path.substring(path.lastIndexOf(".") + 1);
}