#define RE_MAXJOINTS 128

struct ModelTransformBlock {
	mat4 matrix;
	mat4 prevMatrix;
};

struct NodeTransformBlock {
	mat4 prevMatrix;
	mat4 matrix;
	float jointCount;
	float padding[15];
};

struct SkinTransformBlock {
	mat4 jointMatrix[RE_MAXJOINTS];
	mat4 prevJointMatrix[RE_MAXJOINTS];
};

layout(binding = 0) uniform UBOView {
	mat4 view;
	mat4 projection;
	mat4 prevView;
	vec3 cameraPos;
	vec2 haltonJitter;
	vec2 clipData;			// x = near plane, y = far plane
	ivec2 raycastTarget;
	float padding[6];
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