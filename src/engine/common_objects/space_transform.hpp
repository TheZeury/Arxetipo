#pragma once

#include <unordered_set>

#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace arx
{
	auto get_position(const glm::mat4& matrix) -> glm::vec3 {
		return glm::vec3(matrix[3]);
	}

	auto get_scale(const glm::mat4& matrix) -> glm::vec3 {
		glm::mat3 mat3x3{ matrix };
		glm::vec3 unflipped = glm::vec3(glm::length(mat3x3[0]), glm::length(mat3x3[1]), glm::length(mat3x3[2]));
		float det = glm::determinant(mat3x3);
		glm::vec3 scale = det < 0 ? -unflipped : unflipped;
		return scale;
	}

	auto get_rotation(const glm::mat4& matrix) -> glm::quat {
		glm::mat3 mat3x3{ matrix };
		mat3x3 = { glm::normalize(mat3x3[0]), glm::normalize(mat3x3[1]), glm::normalize(mat3x3[2]) };
		float det = glm::determinant(mat3x3);
		mat3x3 = det < 0 ? -mat3x3 : mat3x3;
		return glm::quat_cast(mat3x3);
	}

	struct SpaceTransform
	{
	public:
		template<typename S>
		auto register_to_systems(S* s) -> void {
			// nothing to do.
		}

		auto update_matrix() -> std::tuple<bool, glm::mat4*> {
			bool updated;

			if (parent == nullptr)
			{
				updated = global_changed;
				if (updated)
				{
					global_matrix = get_local_matrix();
				}
			}
			else
			{
				auto [parent_updated, parent_transform] = parent->update_matrix();
				updated = parent_updated || global_changed;
				if (updated)
				{
					global_matrix = *parent_transform * get_local_matrix();
				}
			}

			if (updated)
			{
				for (auto& child : children)
				{
					child->global_changed = true;
				}
			}
			global_changed = false;
			return std::make_tuple(updated, &global_matrix);
		}

		auto set_local_position(const glm::vec3& pos) -> void {
			local_position = pos;
			local_changed = true;
			global_changed = true;
		}
		auto get_local_position() -> glm::vec3 {
			return local_position;
		}

		auto set_local_rotation(const glm::quat& rotat) -> void {
			local_rotation = rotat;
			local_changed = true;
			global_changed = true;
		}
		auto get_local_rotation() -> glm::quat {
			return local_rotation;
		}

		auto set_local_scale(const glm::vec3& scal) -> void {
			local_scale = scal;
			local_changed = true;
			global_changed = true;
		}
		auto get_local_scale() -> glm::vec3 {
			return local_scale;
		}

		auto set_local_matrix(const glm::mat4& mat) -> void {
			local_matrix = mat;
			local_position = get_position(mat);
			local_scale = get_scale(mat);
			local_rotation = get_rotation(mat);
			local_changed = false;
			global_changed = true;
		}
		auto get_local_matrix() -> glm::mat4 {
			if (local_changed)
			{
				local_matrix = glm::translate(glm::mat4(1.f), local_position) * glm::mat4_cast(local_rotation) * glm::scale(glm::mat4(1.f), local_scale);
				local_changed = false;
			}
			return local_matrix;
		}

		auto set_global_position(const glm::vec3& pos) -> void {
			if (parent == nullptr)
			{
				set_local_position(pos);
			}
			else
			{
				auto [parent_updated, parent_transform] = parent->update_matrix();
				set_local_position(glm::vec3(glm::inverse(*parent_transform) * glm::vec4(pos, 1.f)));
			}
		}
		auto get_global_position() -> glm::vec3 {
			update_matrix();
			return get_position(global_matrix);
		}

		auto set_global_rotation(const glm::quat& rotat) -> void {
			if (parent == nullptr)
			{
				set_local_rotation(rotat);
			}
			else
			{
				auto [parent_updated, parent_transform] = parent->update_matrix();
				set_local_rotation(glm::inverse(glm::quat(*parent_transform)) * rotat);
			}
		}
		auto get_global_rotation() -> glm::quat {
			update_matrix();
			return get_rotation(global_matrix);
		}

		auto set_global_scale(const glm::vec3& scal) -> void {
			if (parent == nullptr)
			{
				set_local_scale(scal);
			}
			else
			{
				auto [parent_updated, parent_transform] = parent->update_matrix();
				set_local_scale(glm::vec3(glm::length(glm::inverse(*parent_transform) * glm::vec4(scal, 0.f))));
			}
		}
		auto get_global_scale() -> glm::vec3 {
			update_matrix();
			return get_scale(global_matrix);
		}

		auto set_global_matrix(const glm::mat4& mat) -> void {
			if (parent == nullptr)
			{
				set_local_matrix(mat);
			}
			else
			{
				auto [parent_updated, parent_transform] = parent->update_matrix();
				set_local_matrix(glm::inverse(*parent_transform) * mat);
			}
		}
		auto get_global_matrix() -> glm::mat4 {
			update_matrix();
			return global_matrix;
		}

		auto set_parent(SpaceTransform* parent) -> void {
			if (this->parent != nullptr)
			{
				this->parent->remove_child_internal(this);
			}
			if (parent != nullptr)
			{
				parent->add_child_internal(this);
			}
		}
		auto get_parent() -> SpaceTransform* {
			return parent;
		}
		auto add_child(SpaceTransform* child) -> void {
			child->set_parent(this);
		}
		auto remove_child(SpaceTransform* child) -> void {
			child->set_parent(nullptr);
		}
		auto get_children() -> std::unordered_set<SpaceTransform*> {
			return children;
		}

	private:
		auto add_child_internal(SpaceTransform* child) -> void {
			children.insert(child);
			child->parent = this;
			child->global_changed = true;
		}
		auto remove_child_internal(SpaceTransform* child) -> void {
			children.erase(child);
			child->parent = nullptr;
			child->global_changed = true;
		}

	public:
		glm::mat4 global_matrix{ 1.f };

		glm::vec3 local_position = { 0.f, 0.f, 0.f };
		glm::quat local_rotation = { 1.f, 0.f, 0.f, 0.f };
		glm::vec3 local_scale = { 1.f, 1.f, 1.f };
		glm::mat4 local_matrix{ 1.f };

		SpaceTransform* parent = nullptr;
		std::unordered_set<SpaceTransform*> children;

		bool global_changed = true;
		bool local_changed = true;
	};
}