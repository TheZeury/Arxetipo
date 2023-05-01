# Systems

Systems are managers of related game logics, and can be based on foundations and other systems. 

## Interfaces (aka. Concept)

By convention a system type is a type that implements the following member functions:

```cpp
auto mobilize() -> void;
auto freeze() -> void;
auto update() -> void;

template<typename System>
requires /*...*/
auto get() -> System*;

template<typename System>
auto contains() -> bool;
```

The `get` and `contains` function can be useful when extracting a particular system from a [**System Composition**](#system-composition). 
For a single system which you would most likely write, it should be implemented as follows:

```cpp
struct MySystem {
	// ... 

	template<typename System>
	requires std::same_as<System, MySystem>
	auto get() noexcept -> System* {
		return this;
	}

	template<typename System>
	constexpr auto contains() -> bool {
		return std::same_as<System, MySystem>;
	}
};
```

## System Composition

A system composition is a type that contains a collection of systems. 
It is also a system itself as it implements the same interface as a system. 

Arxetipo implements are two kinds of system compositions: **Static System Composition** and **Dynamic System Composition**.

regradless of which kind a system composition is, it behaves as:

- call the `mobilize` functions of all systems it contains when `mobilize` is called.
- call the `freeze` functions of all systems it contains when `freeze` is called.
- call the `update` functions of all systems it contains when `update` is called.
- return a pointer to the required system when `get` is called.

The most common use case is to pass a System Composition which contains all required systems when registering a component to systems using its member function `register_to_systems`. 
See more: [Component](component.html#register_to_systems).

On the other hand, `mobilize`, `freeze` and `update` functions of a system composition are not commonly used, as the exact point at which they should be called varies from system to system. 
But they can still be useful when some systems are truely highly associated.

Both static system composition and dynamic system composition excute underlying systems in the order they are added. 
You can write your own system composition in the way you would like it to be.

### Static System Composition

```cpp
template<typename... Types>
struct StaticSystemComposition;
```

A static system composition is a system composition which types of systems it contains are known at compile time. 
Removing or adding a new system to a static system after initializion **IS NOT** possible.

#### Usage

```cpp
StaticSystemCompostion<GraphicsSystem, XRSystem, PhysicsSystem> systems{ { }, { }, { } };

RigidDynamicComponent ball{ /*...*/ }; // A component that can be registered to both GraphicsSystem and PhysicsSystem.

ball.register_to_systems(systems);
```

#### `get`

```cpp
template<typename System>
requires (std::same_as<System, Types> || ...)
auto get() noexcept -> System*;
```

Any system type that is not contained in the static system composition will cause a compile error.

The exact signature is different from above because of the implementation, but they work the same.

#### `contains`

```cpp
template<typename System>
constexpr auto contains() -> bool {
	return std::same_as<System, Types> || ...;
}
```

A constexpr function that is evaluated at compile time.

### Dynamic System Composition

```
struct DynamicSystemComposition;
```

A dynamic system composition is a system composition which types of systems it contains are known at runtime. 
Removing or adding a new system to a dynamic system after initializion **IS** possible.

#### Usage

```cpp
DynamicSystemComposition systems;
systems.add<GraphicsSystem>();
systems.add<XRSystem>();
systems.add<PhysicsSystem>();
systems.remove<XRSystem>();

RigidDynamicComponent ball{ /*...*/ }; 

ball.register_to_systems(systems);
```

#### `get`

```cpp
template<typename System>
auto get() -> System*;
```

Any system type that is not contained in the dynamic system composition will cause a runtime error.

#### `contains`

```cpp
template<typename System>
auto contains() -> bool;
```

Return if it contains a system at runtime.

### Compare of `StaticSystemCompositon` and `DynamicSystemComposition`

| Properties | StaticSystemComposition | DynamicSystemComposition |
| --- | --- | --- |
| Memory | On stack | On heap |
| Add System | Not allowed | Allowed |
| Remove System | Not allowed | Allowed |
| `get` type check | Compile time | Runtime |
| `get` noexcept | True | False |
| constexpr `contains` | True | False |
