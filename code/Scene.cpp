#include "Scene.h"


Scene::Scene() {
	glGenBuffers(2, nodeSSBO);

	size_t maxNodesSize = Config::SCENE_MAX_NODES * sizeof(GPUNode);

	for (int i = 0; i < 2; i++) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeSSBO[i]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, maxNodesSize, nullptr, GL_DYNAMIC_DRAW);
	}

	glGenBuffers(1, &triSSBO);
	glGenBuffers(1, &instSSBO);
	glGenBuffers(1, &matSSBO);

	DefineDefaultMaterials();
}

Scene::~Scene() {
	glDeleteBuffers(2, nodeSSBO);
	glDeleteBuffers(1, &triSSBO);
	glDeleteBuffers(1, &instSSBO);
	glDeleteBuffers(1, &matSSBO);
}

void Scene::DefineDefaultMaterials() {
	auto createMat = [](glm::vec3 albedo, float rough, float metal, float emissive, float ior, int type, glm::vec3 tint) {
		GPUMaterial m{};
		m.albedo_rough = glm::vec4(albedo, rough);
		m.tint_metal = glm::vec4(tint, metal);
		m.params = glm::vec4(emissive, ior, (float)type, 1.0f); // AO defaults to 1.0
		return m;
	};

	// LIGHT
	materials.push_back(createMat(
		glm::vec3(1.f), // albedo
		1.0f, // roughness
		0.0f, // metallic
		4.0f, // emission
		1.0f, // ior
		0, // type
		glm::vec3(1.f) // tint
	));

	// GLASS
	materials.push_back(createMat(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, 0.0f, 1.50f, 1, glm::vec3(0.92f, 0.98f, 0.94f)));

	// MIRROR
	materials.push_back(createMat(glm::vec3(0.97f),            0.0f, 1.0f, 0.0f, 1.0f, 0, glm::vec3(0.0f)));

	// RED
	materials.push_back(createMat(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.0f, 0.0f, 1.0f, 0, glm::vec3(0.0f)));

	// GREEN
	materials.push_back(createMat(glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 0.0f, 0.0f, 1.0f, 0, glm::vec3(0.0f)));

	// BLUE
	materials.push_back(createMat(glm::vec3(0.0f, 0.0f, 1.0f), 0.04f, 1.0f, 0.0f, 1.0f, 0, glm::vec3(0.0f)));

	// WHITE
	materials.push_back(createMat(glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, 0.0f, 0.0f, 1.0f, 0, glm::vec3(0.0f)));

	// SEAGLASS
	materials.push_back(createMat(glm::vec3(1.0f, 1.0f, 1.0f), 0.4f, 0.01f, 0.0f, 1.52f, 1, glm::vec3(0.55f, 0.85f, 0.75f)));
}


void Scene::UploadToGPU() {

	tlasRootIndex = (int)blasNodes.size();

	// BLAS then TLAS nodes [Dynamic]
	for (int i = 0; i < 2; i++) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeSSBO[i]);

		// Upload BLAS nodes first
		if (!blasNodes.empty()) {
			glBufferSubData(
				GL_SHADER_STORAGE_BUFFER, 
				0, 
				blasNodes.size() * sizeof(GPUNode), 
				blasNodes.data()
			);
		}
		// Upload TLAS nodes second
		if (!tlasNodes.empty()) {
			glBufferSubData(
				GL_SHADER_STORAGE_BUFFER,
				tlasRootIndex * sizeof(GPUNode),
				tlasNodes.size() * sizeof(GPUNode), 
				tlasNodes.data()
			);
		}
	}
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, nodeSSBO[currentBuffer]);

	// Triangles [Static]
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, triSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, globalTris.size() * sizeof(GPUTriangle), globalTris.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triSSBO);

	// Instances [Dynamic]
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, instSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, gpuInstances.size() * sizeof(GPUInstance), gpuInstances.data(), GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, instSSBO);

	// Materials [Static]
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, matSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, materials.size() * sizeof(GPUMaterial), materials.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, matSSBO);
}


void Scene::BuildTLAS() {

	// Build new TLAS
	std::vector<ModelInstance> tempInstances(instances.begin(), instances.end());
	BVH<ModelInstance> newTlas(tempInstances, Config::TLAS_MAX_DEPTH, Config::TLAS_MIN_PRIMS);
	
	tlasNodes = newTlas.flattenNodes(tlasRootIndex);

	// Swap buffers (double buffering)
	currentBuffer = 1 - currentBuffer;

	// Overwrite just TLAS nodes on GPU with new ones
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodeSSBO[currentBuffer]);
	glBufferSubData(
		GL_SHADER_STORAGE_BUFFER,
		tlasRootIndex * sizeof(GPUNode),
		tlasNodes.size() * sizeof(GPUNode),
		tlasNodes.data()
	);

	// Ensure write finishes before GPU reads
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}


bool Scene::UpdateDynamicData() {

	bool tlasNeedsRebuild = false;
	bool instancesUpdated = false;

	for (ModelInstance& instance : instances) {
		if (instance.needsUpdate) {
			if (!instance.ignoresMovement) tlasNeedsRebuild = true;
			instancesUpdated = true;

			// Update GPUInstances in Scene from ModelInstances
			glm::mat4 model = instance.getTransform();
			gpuInstances[instance.gpuInstanceIndex].modelMatrix = model;
			gpuInstances[instance.gpuInstanceIndex].invModelMatrix = glm::inverse(model);

			instance.needsUpdate = false;
		}
	}

	// Update Instances on the GPU
	if (instancesUpdated) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, instSSBO);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gpuInstances.size() * sizeof(GPUInstance), gpuInstances.data());
	
		BuildTLAS();

		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
	}

	return tlasNeedsRebuild;
}


ModelInstance* Scene::AddInstance(Model* model) {
	if (model->isSphere) return AddSphereInstance(model, 0); // Default to first material

	// If model hasn't been baked yet
	if (cookedModels.find(model) == cookedModels.end()) {
		int startNode = tlasRootIndex; // (int)blasNodes.size();
		int startTri = (int)globalTris.size();

		BVH<Triangle> blas(model->triangles, Config::BLAS_MAX_DEPTH, Config::BLAS_MIN_PRIMS);
		std::vector<GPUNode> newBlasNodes = blas.flattenNodes(startNode);
		std::vector<GPUTriangle> newBlasTris = blas.prepareTriangles();

		blasNodes.insert(blasNodes.end(), newBlasNodes.begin(), newBlasNodes.end());
		globalTris.insert(globalTris.end(), newBlasTris.begin(), newBlasTris.end());

		cookedModels[model] = ModelData{ startNode, startTri };

		tlasRootIndex = (int)blasNodes.size();
	}

	// Create ModelInstance
	ModelInstance instance(model, (int)gpuInstances.size());
	instances.push_back(instance);

	// Create GPUInstance
	GPUInstance gpuInstance{};
	gpuInstance.blasRoot = cookedModels[model].blasRoot;
	gpuInstance.meshOffset = cookedModels[model].meshOffset;
	gpuInstance.modelMatrix = instance.getTransform();
	gpuInstance.invModelMatrix = glm::inverse(gpuInstance.modelMatrix);
	gpuInstances.push_back(gpuInstance);

	return &instances.back();
}


ModelInstance* Scene::AddInstance(Model* model, int matID) {
	for (Triangle& tri : model->triangles)
		tri.materialID = matID;
	return AddInstance(model);
}


ModelInstance* Scene::AddSphereInstance(Model* model, int matID) {
	// Create ModelInstance
	ModelInstance instance(model, (int)gpuInstances.size());

	instances.push_back(instance);

	// Create GPUInstance
	GPUInstance gpuInstance{};
	gpuInstance.blasRoot = matID;
	gpuInstance.meshOffset = -1;
	gpuInstance.modelMatrix = instance.getTransform();
	gpuInstance.invModelMatrix = glm::inverse(gpuInstance.modelMatrix);
	
	gpuInstances.push_back(gpuInstance);

	return &instances.back();
}