#define RE_MAXJOINTS 128

struct ModelTransformBlock {
	mat4 matrix;
	mat4 prevMatrix;
};

struct NodeTransformBlock {
	mat4 matrix;
	float jointCount;
	mat4 prevMatrix;
	float padding[12];
};

struct SkinTransformBlock {
	mat4 jointMatrix[RE_MAXJOINTS];
	mat4 prevJointMatrix[RE_MAXJOINTS];
};

layout(binding = 0) uniform UBOView {
	mat4 view;
	mat4 projection;
	vec3 cameraPos;
	vec2 haltonJitter;
} scene;

layout (set = 1, binding = 0) buffer UBOMesh0 {
	ModelTransformBlock block[];
} model;

layout (set = 1, binding = 1) buffer UBOMesh1 {
	NodeTransformBlock block[];
} node;

layout (set = 1, binding = 2) buffer UBOMesh2 {
	SkinTransformBlock block[];
} skin;