#pragma once

#include <Angaraka/Base.hpp>
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>


// --- Helper Macro for Event Declaration ---
// This macro helps in boilerplate reduction for concrete event classes.
#ifndef EVENT_CLASS_TYPE
#define EVENT_CLASS_TYPE(type)                                                       \
inline static size_t GetStaticType_s() { return Angaraka::Events::GetEventTypeId<type>(); } \
inline size_t GetStaticType() const override { return GetStaticType_s(); }                  \
inline const char* GetName() const override { return #type; }
#endif

#ifndef EVENT_CLASS_CATEGORY
#define EVENT_CLASS_CATEGORY(category) inline int GetCategoryFlags() const override { return static_cast<int>(category); }
#endif
