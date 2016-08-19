#include "ContentType.h"

String ContentType::getTypeFromExtension(String extension) {
	for (unsigned char i = 0; i < numberOfTypes; i++) {
		if (extensions[i] == extension)
			return types[i];
	}

	return "";
}
