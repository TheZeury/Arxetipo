# Foundations

**Foundation** are the first abstraction layers of low-level APIs, complex codes, and third-party libraries.

## Suggested Coventions

There are no generic requirements for a foundation since the way they work varies, as they are based on different low-level and third-party APIs. 
However, there are some conventions that are suggested to be followed:

### 1. Providing Common Interfaces

This is mainly for modularity to ensure the possibility to replace a foundation with another implementation without or only with minimal changes in the other codes.

For example, a renderer should provide a common interface to create models, render frames, and so on. 
By providing the same interface, you can write your own renderer and replace the default one with it.

### 2. Using Proxies to Resolve Dependencies

Foundations can have dependencies, such as renderers and xr plugins always depending on each other. 
By extracting the communication between them to a proxy and providing interfaces with that proxy, one foundation can be replaced without changing the other one.

## Engine implemented Foundations

Arxeitpo provides some common used foundations. 
They are sufficient for most use cases, so you typically don't need to write your own.

| Name | Description |
| --- | --- |
| [VulkanRenderer]() | A Vulkan based renderer. |
| [OpenXRPlugin]() | An OpenXR based XR plugin. |
| [PhysXEngine]() | A PhysX based physics engine. |
| [CommandRuntime]() | A command runtime excuting commands written in "Arxemando"|