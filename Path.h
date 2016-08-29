#pragma once

#include <WString.h>

namespace Path {
	String combinePaths(String path1, String path2);

	String normalisePath(String path);

	bool hasFile(String path);

	String getFileName(String path);

	String getFileExtension(String path);
}