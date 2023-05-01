# Components

Components are small pieces of game logics that require to be *registered* to one or more systems to function.

## Interface

A type is considered as a component type as long as it has the following member function:

```cpp
template<typename Systems>
auto register_to_systems(Systems* systems) -> void;
```

## `register_to_systems`

Example:

```cpp
template<typename Systems>
auto register_to_systems(Systems* systems) -> void
{
	if constexpr (systems->contains<MySystemA>()) {
		auto system_a = systems->get<MySystemA>();
		// ...
	}

	if constexpr (systems->contains<MySystemB>()) {
		auto system_b = systems->get<MySystemB>();
		// ...
	}
}
```

You won't get any compiler error if the `Systems` as a static system componsition doesn't contain the sepicified system or as a concrete system type that is not the specified one, as that can be infered in compile time and will be ignored thanks to `if constexpr`. 
It's up to you to decide if you want to add `requires` or `static_assert`, but it's recommended to leave it as it is.

`register_to_systems` can be called multiple times on different systems, sometimes this way is more convenient than calling it once on a system composition that contains all the systems when if register to a particuar system depends.

```cpp
struct MyComponent
{
	template<typename Systems>
	auto register_to_systems(Systems* systems) -> void
	{
		if constexpr (systems->contains<PhysicsSystem>()) {
			auto system_a = systems->get<PhysicsSystem>();
			// ...
		}
		if constexpr (systems->contains<GraphicsSystem>()) {
			auto system_b = systems->get<GraphicsSystem>();
			// ...
		}
	}

	// ...
};

MyComponent component;

component.register_to_systems(&physics_systems);

if constexpr (visual_debug) {
	component.register_to_systems(&graphics_systems);
}
```

## Working with Systems

Most components can't participate in the game logics on their own without registering to any systems.
You should focus on designing systems and make the systems to organize how components work. 
Therefore, Arxetipo's components by themselves may not have `update` or `on_enable` functions, but you can implement such things that are called by systems.

Arxetipo at first may not be intuitive compare to many game engines where learners first add a "thing" called "game object", "actor" or "entity" to the scene, then add components to form that "thing".
With Arxetipo, you first design systems, and then design components that work with the systems.
It might be quite abstractive to start with, but it's more compact, flexible and potentially more efficient.

## Component Composition

Just like systems, components can be composed to form a new component.

The `register_to_systems` function of a component composition will call the `register_to_systems` function of each component it contains.

By definition, a component composition is still considered as a component, despite it's concept to gather components together may be similar to what an "entity" does in a typical ECS architecture.

There are also two engine-defined composition types: Static Component Composition and Dynamic Component Composition, 
while you always implement something similar to that on your own.

### Static Component Composition

```cpp
template<typename... Types>
struct StaticComponentComposition;
```

#### Usage

```cpp
using ModelComponent = StaticComponentComposition<SpaceTransform, MeshModelComponent, RigidDynamicComponent>;

ModelComponent model_component{ /*...*/ };
model_component.register_to_systems(&physics_systems);
model_component.register_to_systems(&graphics_systems);
```

### Dynamic Component Composition

```cpp
struct DynamicComponentComposition;
```

#### Usage

```cpp
DynamicComponentComposition model_component;
model_component.add<SpaceTransform>({ /*...*/ });
model_component.add<MeshModelComponent>({ /*...*/ });
model_component.add<RigidStaticComponent>({ /*...*/ });
model_component.remove<RigidStaticComponent>();
model_component.add<RigidDynamicComponent>({ /*...*/ });

if (simulating) {
	model_component.register_to_systems(&physics_systems);
}

if (visual_debug) {
	model_component.register_to_systems(&graphics_systems);
}
else {
	model_component.get<MeshModelComponent>().register_to_systems(&graphics_systems);
	// RigidDynamicComponent is not registered to graphics_systems.
}
```