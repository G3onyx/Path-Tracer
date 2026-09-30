#include "BVH.h"


template <typename T>
AABB BVH<T>::makeAABB(int start, int count) {
	glm::vec3 min(FLT_MAX);
	glm::vec3 max(-FLT_MAX);

	for (int i = start; i < start + count; ++i) {
		AABB b = getBounds(primitives[i]);
		min = glm::min(min, b.min);
		max = glm::max(max, b.max);
	}
	return AABB{ min - glm::vec3(0.001f), max + glm::vec3(0.001f) };
}

template <typename T>
int BVH<T>::splitMTLA(AABB bounds, int start, int count) {
	
	// Calculate longest axis
	glm::vec3 size = bounds.max - bounds.min;
	int axis = 0; // x=0, y=1, z=2
	if (size.y > size.x) axis = 1;
	if (size.z > (axis == 0 ? size.x : size.y)) axis = 2;

	// Calculate average of centroids along axis
	float splitPos = 0.f;
	for (int i = start; i < start + count; ++i)
		splitPos += getCentroid(primitives[i])[axis];
	splitPos /= (float)count;

	// Split
	auto it = std::partition(primitives.begin() + start, primitives.begin() + start + count,
		[axis, splitPos](const T& p) {
			return getCentroid(p)[axis] < splitPos;
		});

	int leftCount = (int)std::distance(primitives.begin() + start, it);

	// Handle case with empty split
	if (leftCount == 0 || leftCount == count)
		leftCount = count / 2;

	return leftCount;
}


auto getArea = [](const AABB& bounds) {
	glm::vec3 size = glm::max(glm::vec3(0.f), bounds.max - bounds.min);
	return 2.f * (size.x * size.y + size.x * size.z + size.y * size.z);
};

struct Bin {
	AABB bounds = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
	int count = 0;
};

template <typename T>
int BVH<T>::splitSAH(AABB bounds, int start, int count) {
	constexpr int BINS = 16; // best research has found

	float bestCost = FLT_MAX;
	int bestAxis = -1;
	int bestSplitBin = -1;

	// Calculate the bounds of the centroids to determine bin extents
	glm::vec3 cmin(FLT_MAX);
	glm::vec3 cmax(-FLT_MAX);
	for (int i = start; i < start + count; ++i) {
		glm::vec3 c = getCentroid(primitives[i]);
		cmin = glm::min(cmin, c);
		cmax = glm::max(cmax, c);
	}
	glm::vec3 cDims = cmax - cmin;
	
	// Find optimal axis and position to split
	for (int axis = 0; axis < 3; ++axis) {
		if (cDims[axis] == 0.f) continue; // skip axis if bounds are flat

		Bin bins[BINS];

		// Add primitives to bins
		for (int i = start; i < start + count; ++i) {
			glm::vec3 c = getCentroid(primitives[i]);
			float pos = (c[axis] - cmin[axis]) / cDims[axis];

			int bindex = std::min((int)(pos * BINS), BINS - 1);

			bins[bindex].count++;

			// Expand bounds of the bin to include the primitive
			AABB pBounds = getBounds(primitives[i]);
			bins[bindex].bounds.min = glm::min(bins[bindex].bounds.min, pBounds.min);
			bins[bindex].bounds.max = glm::max(bins[bindex].bounds.max, pBounds.max);
		}

		// Sweep forwards, calculate and store left side areas/counts
		float leftAreas[BINS - 1];
		int leftCounts[BINS - 1];
		AABB leftBox = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
		int leftCount = 0;

		for (int i = 0; i < BINS - 1; ++i) {
			leftCount += bins[i].count;
			leftCounts[i] = leftCount;

			leftBox.min = glm::min(leftBox.min, bins[i].bounds.min);
			leftBox.max = glm::max(leftBox.max, bins[i].bounds.max);
			leftAreas[i] = getArea(leftBox);
		}

		// Sweep backwards, calculate right side areas/counts, evaluate SAH costs
		AABB rightBox = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
		int rightCount = 0;

		for (int i = BINS - 1; i > 0; --i) {
			rightCount += bins[i].count;

			rightBox.min = glm::min(rightBox.min, bins[i].bounds.min);
			rightBox.max = glm::max(rightBox.max, bins[i].bounds.max);

			float cost = (leftAreas[i - 1] * leftCounts[i - 1]) + (getArea(rightBox) * rightCount);

			if (cost < bestCost) {
				bestCost = cost;
				bestAxis = axis;
				bestSplitBin = i - 1;
			}
		}
	}

	if (bestAxis == -1) return count / 2; // bail if all primitives are on the exact same point

	// Sort primitives using the optimal axis and split position
	auto it = std::partition(primitives.begin() + start, primitives.begin() + start + count,
		[bestAxis, bestSplitBin, cmin, cDims, BINS](const T& p) {
			glm::vec3 c = getCentroid(p);
			float pos = (c[bestAxis] - cmin[bestAxis]) / cDims[bestAxis];
			int bindex = std::min((int)(pos * BINS), BINS - 1);
			return bindex <= bestSplitBin;
		});
	int leftFinalCount = (int)std::distance(primitives.begin() + start, it);
	
	if (leftFinalCount == 0 || leftFinalCount == count) return count / 2; // bail if partition puts everything on one side

	return leftFinalCount;
}

template <typename T>
void BVH<T>::collapseToQuad(BVHNode* node) {
	if (!node || node->isLeaf()) return;

	// While node has room for more children
	while (node->childCount < 4) {

		// Find internal node child with the largest surface area
		int splitIndex = -1;
		float maxArea = -1.f;

		for (int i = 0; i < node->childCount; ++i) {
			if (!node->children[i]->isLeaf()) {
				float area = getArea(node->children[i]->bounds);
				if (area > maxArea) {
					maxArea = area;
					splitIndex = i;
				}
			}
		}

		if (splitIndex == -1) break; // done if all children are leaves

		// Replace internal node with it's 2 children
		BVHNode* toSplit = node->children[splitIndex];

		node->children[splitIndex] = toSplit->children[0]; // replace internal node with it's first child
		node->children[node->childCount++] = toSplit->children[1]; // append second child

		delete toSplit;
	}

	// Collapse the rest of the tree (recursive DFS)
	for (int i = 0; i < node->childCount; ++i)
		collapseToQuad(node->children[i]);

	// Recalculate number of child nodes per node
	node->numNodes = 1;
	for (int i = 0; i < node->childCount; ++i)
		node->numNodes += node->children[i]->numNodes;
}


template <typename T>
BVHNode* BVH<T>::build(int start, int count, unsigned int depth) {

	BVHNode* node = new BVHNode(); // leaf or internal
	node->bounds = makeAABB(start, count);

	// Set as leaf node if primitives are low enough or max depth reached
	if (count <= MAX_PRIMS || depth == 0) {
		node->start = start;
		node->count = count;
		node->numNodes = 1;
		node->childCount = 0; // mark as leaf node
		return node;
	}

	int leftCount = splitSAH(node->bounds, start, count);

	// Building binary tree first
	node->childCount = 2; 
	node->children[0] = build(start, leftCount, depth - 1);
	node->children[1] = build(start + leftCount, count - leftCount, depth - 1);

	node->count = 0; // mark as internal node
	node->numNodes = 1 + node->children[0]->numNodes + node->children[1]->numNodes;

	return node;
}

template <typename T>
std::vector<GPUNode> BVH<T>::flattenNodes(int globalIndex) {
	std::vector<GPUNode> flatNodes{};

	if (root) {
		collapseToQuad(root);

		flatNodes.reserve(root->numNodes);
		flatten(root, flatNodes, globalIndex);
	}

	return flatNodes;
}

template <typename T>
int BVH<T>::flatten(BVHNode* node, std::vector<GPUNode>& flatNodes, int indexOffset) {

	int index = (int)flatNodes.size();
	flatNodes.emplace_back();

	flatNodes[index].min = node->bounds.min;

	glm::vec3 dims = node->bounds.max - node->bounds.min;
	flatNodes[index].scaledDims = dims / 255.f;

	glm::vec3 invDims = 1.0f / glm::max(dims, glm::vec3(1e-7f));

	uint32_t final_qminx = 0, final_qmaxx = 0, final_qminy = 0, final_qmaxy = 0, final_qminz = 0, final_qmaxz = 0;

	auto qMin = [](float cMin, float pMin, float invExt) -> uint32_t {
		return (uint32_t)std::clamp(std::floor(255.f * (cMin - pMin) * invExt), 0.f, 255.f);
	};
	auto qMax = [](float cMax, float pMin, float invExt) -> uint32_t {
		return (uint32_t)std::clamp(std::ceil(255.f * (cMax - pMin) * invExt), 0.f, 255.f);
	};

	for (int i = 0; i < 4; ++i) {
		if (i < node->childCount) {
			BVHNode* child = node->children[i];

			// Quantize bounds for children
			uint32_t cxMin = qMin(child->bounds.min.x, flatNodes[index].min.x, invDims.x);
			uint32_t cxMax = qMax(child->bounds.max.x, flatNodes[index].min.x, invDims.x);
			uint32_t cyMin = qMin(child->bounds.min.y, flatNodes[index].min.y, invDims.y);
			uint32_t cyMax = qMax(child->bounds.max.y, flatNodes[index].min.y, invDims.y);
			uint32_t czMin = qMin(child->bounds.min.z, flatNodes[index].min.z, invDims.z);
			uint32_t czMax = qMax(child->bounds.max.z, flatNodes[index].min.z, invDims.z);

			// Pack child bounds into single integers (4x 8 bits for each axis)
			final_qminx |= (cxMin << (i * 8)); final_qmaxx |= (cxMax << (i * 8));
			final_qminy |= (cyMin << (i * 8)); final_qmaxy |= (cyMax << (i * 8));
			final_qminz |= (czMin << (i * 8)); final_qmaxz |= (czMax << (i * 8));

			// Encode child data
			if (child->isLeaf()) {
				uint32_t leafCount = child->count & 0x7F;
				uint32_t offset = (uint32_t)getLeafOffset(primitives, child->start) & 0xFFFFFF;
				flatNodes[index].childData[i] = (1u << 31) | (leafCount << 24) | offset;
			}
			else {
				int childIndex = flatten(child, flatNodes, indexOffset);
				flatNodes[index].childData[i] = childIndex + indexOffset;
			}
		}
		else {
			// Give unused child slots impossible bounding boxes (so all checks are rejected)
			final_qminx |= (255u << (i * 8)); final_qmaxx |= (0u << (i * 8));
			final_qminy |= (255u << (i * 8)); final_qmaxy |= (0u << (i * 8));
			final_qminz |= (255u << (i * 8)); final_qmaxz |= (0u << (i * 8));
			flatNodes[index].childData[i] = 0;
		}
	}

	flatNodes[index].qminx = final_qminx; flatNodes[index].qmaxx = final_qmaxx;
	flatNodes[index].qminy = final_qminy; flatNodes[index].qmaxy = final_qmaxy;
	flatNodes[index].qminz = final_qminz; flatNodes[index].qmaxz = final_qmaxz;

	return index;
}

// TODO: Either pack and use UVs for albedo texturing, or remove 'extra' to reduce memory
std::vector<GPUTriangle> BVH<Triangle>::prepareTriangles() {
	std::vector<GPUTriangle> gpuTris{};
	gpuTris.reserve(primitives.size());

	for (const Triangle& t : primitives) {
		GPUTriangle gt{};
		gt.a = glm::vec4(t.a, 0.f);
		gt.b = glm::vec4(t.b - t.a, 0.f);
		gt.c = glm::vec4(t.c - t.a, (float)t.materialID);
		gpuTris.push_back(gt);
	}

	return gpuTris;
}

// DUMMY
template <typename T>
std::vector<GPUTriangle> BVH<T>::prepareTriangles() {
	return std::vector<GPUTriangle>();
}

template class BVH<Triangle>;
template class BVH<ModelInstance>;