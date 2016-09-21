#include "ContentType.h"

/// <summary>
/// Matches a file extension with its corrisponding media type.
/// </summary>
/// <param name="filepath">Extension of a file to find the media type for</param>
/// <returns>The corrisponding media type of the given extension, or empty string if none found</returns>
String ContentType::getTypeFromExtension(String extension) {
	for (unsigned char i = 0; i < numberOfTypes; i++) {
		if (extensions[i] == extension)
			return types[i];
	}

	return "";
}
