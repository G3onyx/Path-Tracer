#pragma once

#include <GL/gl3w.h>
#include "stb_image.h"
#include <iostream>

class Texture {
public:
	Texture(std::string path, bool isData = false);
	~Texture();

	void bind(unsigned int unit) const;
	void unbind() const;

private:
	unsigned int id;
};