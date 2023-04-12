#pragma once

namespace arx
{
	struct MeshVertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;

		static std::array<vk::VertexInputBindingDescription, 1> get_binding_descriptions()
		{
			return {
				vk::VertexInputBindingDescription{ 0, sizeof(MeshVertex), vk::VertexInputRate::eVertex },
			};
		}

		static std::array<vk::VertexInputAttributeDescription, 5> get_attribute_descriptions()
		{
			return {
				vk::VertexInputAttributeDescription{ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, position) },
				vk::VertexInputAttributeDescription{ 1, 0, vk::Format::eR32G32Sfloat, offsetof(MeshVertex, uv) },
				vk::VertexInputAttributeDescription{ 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, normal) },
				vk::VertexInputAttributeDescription{ 3, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, tangent) },
				vk::VertexInputAttributeDescription{ 4, 0, vk::Format::eR32G32B32Sfloat, offsetof(MeshVertex, bitangent) },
			};
		}

		bool operator==(const MeshVertex& other) const
		{
			return position == other.position && normal == other.normal && uv == other.uv;
			// There's no need to take tangent and bitangent into consideration since they are averaged through all vertices that have the same position, uv, and normal values;
		}
	};

	using MeshIndexedTriangle = std::tuple<uint32_t, uint32_t, uint32_t>;
	using MeshTriangle = std::tuple<MeshVertex, MeshVertex, MeshVertex>;
}

namespace std
{
	template<>
	struct hash<arx::MeshVertex>
	{
		size_t operator()(arx::MeshVertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.position) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};

	template<>
	struct hash<arx::MeshIndexedTriangle>
	{
		size_t operator()(arx::MeshIndexedTriangle const& triangle) const
		{
			auto [v1, v2, v3] = triangle;
			return ((hash<uint32_t>()(v1) ^
				(hash<uint32_t>()(v2) << 1)) >> 1) ^
				(hash<uint32_t>()(v3) << 1);
		}
	};
}

namespace arx
{
	struct MeshBuilder;

	struct MeshModel
	{
	public:
		using Vertex = MeshVertex;
		using Builder = MeshBuilder;

	public:
		MeshModel(const MeshModel&) = delete;
		MeshModel& operator=(const MeshModel&) = delete;

		MeshModel(MeshModel&& rhs) {
			if (rhs.moved) {
				moved = true;
				return;
			}
			vertex_count = std::move(rhs.vertex_count);
			index_count = std::move(rhs.index_count);
			vertex_buffer = std::move(rhs.vertex_buffer);
			vertex_buffer_memory = std::move(rhs.vertex_buffer_memory);
			index_buffer = std::move(rhs.index_buffer);
			index_buffer_memory = std::move(rhs.index_buffer_memory);
			moved = false;
			rhs.moved = true;
		}
		MeshModel& operator=(MeshModel&& rhs) {
			if (rhs.moved) {
				moved = true;
				return *this;
			}
			vertex_count = std::move(rhs.vertex_count);
			index_count = std::move(rhs.index_count);
			vertex_buffer = std::move(rhs.vertex_buffer);
			vertex_buffer_memory = std::move(rhs.vertex_buffer_memory);
			index_buffer = std::move(rhs.index_buffer);
			index_buffer_memory = std::move(rhs.index_buffer_memory);
			moved = false;
			rhs.moved = true;
			return *this;
		}

		MeshModel(uint32_t vertex_count, uint32_t index_count, vk::Buffer vertex_buffer, vk::DeviceMemory vertex_buffer_memory, vk::Buffer index_buffer, vk::DeviceMemory index_buffer_memory)
			: vertex_count{ vertex_count }, index_count{ index_count }, vertex_buffer{ vertex_buffer }, vertex_buffer_memory{ vertex_buffer_memory }, index_buffer{ index_buffer }, index_buffer_memory{ index_buffer_memory }, moved{ false } {

		}
		~MeshModel() {
			if (moved) {
				return;
			}
			// TODO
		}

	public:
		uint32_t vertex_count;
		uint32_t index_count;
		vk::Buffer vertex_buffer;
		vk::DeviceMemory vertex_buffer_memory;
		vk::Buffer index_buffer;
		vk::DeviceMemory index_buffer_memory;

	private:
		bool moved = false;
	};

	struct MeshBuilder
	{
	public:
		using Vertex = MeshVertex;
		using IndexedTriangle = MeshIndexedTriangle;
		using Triangle = MeshTriangle;

	public:
		// Add a vertex and return it's index in the model.
		auto addVertex(Vertex vertex) -> uint32_t {
			if (!uniqueVertices.contains(vertex))
			{
				if (removedVertices.empty())
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}
				else
				{
					auto index = *removedVertices.begin(); removedVertices.erase(index);
					uniqueVertices[vertex] = index;
					vertices[index] = vertex;
				}
			}
			return uniqueVertices[vertex];
		}

		// Add a new Traingle based on three vertices. Counter-clock side is the front. Face index is returned.
		auto addTriangle(uint32_t v1, uint32_t v2, uint32_t v3) -> uint32_t {
			if (v1 == v2 || v2 == v3 || v3 == v1)
			{
				// throw std::runtime_error("Three vertices must be all different."); // No need to crash, just don't do anything.
				return UINT32_MAX;
			}
			auto tester = [&](uint32_t v) { if (v >= vertices.size() || removedVertices.contains(v)) throw std::runtime_error(std::format("Newly added vertex {} doesn't exist.", v)); };
			tester(v1); tester(v2); tester(v3);

			MeshIndexedTriangle triangle;

			if (v1 < v2 && v1 < v3) triangle = std::make_tuple(v1, v2, v3);		// deduplication.
			else if (v2 < v3) triangle = std::make_tuple(v2, v3, v1);			//
			else triangle = std::make_tuple(v3, v1, v2);						//

			if (!uniqueTriangles.contains(triangle))
			{
				if (removeTriangles.empty())
				{
					uniqueTriangles[triangle] = static_cast<uint32_t>(triangles.size());
					triangles.push_back(triangle);
				}
				else
				{
					auto index = *removeTriangles.begin(); removeTriangles.erase(index);
					uniqueTriangles[triangle] = index;
					triangles[index] = triangle;
				}
			}
			return uniqueTriangles[triangle];
		}

		// Add a new Traingle with three specified vertices. return a tuple of [traingle_index, vertex_index_1, vertex_index_2, vertex_index_3];
		auto addTriangle(Vertex v1, Vertex v2, Vertex v3) -> std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> {
			auto i1 = addVertex(v1), i2 = addVertex(v2), i3 = addVertex(v3);
			return std::make_tuple(addTriangle(i1, i2, i3), i1, i2, i3);
		}

		// Update a existing vertex by pass it's index and new value.
		auto updateVertex(uint32_t v, Vertex value) -> void {
			uniqueVertices.erase(vertices[v]);
			vertices[v] = value;
			uniqueVertices[value] = v;
		}

		// Remove a particular vertex. All traingles using this vertex will be deleted also. Avoid calling this since current implementaton is expensive ( Iterate through all face to find those to delete. ).
		auto removeVertex(uint32_t v) -> void {
			// TODO
			uniqueVertices.erase(vertices[v]);
			removedVertices.insert(v);

			for (auto [triangle, index] : uniqueTriangles)
			{
				auto [v1, v2, v3] = triangle;
				if (v1 == v || v2 == v || v3 == v)
				{
					removeTriangle(index);
				}
			}
		}

		// Remove a particular traingle. Vertics it's using won't be deleted.
		auto removeTriangle(uint32_t t) -> void {
			uniqueTriangles.erase(triangles[t]);
			removeTriangles.insert(t);
		}

		// 
		auto getVertex(uint32_t v) -> Vertex {
			return vertices[v];
		}

		// 
		auto getTriangle(uint32_t t) -> Triangle {
			auto [v1, v2, v3] = triangles[t];
			return std::make_tuple(vertices[v1], vertices[v2], vertices[v3]);
		}

		//
		auto getIndexedTriangle(uint32_t t) -> IndexedTriangle {
			return triangles[t];
		}

		auto build() const -> std::tuple<std::vector<Vertex>, std::vector<uint32_t>> {
			std::vector<MeshModel::Vertex> build_vertices;
			std::vector<uint32_t> build_indices;
			std::vector<uint32_t> vertices_to_build_vertices_map(static_cast<size_t>(vertices.size()));

			for (uint32_t v = 0; v < vertices.size(); ++v)
			{
				vertices_to_build_vertices_map[v] = static_cast<uint32_t>(build_vertices.size());
				if (!removedVertices.contains(v))
				{
					build_vertices.push_back(vertices[v]);
				}
			}

			for (auto& triangle : triangles)
			{
				auto [v1, v2, v3] = triangle;
				build_indices.push_back(vertices_to_build_vertices_map[v1]);
				build_indices.push_back(vertices_to_build_vertices_map[v2]);
				build_indices.push_back(vertices_to_build_vertices_map[v3]);
			}

			return std::make_tuple(build_vertices, build_indices);
		}

	public:
		static MeshBuilder Box(float halfX, float halfY, float halfZ) {
			MeshBuilder mesh;
			mesh.vertices = {
				// down
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f, -1.f,  1.f }, { 0.f, 1.f }, {  0.f, -1.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f, -1.f, -1.f }, { 1.f, 1.f }, {  0.f, -1.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f, -1.f, -1.f }, { 1.f, 0.f }, {  0.f, -1.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f, -1.f,  1.f }, { 0.f, 0.f }, {  0.f, -1.f,  0.f }, { }, { } },
				// up
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f,  1.f,  1.f }, { 0.f, 1.f }, {  0.f,  1.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f,  1.f,  1.f }, { 1.f, 1.f }, {  0.f,  1.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f,  1.f, -1.f }, { 1.f, 0.f }, {  0.f,  1.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f,  1.f, -1.f }, { 0.f, 0.f }, {  0.f,  1.f,  0.f }, { }, { } },
				// front
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f, -1.f,  1.f }, { 0.f, 1.f }, {  0.f,  0.f,  1.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f, -1.f,  1.f }, { 1.f, 1.f }, {  0.f,  0.f,  1.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f,  1.f,  1.f }, { 1.f, 0.f }, {  0.f,  0.f,  1.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f,  1.f,  1.f }, { 0.f, 0.f }, {  0.f,  0.f,  1.f }, { }, { } },
				// back
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f, -1.f, -1.f }, { 0.f, 1.f }, {  0.f,  0.f, -1.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f,  1.f, -1.f }, { 1.f, 1.f }, {  0.f,  0.f, -1.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f,  1.f, -1.f }, { 1.f, 0.f }, {  0.f,  0.f, -1.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f, -1.f, -1.f }, { 0.f, 0.f }, {  0.f,  0.f, -1.f }, { }, { } },
				// left
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f, -1.f,  1.f }, { 0.f, 1.f }, { -1.f,  0.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f,  1.f,  1.f }, { 1.f, 1.f }, { -1.f,  0.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f,  1.f, -1.f }, { 1.f, 0.f }, { -1.f,  0.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{ -1.f, -1.f, -1.f }, { 0.f, 0.f }, { -1.f,  0.f,  0.f }, { }, { } },
				// right
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f, -1.f,  1.f }, { 0.f, 1.f }, {  1.f,  0.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f, -1.f, -1.f }, { 1.f, 1.f }, {  1.f,  0.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f,  1.f, -1.f }, { 1.f, 0.f }, {  1.f,  0.f,  0.f }, { }, { } },
				Vertex{ glm::vec3{ halfX, halfY, halfZ } *glm::vec3{  1.f,  1.f,  1.f }, { 0.f, 0.f }, {  1.f,  0.f,  0.f }, { }, { } },
			};
			mesh.triangles = {
				MeshIndexedTriangle{ 0 + 0, 1 + 0, 2 + 0, },	// down
				MeshIndexedTriangle{ 2 + 0, 3 + 0, 0 + 0, },
				MeshIndexedTriangle{ 0 + 4, 1 + 4, 2 + 4, },	// up
				MeshIndexedTriangle{ 2 + 4, 3 + 4, 0 + 4, },
				MeshIndexedTriangle{ 0 + 8, 1 + 8, 2 + 8, },	// front
				MeshIndexedTriangle{ 2 + 8, 3 + 8, 0 + 8, },
				MeshIndexedTriangle{ 0 + 12, 1 + 12, 2 + 12, },	// back
				MeshIndexedTriangle{ 2 + 12, 3 + 12, 0 + 12, },
				MeshIndexedTriangle{ 0 + 16, 1 + 16, 2 + 16, },	// left
				MeshIndexedTriangle{ 2 + 16, 3 + 16, 0 + 16, },
				MeshIndexedTriangle{ 0 + 20, 1 + 20, 2 + 20, },	// right
				MeshIndexedTriangle{ 2 + 20, 3 + 20, 0 + 20, },
			};
			return mesh;
		}
		static MeshBuilder UVSphere(float radius, uint32_t rings, uint32_t segments)
		{
			MeshBuilder mesh;

			if (rings < 2) rings = 2;
			if (segments < 3) segments = 3;

			auto v0 = mesh.addVertex(Vertex{ {  0.f,  radius,  0.f }, { }, {  0.f,  1.f,  0.f }, { }, { } });

			for (uint32_t i = 0; i < rings - 1; ++i)
			{
				auto phi = glm::pi<float>() * (i + 1) / rings;
				for (uint32_t j = 0; j < segments; ++j)
				{
					auto theta = 2.f * glm::pi<float>() * j / segments;
					auto x = glm::sin(phi) * glm::cos(theta);
					auto y = glm::cos(phi);
					auto z = glm::sin(phi) * glm::sin(theta);
					mesh.addVertex(Vertex{ radius * glm::vec3{ x, y, z }, { }, glm::normalize(glm::vec3{ x, y, z }), { }, { } });
				}
			}

			auto v1 = mesh.addVertex(Vertex{ {  0.f, -radius,  0.f }, { }, {  0.f, -1.f,  0.f }, { }, { } });

			for (uint32_t i = 0; i < segments; ++i)
			{
				auto i0 = i + 1;
				auto i1 = (i + 1) % segments + 1;
				mesh.addTriangle(v0, i1, i0);
				i0 = i + segments * (rings - 2) + 1;
				i1 = (i + 1) % segments + segments * (rings - 2) + 1;
				mesh.addTriangle(v1, i0, i1);
			}

			for (uint32_t j = 0; j < rings - 2; ++j)
			{
				auto j0 = j * segments + 1;
				auto j1 = (j + 1) * segments + 1;
				for (uint32_t i = 0; i < segments; ++i)
				{
					auto i0 = j0 + i;
					auto i1 = j0 + (i + 1) % segments;
					auto i2 = j1 + (i + 1) % segments;
					auto i3 = j1 + i;
					mesh.addTriangle(i0, i1, i2);
					mesh.addTriangle(i2, i3, i0);
				}
			}

			return mesh;
		}
		static MeshBuilder Icosphere(float radius, uint32_t level) {
			MeshBuilder mesh;

			const float phi = (1.0f + glm::sqrt(5.0f)) * 0.5f; // Golden ratio.
			const float a = 1.0f;
			const float b = 1.0f / phi;

			// Icosahedron.
			mesh.vertices = {
				Vertex{ glm::normalize(glm::vec3{  0,  b, -a }), { }, glm::normalize(glm::vec3{  0,  b, -a }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{  b,  a,  0 }), { }, glm::normalize(glm::vec3{  b,  a,  0 }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{ -b,  a,  0 }), { }, glm::normalize(glm::vec3{ -b,  a,  0 }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{  0,  b,  a }), { }, glm::normalize(glm::vec3{  0,  b,  a }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{  0, -b,  a }), { }, glm::normalize(glm::vec3{  0, -b,  a }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{ -a,  0,  b }), { }, glm::normalize(glm::vec3{ -a,  0,  b }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{  0, -b, -a }), { }, glm::normalize(glm::vec3{  0, -b, -a }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{  a,  0, -b }), { }, glm::normalize(glm::vec3{  a,  0, -b }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{  a,  0,  b }), { }, glm::normalize(glm::vec3{  a,  0,  b }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{ -a,  0, -b }), { }, glm::normalize(glm::vec3{ -a,  0, -b }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{  b, -a,  0 }), { }, glm::normalize(glm::vec3{  b, -a,  0 }), { }, { } },
				Vertex{ glm::normalize(glm::vec3{ -b, -a,  0 }), { }, glm::normalize(glm::vec3{ -b, -a,  0 }), { }, { } },
			};

			std::vector<MeshIndexedTriangle> readList = {
				{  2,  1,  0 },
				{  1,  2,  3 },
				{  5,  4,  3 },
				{  4,  8,  3 },
				{  7,  6,  0 },
				{  6,  9,  0 },
				{ 11, 10,  4 },
				{ 10, 11,  6 },
				{  9,  5,  2 },
				{  5,  9, 11 },
				{  8,  7,  1 },
				{  7,  8, 10 },
				{  2,  5,  3 },
				{  8,  1,  3 },
				{  9,  2,  0 },
				{  1,  7,  0 },
				{ 11,  9,  6 },
				{  7, 10,  6 },
				{  5, 11,  4 },
				{ 10,  8,  4 },
			};

			std::vector<MeshIndexedTriangle> writeList;

			auto midpoint = [](const Vertex& va, const Vertex& vb)
			{
				auto position = glm::normalize((va.position + vb.position) / 2.f);
				return Vertex{ position, { }, position, { }, { } };
			};

			while (level--)
			{
				writeList.clear();

				for (auto& triangle : readList)
				{
					auto [v1, v2, v3] = triangle;
					auto va = mesh.addVertex(midpoint(mesh.vertices[v1], mesh.vertices[v2]));
					auto vb = mesh.addVertex(midpoint(mesh.vertices[v2], mesh.vertices[v3]));
					auto vc = mesh.addVertex(midpoint(mesh.vertices[v3], mesh.vertices[v1]));

					writeList.push_back({ v1, va, vc });
					writeList.push_back({ v2, vb, va });
					writeList.push_back({ v3, vc, vb });
					writeList.push_back({ va, vb, vc });
				}

				readList = std::move(writeList);
			}

			for (auto& v : mesh.vertices)
			{
				v.position *= radius;
			}
			mesh.triangles = std::move(readList);

			return mesh;
		}
		static MeshBuilder Cone(float bottomRadius, float topRadius, float height, uint32_t segments) {
			MeshBuilder mesh;

			if (segments < 3) segments = 3;
			if (bottomRadius < 0.f || topRadius < 0.f || height <= 0.f)
			{
				throw std::runtime_error("Invalid input parameters for a Cone.");
			}

			const float top = height / 2;
			const float bottom = -height / 2;


			// Top.
			for (uint32_t i = 0; i < segments; ++i)
			{
				auto theta = 2.f * glm::pi<float>() * i / segments;
				auto x = glm::cos(theta) * topRadius;
				auto y = top;
				auto z = glm::sin(theta) * topRadius;
				mesh.vertices.push_back(Vertex{ glm::vec3{ x, y, z }, { }, {  0.f,  1.f,  0.f }, { }, { } });
			}
			for (uint32_t i = 1; i < segments - 1; ++i)
			{
				mesh.addTriangle(0, i + 1, i);
			}

			// Bottom.
			for (uint32_t i = 0; i < segments; ++i)
			{
				auto theta = 2.f * glm::pi<float>() * i / segments;
				auto x = glm::cos(theta) * bottomRadius;
				auto y = bottom;
				auto z = glm::sin(theta) * bottomRadius;
				mesh.vertices.push_back(Vertex{ glm::vec3{ x, y, z }, { }, {  0.f, -1.f,  0.f }, { }, { } });
			}
			for (uint32_t i = 1; i < segments - 1; ++i)
			{
				mesh.addTriangle(segments + 0, segments + i, segments + i + 1);
			}

			// Lateral.
			for (uint32_t i = 0; i < segments; ++i)
			{
				auto vt = mesh.getVertex(i);
				auto vb = mesh.getVertex(segments + i);
				auto segment = vt.position - vb.position;
				auto plane = glm::normalize(glm::cross(vt.position, vb.position));
				auto normal = glm::normalize(glm::cross(plane, segment));
				vt.normal = normal;
				vb.normal = normal;
				mesh.vertices.push_back(vt);
				mesh.vertices.push_back(vb);
			}
			for (uint32_t i = 0; i < segments; ++i)
			{
				auto v1 = 2 * segments + 2 * i + 0;
				auto v2 = 2 * segments + 2 * ((i + 1) % segments) + 0;
				auto v3 = 2 * segments + 2 * ((i + 1) % segments) + 1;
				auto v4 = 2 * segments + 2 * i + 1;
				mesh.addTriangle(v1, v2, v3);
				mesh.addTriangle(v3, v4, v1);
			}

			return mesh;
		}

	private:
		std::vector<Vertex> vertices;
		std::vector<IndexedTriangle> triangles;
		std::unordered_map<Vertex, uint32_t> uniqueVertices;
		std::unordered_map<IndexedTriangle, uint32_t> uniqueTriangles;
		std::unordered_set<uint32_t> removedVertices;
		std::unordered_set<uint32_t> removeTriangles;
	};
}