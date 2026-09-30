#pragma once

#include <GL/gl3w.h>
#include <glm/glm.hpp>

#include <vector>
#include <list>
#include <unordered_map>

#include "BVH.h"
#include "Model.h"
#include "ModelInstance.h"
#include "Config.h"

struct ModelData {
	int blasRoot;
	int meshOffset;
};


class Scene {
public:

	std::unordered_map<Model*, ModelData> cookedModels{};

	std::list<ModelInstance> instances{};

	
	// GPU SSBOs and data
	GLuint nodeSSBO[2];
	int currentBuffer = 0;

	GLuint triSSBO, instSSBO, matSSBO;
	std::vector<GPUNode> blasNodes{};
	std::vector<GPUNode> tlasNodes{}; // dynamic
	std::vector<GPUTriangle> globalTris{};
	std::vector<GPUInstance> gpuInstances{}; // dynamic
	std::vector<GPUMaterial> materials;


	int tlasRootIndex = 0; // end of blas nodes
	int lightBallIndex;

	Scene();
	~Scene();

	void UploadToGPU();
	bool UpdateDynamicData();

	ModelInstance* AddInstance(Model* model);
	ModelInstance* AddInstance(Model* model, int matID);
	ModelInstance* AddSphereInstance(Model* model, int matID);



private:
	void DefineDefaultMaterials();
	void BuildTLAS();
};