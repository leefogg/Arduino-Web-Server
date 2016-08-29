#pragma once
#include <WString.h>

#include "Path.h"

String Path::combinePaths(String path1, String path2) {
	if (path1.endsWith("/"))
		return path1 + path2;
	else
		return path1 + "/" + path2;
}

String Path::normalisePath(String path) {
	if (Path::hasFile(path))
		return path;

	if (path.endsWith("/"))
		return path;
	else
		return path + "/";
}

bool Path::hasFile(String path) {
	return path.indexOf('.') != -1;
}

String Path::getFileName(String path) {
	Path::normalisePath(path);
	return path.substring(path.lastIndexOf("\\") + 1);
}

String Path::getFileExtension(String path) {
	if (!Path::hasFile(path))
		return "";

	return path.substring(path.lastIndexOf(".") + 1);
}