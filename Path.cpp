#pragma once
#include <WString.h>

#include "Path.h"

/// <summary>
/// Appends path2 to path1, ensuring a valid path format
/// </summary>
String Path::combinePaths(String path1, String path2) {
	path1 = normalisePath(path1);
	path2 = normalisePath(path2);

	return path1 + path2;
}

/// <summary>
/// Appends a / to a folder path to ensure a format.
/// Removed prefixed / and appends / to folders
/// </summary>
/// <reutrns>A new normalised version of the given path</returns>
String Path::normalisePath(String path) {
	String newpath = path; // Make a copy in case string is readonly

	newpath.replace("\\", "/");

	if (newpath.startsWith("/")) {
		newpath = newpath.substring(1);
	}

	if (Path::hasFile(newpath))
		return newpath;

	if (!newpath.endsWith("/"))
		newpath = newpath + "/";

	return newpath;
}

/// <summary>
/// Checks whether path mentions a file
/// </summary>
/// <returns>True if a file exists in the given path</returns>
bool Path::hasFile(String path) {
	unsigned int extensionindex = path.indexOf('.');
	return (extensionindex != -1) && (extensionindex != path.length()-1);
}

/// <summary>
/// Extracts file name from a path
/// </summary>
/// <return>File and extension from given path or empoty string if no file exists</returns>
String Path::getFileName(String path) {
	path = normalisePath(path);
	if (!hasFile(path))
		return "";

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