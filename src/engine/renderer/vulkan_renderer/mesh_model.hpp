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
		auto add_vertex(Vertex vertex) -> uint32_t {
			if (!unique_vertices.contains(vertex))
			{
				if (removed_vertices.empty())
				{
					unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}
				else
				{
					auto index = *removed_vertices.begin(); removed_vertices.erase(index);
					unique_vertices[vertex] = index;
					vertices[index] = vertex;
				}
			}
			return unique_vertices[vertex];
		}

		// Add a new Traingle based on three vertices. Counter-clock side is the front. Face index is returned.
		auto add_triangle(uint32_t v1, uint32_t v2, uint32_t v3) -> uint32_t {
			if (v1 == v2 || v2 == v3 || v3 == v1)
			{
				// throw std::runtime_error("Three vertices must be all different."); // No need to crash, just don't do anything.
				return UINT32_MAX;
			}
			auto tester = [&](uint32_t v) { if (v >= vertices.size() || removed_vertices.contains(v)) throw std::runtime_error(std::format("Newly added vertex {} doesn't exist.", v)); };
			tester(v1); tester(v2); tester(v3);

			MeshIndexedTriangle triangle;

			if (v1 < v2 && v1 < v3) triangle = std::make_tuple(v1, v2, v3);		// deduplication.
			else if (v2 < v3) triangle = std::make_tuple(v2, v3, v1);			//
			else triangle = std::make_tuple(v3, v1, v2);						//

			if (!unique_triangles.contains(triangle))
			{
				if (removed_triangles.empty())
				{
					unique_triangles[triangle] = static_cast<uint32_t>(triangles.size());
					triangles.push_back(triangle);
				}
				else
				{
					auto index = *removed_triangles.begin(); removed_triangles.erase(index);
					unique_triangles[triangle] = index;
					triangles[index] = triangle;
				}
			}
			return unique_triangles[triangle];
		}

		// Add a new Traingle with three specified vertices. return a tuple of [traingle_index, vertex_index_1, vertex_index_2, vertex_index_3];
		auto add_triangle(Vertex v1, Vertex v2, Vertex v3) -> std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> {
			auto i1 = add_vertex(v1), i2 = add_vertex(v2), i3 = add_vertex(v3);
			return std::make_tuple(add_triangle(i1, i2, i3), i1, i2, i3);
		}

		// Update a existing vertex by pass it's index and new value.
		auto update_vertex(uint32_t v, Vertex value) -> void {
			unique_vertices.erase(vertices[v]);
			vertices[v] = value;
			unique_vertices[value] = v;
		}

		// Remove a particular vertex. All traingles using this vertex will be deleted also. Avoid calling this since current implementaton is expensive ( Iterate through all face to find those to delete. ).
		auto remove_vertex(uint32_t v) -> void {
			// TODO
			unique_vertices.erase(vertices[v]);
			removed_vertices.insert(v);

			for (auto [triangle, index] : unique_triangles)
			{
				auto [v1, v2, v3] = triangle;
				if (v1 == v || v2 == v || v3 == v)
				{
					remove_triangle(index);
				}
			}
		}

		// Remove a particular traingle. Vertics it's using won't be deleted.
		auto remove_triangle(uint32_t t) -> void {
			unique_triangles.erase(triangles[t]);
			removed_triangles.insert(t);
		}

		// 
		auto get_vertex(uint32_t v) -> Vertex {
			return vertices[v];
		}

		// 
		auto get_triangle(uint32_t t) -> Triangle {
			auto [v1, v2, v3] = triangles[t];
			return std::make_tuple(vertices[v1], vertices[v2], vertices[v3]);
		}

		//
		auto get_indexed_triangle(uint32_t t) -> IndexedTriangle {
			return triangles[t];
		}

		auto transform(const glm::mat4& matrix) -> MeshBuilder& {
			for (auto& vertex : vertices)
			{
				vertex.position = matrix * glm::vec4(vertex.position, 1.0f);
				vertex.normal = matrix * glm::vec4(vertex.normal, 0.0f);
			}
			regenerate_unique_vertices();
			return *this;
		}

		auto regenerate_unique_vertices() -> void {
			unique_vertices.clear();
			for (uint32_t v = 0; v < vertices.size(); ++v)
			{
				if (!removed_vertices.contains(v)) {
					unique_vertices[vertices[v]] = v;
				}
			}
		}

		auto build() const -> std::tuple<std::vector<Vertex>, std::vector<uint32_t>> {
			std::vector<MeshModel::Vertex> build_vertices;
			std::vector<uint32_t> build_indices;
			std::vector<uint32_t> vertices_to_build_vertices_map(static_cast<size_t>(vertices.size()));

			for (uint32_t v = 0; v < vertices.size(); ++v)
			{
				vertices_to_build_vertices_map[v] = static_cast<uint32_t>(build_vertices.size());
				if (!removed_vertices.contains(v))
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
		static MeshBuilder Box(float half_x, float half_y, float half_z) {
			MeshBuilder mesh;
			std::array<uint32_t, 24> v = {
				// down
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f, -1.f,  1.f }, .uv = { 0.f, 1.f }, .normal = {  0.f, -1.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f, -1.f, -1.f }, .uv = { 1.f, 1.f }, .normal = {  0.f, -1.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f, -1.f, -1.f }, .uv = { 1.f, 0.f }, .normal = {  0.f, -1.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f, -1.f,  1.f }, .uv = { 0.f, 0.f }, .normal = {  0.f, -1.f,  0.f }, .tangent = { }, .bitangent = { } }),
				// up									   
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f,  1.f,  1.f }, .uv = { 0.f, 1.f }, .normal = {  0.f,  1.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f,  1.f,  1.f }, .uv = { 1.f, 1.f }, .normal = {  0.f,  1.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f,  1.f, -1.f }, .uv = { 1.f, 0.f }, .normal = {  0.f,  1.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f,  1.f, -1.f }, .uv = { 0.f, 0.f }, .normal = {  0.f,  1.f,  0.f }, .tangent = { }, .bitangent = { } }),
				// front								   
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f, -1.f,  1.f }, .uv = { 0.f, 1.f }, .normal = {  0.f,  0.f,  1.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f, -1.f,  1.f }, .uv = { 1.f, 1.f }, .normal = {  0.f,  0.f,  1.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f,  1.f,  1.f }, .uv = { 1.f, 0.f }, .normal = {  0.f,  0.f,  1.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f,  1.f,  1.f }, .uv = { 0.f, 0.f }, .normal = {  0.f,  0.f,  1.f }, .tangent = { }, .bitangent = { } }),
				// back									   
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f, -1.f, -1.f }, .uv = { 0.f, 1.f }, .normal = {  0.f,  0.f, -1.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f,  1.f, -1.f }, .uv = { 1.f, 1.f }, .normal = {  0.f,  0.f, -1.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f,  1.f, -1.f }, .uv = { 1.f, 0.f }, .normal = {  0.f,  0.f, -1.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f, -1.f, -1.f }, .uv = { 0.f, 0.f }, .normal = {  0.f,  0.f, -1.f }, .tangent = { }, .bitangent = { } }),
				// left									   
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f, -1.f,  1.f }, .uv = { 0.f, 1.f }, .normal = { -1.f,  0.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f,  1.f,  1.f }, .uv = { 1.f, 1.f }, .normal = { -1.f,  0.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f,  1.f, -1.f }, .uv = { 1.f, 0.f }, .normal = { -1.f,  0.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{ -1.f, -1.f, -1.f }, .uv = { 0.f, 0.f }, .normal = { -1.f,  0.f,  0.f }, .tangent = { }, .bitangent = { } }),
				// right								   
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f, -1.f,  1.f }, .uv = { 0.f, 1.f }, .normal = {  1.f,  0.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f, -1.f, -1.f }, .uv = { 1.f, 1.f }, .normal = {  1.f,  0.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f,  1.f, -1.f }, .uv = { 1.f, 0.f }, .normal = {  1.f,  0.f,  0.f }, .tangent = { }, .bitangent = { } }),
				mesh.add_vertex(Vertex{ .position = glm::vec3{ half_x, half_y, half_z } * glm::vec3{  1.f,  1.f,  1.f }, .uv = { 0.f, 0.f }, .normal = {  1.f,  0.f,  0.f }, .tangent = { }, .bitangent = { } }),
			};
			{
				mesh.add_triangle(v[0] + v[ 0], v[1] + v[ 0], v[2] + v[ 0]);	// down
				mesh.add_triangle(v[2] + v[ 0], v[3] + v[ 0], v[0] + v[ 0]);
				mesh.add_triangle(v[0] + v[ 4], v[1] + v[ 4], v[2] + v[ 4]);	// up
				mesh.add_triangle(v[2] + v[ 4], v[3] + v[ 4], v[0] + v[ 4]);
				mesh.add_triangle(v[0] + v[ 8], v[1] + v[ 8], v[2] + v[ 8]);	// front
				mesh.add_triangle(v[2] + v[ 8], v[3] + v[ 8], v[0] + v[ 8]);
				mesh.add_triangle(v[0] + v[12], v[1] + v[12], v[2] + v[12]);	// back
				mesh.add_triangle(v[2] + v[12], v[3] + v[12], v[0] + v[12]);
				mesh.add_triangle(v[0] + v[16], v[1] + v[16], v[2] + v[16]);	// left
				mesh.add_triangle(v[2] + v[16], v[3] + v[16], v[0] + v[16]);
				mesh.add_triangle(v[0] + v[20], v[1] + v[20], v[2] + v[20]);	// right
				mesh.add_triangle(v[2] + v[20], v[3] + v[20], v[0] + v[20]);
			};
			return mesh;
		}
		static MeshBuilder UVSphere(float radius, uint32_t rings, uint32_t segments)
		{
			MeshBuilder mesh;

			if (rings < 2) rings = 2;
			if (segments < 3) segments = 3;

			std::vector<uint32_t> v;

			auto v0 = mesh.add_vertex(Vertex{ {  0.f,  radius,  0.f }, { }, {  0.f,  1.f,  0.f }, { }, { } });
			v.push_back(v0);

			for (uint32_t i = 0; i < rings - 1; ++i)
			{
				auto phi = glm::pi<float>() * (i + 1) / rings;
				for (uint32_t j = 0; j < segments; ++j)
				{
					auto theta = 2.f * glm::pi<float>() * j / segments;
					auto x = glm::sin(phi) * glm::cos(theta);
					auto y = glm::cos(phi);
					auto z = glm::sin(phi) * glm::sin(theta);
					v.push_back(mesh.add_vertex(Vertex{ radius * glm::vec3{ x, y, z }, { }, glm::normalize(glm::vec3{ x, y, z }), { }, { } }));
				}
			}

			auto v1 = mesh.add_vertex(Vertex{ {  0.f, -radius,  0.f }, { }, {  0.f, -1.f,  0.f }, { }, { } });
			v.push_back(v1);

			for (uint32_t i = 0; i < segments; ++i)
			{
				auto i0 = i + 1;
				auto i1 = (i + 1) % segments + 1;
				mesh.add_triangle(v0, v[i1], v[i0]);
				i0 = i + segments * (rings - 2) + 1;
				i1 = (i + 1) % segments + segments * (rings - 2) + 1;
				mesh.add_triangle(v1, v[i0], v[i1]);
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
					mesh.add_triangle(v[i0], v[i1], v[i2]);
					mesh.add_triangle(v[i2], v[i3], v[i0]);
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
			std::vector<Vertex> vertices = {
				Vertex{ .position = glm::normalize(glm::vec3{  0,  b, -a }), .uv = { }, .normal = glm::normalize(glm::vec3{  0,  b, -a }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{  b,  a,  0 }), .uv = { }, .normal = glm::normalize(glm::vec3{  b,  a,  0 }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{ -b,  a,  0 }), .uv = { }, .normal = glm::normalize(glm::vec3{ -b,  a,  0 }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{  0,  b,  a }), .uv = { }, .normal = glm::normalize(glm::vec3{  0,  b,  a }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{  0, -b,  a }), .uv = { }, .normal = glm::normalize(glm::vec3{  0, -b,  a }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{ -a,  0,  b }), .uv = { }, .normal = glm::normalize(glm::vec3{ -a,  0,  b }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{  0, -b, -a }), .uv = { }, .normal = glm::normalize(glm::vec3{  0, -b, -a }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{  a,  0, -b }), .uv = { }, .normal = glm::normalize(glm::vec3{  a,  0, -b }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{  a,  0,  b }), .uv = { }, .normal = glm::normalize(glm::vec3{  a,  0,  b }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{ -a,  0, -b }), .uv = { }, .normal = glm::normalize(glm::vec3{ -a,  0, -b }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{  b, -a,  0 }), .uv = { }, .normal = glm::normalize(glm::vec3{  b, -a,  0 }), .tangent = { }, .bitangent = { } },
				Vertex{ .position = glm::normalize(glm::vec3{ -b, -a,  0 }), .uv = { }, .normal = glm::normalize(glm::vec3{ -b, -a,  0 }), .tangent = { }, .bitangent = { } },
			};

			std::vector<MeshIndexedTriangle> read_list = {
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

			std::vector<MeshIndexedTriangle> write_list;

			auto midpoint = [](const Vertex& va, const Vertex& vb)
			{
				auto position = glm::normalize((va.position + vb.position) / 2.f);
				return Vertex{ position, { }, position, { }, { } };
			};

			while (level--)
			{
				write_list.clear();

				for (auto& triangle : read_list)
				{
					auto [v1, v2, v3] = triangle;
					auto va = vertices.size();
					vertices.push_back(midpoint(vertices[v1], vertices[v2]));
					auto vb = vertices.size();
					vertices.push_back(midpoint(vertices[v2], vertices[v3]));
					auto vc = vertices.size();
					vertices.push_back(midpoint(vertices[v3], vertices[v1]));

					write_list.push_back({ v1, va, vc });
					write_list.push_back({ v2, vb, va });
					write_list.push_back({ v3, vc, vb });
					write_list.push_back({ va, vb, vc });
				}

				read_list = std::move(write_list);
			}

			std::vector<uint32_t> v;
			v.reserve(vertices.size());
			for (auto& vertex : vertices)
			{
				vertex.position *= radius;
				v.push_back(mesh.add_vertex(vertex));
			}

			for (auto& triangle : read_list)
			{
				mesh.add_triangle(v[std::get<0>(triangle)], v[std::get<1>(triangle)], v[std::get<2>(triangle)]);
			}

			return mesh;
		}
		static MeshBuilder Cone(float bottom_radius, float top_radius, float height, uint32_t segments) {
			MeshBuilder mesh;

			if (segments < 3) segments = 3;
			if (bottom_radius < 0.f || top_radius < 0.f || height <= 0.f)
			{
				throw std::runtime_error("Invalid input parameters for a Cone.");
			}

			const float top = height / 2;
			const float bottom = -height / 2;

			std::vector<uint32_t> v;

			// Top.
			for (uint32_t i = 0; i < segments; ++i)
			{
				auto theta = 2.f * glm::pi<float>() * i / segments;
				auto x = glm::cos(theta) * top_radius;
				auto y = top;
				auto z = glm::sin(theta) * top_radius;
				v.push_back(mesh.add_vertex(Vertex{ glm::vec3{ x, y, z }, { }, {  0.f,  1.f,  0.f }, { }, { } }));
			}
			for (uint32_t i = 1; i < segments - 1; ++i)
			{
				mesh.add_triangle(v[0], v[i + 1], v[i]);
			}

			// Bottom.
			for (uint32_t i = 0; i < segments; ++i)
			{
				auto theta = 2.f * glm::pi<float>() * i / segments;
				auto x = glm::cos(theta) * bottom_radius;
				auto y = bottom;
				auto z = glm::sin(theta) * bottom_radius;
				v.push_back(mesh.add_vertex(Vertex{ glm::vec3{ x, y, z }, { }, {  0.f, -1.f,  0.f }, { }, { } }));
			}
			for (uint32_t i = 1; i < segments - 1; ++i)
			{
				mesh.add_triangle(v[segments + 0], v[segments + i], v[segments + i + 1]);
			}

			// Lateral.
			for (uint32_t i = 0; i < segments; ++i)
			{
				auto vt = mesh.get_vertex(v[i]);
				auto vb = mesh.get_vertex(v[segments + i]);
				auto segment = vt.position - vb.position;
				auto plane = glm::normalize(glm::cross(vt.position, vb.position));
				auto normal = glm::normalize(glm::cross(plane, segment));
				vt.normal = normal;
				vb.normal = normal;
				v.push_back(mesh.add_vertex(vt));
				v.push_back(mesh.add_vertex(vb));
			}
			for (uint32_t i = 0; i < segments; ++i)
			{
				auto v1 = v[2 * segments + 2 * i + 0];
				auto v2 = v[2 * segments + 2 * ((i + 1) % segments) + 0];
				auto v3 = v[2 * segments + 2 * ((i + 1) % segments) + 1];
				auto v4 = v[2 * segments + 2 * i + 1];
				mesh.add_triangle(v1, v2, v3);
				mesh.add_triangle(v3, v4, v1);
			}

			return mesh;
		}

	private:
		std::vector<Vertex> vertices;
		std::vector<IndexedTriangle> triangles;
		std::unordered_map<Vertex, uint32_t> unique_vertices;
		std::unordered_map<IndexedTriangle, uint32_t> unique_triangles;
		std::unordered_set<uint32_t> removed_vertices;
		std::unordered_set<uint32_t> removed_triangles;
	};
}