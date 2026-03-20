# Core API Reference

This document describes the primary classes exposed by the `Core` module.
All headers are included relative to the repository root.

---

## Core::EventBus

**Header:** `Core/Events/EventBus.h`

A type-safe publish/subscribe event bus. Listeners subscribe to a templated
event type and receive a const reference when the event is published. Thread
safety is the caller's responsibility; use from a single thread or guard with
a mutex.

### Key Methods

| Method | Description |
|--------|-------------|
| `Subscribe<T>(Handler)` | Register a callback for event type `T`. Returns a handle. |
| `Unsubscribe(handle)` | Remove a previously registered handler. |
| `Publish<T>(event)` | Dispatch an event to all current subscribers of type `T`. |

### Example

```cpp
#include "Core/Events/EventBus.h"

struct PlayerDied { uint32_t entityId; };

Core::EventBus bus;
auto handle = bus.Subscribe<PlayerDied>([](const PlayerDied& e) {
    std::cout << "Entity " << e.entityId << " died\n";
});

bus.Publish<PlayerDied>({42});
bus.Unsubscribe(handle);
```

---

## Core::MessageBus

**Header:** `Core/Messaging/MessageBus.h`

Decoupled string-keyed message passing. Any module can post a `Message` to a
named channel; all registered listeners on that channel receive it. Useful for
loose coupling across subsystem boundaries.

### Key Methods

| Method | Description |
|--------|-------------|
| `Post(channel, Message)` | Send a message to all listeners on `channel`. |
| `Subscribe(channel, Handler)` | Register a handler returning a subscription token. |
| `Unsubscribe(token)` | Remove a subscription. |

### Example

```cpp
#include "Core/Messaging/MessageBus.h"

Core::MessageBus bus;
auto token = bus.Subscribe("scene.loaded", [](const Core::Message& m) {
    std::cout << "Scene loaded: " << m.payload << "\n";
});

bus.Post("scene.loaded", Core::Message{"level01.json"});
```

---

## Core::TaskSystem

**Header:** `Core/TaskSystem/TaskSystem.h`

Multi-threaded task scheduler backed by a fixed-size thread pool. Tasks are
submitted as `std::function<void()>` and executed on worker threads. Supports
fire-and-forget and future-based result retrieval.

### Key Methods

| Method | Description |
|--------|-------------|
| `Submit(fn)` | Enqueue a void task; returns `std::future<void>`. |
| `Submit<T>(fn)` | Enqueue a typed task; returns `std::future<T>`. |
| `WaitAll()` | Block until all queued tasks have completed. |
| `ThreadCount()` | Returns the number of worker threads. |

### Example

```cpp
#include "Core/TaskSystem/TaskSystem.h"

Core::TaskSystem ts(4);   // 4 worker threads
auto fut = ts.Submit([] { heavyComputation(); });
fut.wait();
```

---

## Core::CommandSystem

**Header:** `Core/CommandSystem/CommandSystem.h`

Undo/redo command stack. Commands implement `Core::ICommand` with `Execute()`
and `Undo()`. The system owns the history and provides `Undo()` / `Redo()`.

### Key Methods

| Method | Description |
|--------|-------------|
| `Execute(ICommand*)` | Run and push command onto history. |
| `Undo()` | Reverse the last command. |
| `Redo()` | Re-apply the last undone command. |
| `CanUndo() / CanRedo()` | Query stack state. |
| `Clear()` | Flush entire history. |

### Example

```cpp
#include "Core/CommandSystem/CommandSystem.h"

struct MoveEntityCmd : Core::ICommand {
    void Execute() override { entity.pos += delta; }
    void Undo()    override { entity.pos -= delta; }
};

Core::CommandSystem cs;
cs.Execute(new MoveEntityCmd{entity, {1,0,0}});
cs.Undo();
```

---

## Core::ResourceManager

**Header:** `Core/ResourceManager/ResourceManager.h`

Asset loading and reference-counted caching. Assets are keyed by path string.
On first load the manager reads from disk; subsequent requests return the
cached copy. Supports synchronous and async (via TaskSystem) loading.

### Key Methods

| Method | Description |
|--------|-------------|
| `Load<T>(path)` | Load or retrieve a cached asset of type `T`. |
| `Unload(path)` | Release the cached copy (if ref count reaches zero). |
| `IsLoaded(path)` | Query cache presence. |
| `Clear()` | Flush all cached assets. |

### Example

```cpp
#include "Core/ResourceManager/ResourceManager.h"

Core::ResourceManager rm;
auto mesh = rm.Load<MeshAsset>("Assets/ship_hull.fbx");
// mesh is a shared_ptr<MeshAsset>
```

---

## Core::Serializer

**Header:** `Core/Serialization/Serializer.h`

JSON serialization helper. Provides `Serialize(object) → string` and
`Deserialize<T>(string) → T`. Uses reflection metadata when available;
falls back to manual field access for plain structs.

### Key Methods

| Method | Description |
|--------|-------------|
| `Serialize(obj)` | Convert an object to a JSON string. |
| `Deserialize<T>(json)` | Parse JSON into an object of type `T`. |
| `SerializeToFile(obj, path)` | Write JSON to a file. |
| `DeserializeFromFile<T>(path)` | Read JSON from a file into type `T`. |

### Example

```cpp
#include "Core/Serialization/Serializer.h"

struct Config { std::string name; int version; };
Core::Serializer ser;

std::string json = ser.Serialize(Config{"MyGame", 1});
auto cfg = ser.Deserialize<Config>(json);
```
