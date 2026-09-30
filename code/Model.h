#pragma once

#include <string>
#include <vector>
#include <functional>
#include <iostream>

#include <tiny_obj_loader.h>
#include "AABB.h"
#include "Config.h"

class Model {
public:
	std::vector<Triangle> triangles{};
	AABB bounds{};

	bool isSphere = false;

	Model(std::string objPath);

	void BuildUnitCube();
	void BuildCornellBox();
	void BuildSierpinski();
	void LoadObj(std::string objPath);

private:
	unsigned int vao;
	int indexCount;

	void UpdateGPU();
};