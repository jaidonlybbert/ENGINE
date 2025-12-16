#include<optional>
#include<nlohmann/json.hpp>

// An renderer-independent Mesh representation based on the GLTF 2.0 spec

struct BufferView {
	size_t buffer;  // index of the buffer
	std::optional<size_t> byteOffset;  // offset into the buffer in bytes
	size_t byteLength;  // the length of bufferView in bytes
	std::optional<size_t> byteStride;  // The stride, in bytes
	std::optional<unsigned int> target;  // the hint representing the intended GPU buffer to use with this buffer view
	std::optional<std::string> name;  // the user-defined name of this object
	std::optional<nlohmann::json> extension;  // JSON object with extension-specific objects
	std::optional<nlohmann::json> extras;  // Application-specific data
};

enum ComponentType {
	BYTE,
	UNSIGNED_BYTE,
	SHORT,
	UNSIGNED_SHORT,
	UNSIGNED_INT,
	FLOAT
};

struct Sparse {
	// unimplemented
};

enum AccessorType {
	SCALAR,
	VEC2,
	VEC3,
	VEC4,
	MAT2,
	MAT3,
	MAT4
};

std::unordered_map<AccessorType, std::string> accessor_type_to_string {
	{AccessorType::SCALAR, "SCALAR"},
	{AccessorType::VEC2, "VEC2"},
	{AccessorType::VEC3, "VEC3"},
	{AccessorType::VEC4, "VEC4"},
	{AccessorType::MAT2, "MAT2"},
	{AccessorType::MAT3, "MAT3"},
	{AccessorType::MAT4, "MAT4"}
};

struct Accessor {
	size_t bufferView;  // index of the bufferView
	size_t byteOffset = 0;  // the offset relative to the start of the buffer view in bytes
	ComponentType componentType;  // the datatype of the accessor's components
	bool normalized = false;  // whether integer data values are normalized before usage
	size_t count;  // the number of elements referenced by this accessor
	std::string type;  // specifies if the accessor's elements are scalars, vectors, or matrices
	std::optional<short> max;  // maximum value of each component in this accessor
	std::optional<short> min;  // minimum value of each component in this accessor
	std::optional<Sparse> sparse;  // sparse storage of elements that deviate from their initialization value
	std::optional<std::string> name; // the user-defined name of this object
	std::optional<nlohmann::json> extension;  // JSON object with extension-specific objects
	std::optional<nlohmann::json> extras;  // Application-specific data
};

enum MeshPrimitiveMode {
	POINTS,
	LINES,
	LINE_LOOP,
	LINE_STRIP,
	TRIANGLES,
	TRIANGLES_STRIP,
	TRIANGLE_FAN
};

struct MorphTarget {

};

struct MeshPrimitive {
	// A plain json object, where each key corresponds to a mesh attribute semantic and each value
	// is the index of the accessor containing attribute's data
	nlohmann::json attributes;  
	std::optional<size_t> indices;  // the index of the material to apply this primitive when rendering
	unsigned int mode = 4;  // the topology type of primitives to render, default is 4: TRIANGLES
	std::vector<MorphTarget> targets;	// an array of morph targets
	std::optional<nlohmann::json> extension;  // JSON object with extension-specific objects
	std::optional<nlohmann::json> extras;  // Application-specific data
};

struct Mesh {
	// a vector of primitives, each defining geometry to be rendered
	std::vector<MeshPrimitive> primitives;
	// vector of weights to be applied to the morph targets. The number of array elements
	// must match the number of morph targets
	std::vector<size_t> weights;  
	std::optional<std::string> name;  // The user-defined name of the object
	std::optional<nlohmann::json> extension;  // JSON object with extension-specific objects
	std::optional<nlohmann::json> extras;  // Application-specific data
};
