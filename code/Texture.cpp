#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"

static const std::string RES_PATH = "res\\";


Texture::Texture(std::string path, bool isData) {
	glGenTextures(1, &id);

	int width, height, depth;

	stbi_set_flip_vertically_on_load(false);

	unsigned char* data = stbi_load((RES_PATH + path).c_str(), &width, &height, &depth, 0);

	if (data) {
		GLenum format = GL_RED;
		if (depth == 3)
			format = GL_RGB;
		else if (depth == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, id);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// Blue noise/sky box (always highest res)
		if (isData) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		// Normal textures (mipmaps)
		else {
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}
	else {
		std::cout << "Texture failed to load at path: " << path << std::endl;
	}

	stbi_image_free(data);
}

void Texture::bind(unsigned int unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture() {
	glDeleteTextures(1, &id);
}