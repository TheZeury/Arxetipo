# Foundations and Objects

## Overview

Arxetipo follows a structured approach to game development, comprising of foundations, systems, and components.

- **Foundation**: First abstraction layers of low-level APIs, complex codes, and third-party libraries. Examples include the Vulkan-based renderer, OpenXR-based XR plugin, and PhysX-based physics engine.

- **System**: Managers of related game logics, and can be based on foundations and other systems. 

- **Component**: Small pieces of game logics that require to be *registered* to one or more systems to function.

Together, systems and components are called **Object**.

## Others

In addition to systems and components, there are other items that cannot be categorized under any of them, such as the materials of the renderer and the physics shapes of the physics engine. While these may fall under the general definition of computer programming "objects," Arxetipo does not consider them as such.

## No Entities

Unlike typical ECS frameworks, Arxetipo does not include a specific "entity" object. 
While the concept of entities is to group components together, a dedicated object is not necessary for this. 
One can have a collection of components in any way they see fit, such as a struct, a std::vector, or even just several components surrounded by comments to indicate that they belong together. 
It is up to the developers to decide whether they want to call this an "entity."

Arxetipo does, however, provide a way to group components together called **Component Composition**. It is still not the same as an "entity." See more at the [Component Composition](../component-composition/README.md) page.

## Implementation-based

It's worth noting that the implementation of Arxetipo is convention-based, rather than inheritance-based. There are no base classes for foundations or objects. While this convention can be broken, it may be difficult to connect the code with other parts of Arxetipo. For example, components that do not follow the convention may not fit into component compositions.