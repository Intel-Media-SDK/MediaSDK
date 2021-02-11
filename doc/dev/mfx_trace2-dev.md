# MediaSDK tracing (mfx_trace2). Developer documentation

## Overview

See [mfx_trace2 user documentation](./mfx_trace2.md).

Main trace system object is presented by global instance of `mfx::Trace` class (instance is called `_mfx_trace`).
Its constructor (`mfx::Trace::Trace()`) reads configuration file (on Linux) or registry values (on Windows), initializes all enabled backends.

mfx::Trace class interface:

```cpp
namespace mfx {

class Trace {
public:
    Trace();
    void pushEvent(const Trace::Event &e);

    struct Event { <variables> };
    class Scope {
    public:
        Scope(SourceLocation sl, const char* name, mfxU8 level = GENERIC);
        Scope(SourceLocation sl, const char* name, const char *category, mfxU8 level = GENERIC);
        void event(SourceLocation sl, const char *key, Args... args)
    }
};

}
```

User is not supposed to touch internals of `mfx::Trace` class except `mfx::Trace::Scope` which is public interface for user to enable scoped tracing. `mfx::Trace::pushEvent(const Trace::Event &e)` function stores event in std::vector `_mfx_trace.events` and calls `handleEvent(const Trace::Event &e)` function for every enabled backend.

Event saves the following information:
- source_location (`mfx::source_location` class)
  It contains file name, line and function name where event was called.
- name
- category (it can be specified by users in `mfx::Trace::Scope` constructor)
- event type
  - BEGIN (scope constructor)
  - END (scope destructor)
  - INFO (`event` method call of `mfx::Trace::Scope` is called)
  - other (other types are reserved for creating custom backend-specific events, for example, OTHER1 and OTHER2 are used in Chrome backend for creating link events))
- id (it is the same among one BEGIN-INFO-END group of events)
- parentIndex (in events of type BEGIN it is always zero, if it is INFO or END type of event then it contains index of corresponding BEGIN event in `_mfx_trace.events` array)
- timestamp (in microseconds)
- threadId
- description
- map of key-value pairs of information that is added using `mfx::Trace::Scope::event(key, value)` method

## Backend structure

Backend class should be created in namespace `mfx`, inherited from `mfx::TraceBackend` and implement:
- `void handleEvent(const mfx::Trace::Event &e)`
- virtual destructor

Backend example:

```cpp
namespace mfx
{

class TextLog : public TraceBackend
{
    ...
public:
    TextLog(const char *filename);
    void handleEvent(const Trace::Event &e) override;
    ~TextLog() override;
};

}
```

- Constructor usually contains some necessary initialization procedures for backend.
- `handleEvent` function lets backend properly handle event on the run, immediately (without postponing handling to the future)
- Destructor usually contains some necessary deinitialization procedures for backend and it may also handle some events. It can be useful if you do not want to slow down the execution by handling events immediately.

Backend can initialized in `mfx::Trace::Trace()` by adding to `backends` vector. Example:

```cpp
#if defined(USE_MFX_TRACE2) && defined(MFX_TRACE_ENABLE_TEXTLOG)
    backends.push_back(std::unique_ptr<TraceBackend>(new TextLog(options["TextLog"].c_str())));
#endif
```

## Helper functions

There are functions that help to format your data properly for passing them to `mfx::Trace::Scope::event` method.

```cpp
static std::string hex(mfxU64 value); // converts integer value to string in hexadecimal representation
```

All functions are defined in namespace `mfx`.

## TODO

- Replace custom source_location implementation (`mfx::source_location`) with standard one after C++20: https://en.cppreference.com/w/cpp/utility/source_location
