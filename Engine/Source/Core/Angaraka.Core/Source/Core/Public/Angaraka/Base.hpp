#pragma once
#ifndef ANGARAKA_ENGINE_BASE_HPP
#define ANGARAKA_ENGINE_BASE_HPP

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <windowsx.h>

#pragma warning(disable:4530)
#endif

#ifdef _DEBUG
#define DEBUG_OP(x) x
#else
#define DEBUG_OP(x)
#endif

#ifndef VIRTUAL
#define VIRTUAL
#endif

#ifndef ANGARAKA_CONFIGURATION_FOLDER
#define ANGARAKA_CONFIGURATION_FOLDER "config"
#endif


#define DISABLE_COPY(T)                     \
            explicit T(const T&) = delete;  \
            T& operator=(const T&) = delete;

#define DISABLE_MOVE(T)                 \
            explicit T(T&&) = delete;   \
            T& operator=(T&&) = delete;

#define DISABLE_COPY_AND_MOVE(T) DISABLE_COPY(T) DISABLE_MOVE(T)

#undef min
#undef max

namespace Angaraka {
    using U8 = uint8_t;
    using U16 = uint16_t;
    using U32 = uint32_t;
    using U64 = uint64_t;

    using I8 = int8_t;
    using I16 = int16_t;
    using I32 = int32_t;
    using I64 = int64_t;

    using F32 = float;
    using F64 = double;

    using VoidPtr = void*;
    using String = std::string;
    using WString = std::wstring;

    inline String WStringToUTF8(const WString& wstr) {
        if (wstr.empty()) return {};

        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (size <= 0) return {};

        String result(size - 1, 0); // omit null terminator
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
        return result;
    }

    inline WString UTF8ToWString(const String& str) {
        if (str.empty()) return {};

        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (size <= 0) return {};

        WString wstr(size - 1, 0); // exclude null terminator
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
        return wstr;
    }

    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T>
    using Reference = std::shared_ptr<T>;

    template<typename T, typename... Args>
    constexpr Scope<T> CreateScope(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    constexpr Reference<T> CreateReference(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template <typename S, typename T, typename ... Args>
    constexpr Reference<S> CreateReference(Args&&... args) {
        return std::dynamic_pointer_cast<S>(std::make_shared<T>(std::forward<Args>(args)...));
    }

    template <typename T>
    std::function<void()> SafeBind(std::weak_ptr<T> weak, void (T::* method)()) {
        return [weak, method]() {
            if (auto shared = weak.lock()) {
                (shared.get()->*method)();
            }
        };
    }

    template <typename T, typename Method, typename... Args>
    auto SafeBind(std::weak_ptr<T> weak, Method method) {
        return [weak, method](Args... args) {
            if (auto shared = weak.lock()) {
                (shared.get()->*method)(args...);
            }
        };
    }

    template <typename T, typename F>
    std::function<void()> SafeBindLambda(std::weak_ptr<T> weak, F&& func) {
        return [weak, func = std::forward<F>(func)]() {
            if (auto shared = weak.lock()) {
                func();
            }
        };
    }
}



/**
 * @brief Helper macro to define the static TypeId for concrete Resource types.
 *
 * This simplifies getting a unique ID for each resource type at compile time.
 * Use like: AGK_RESOURCE_TYPE_ID(MyTextureResource);
 */
#define AGK_RESOURCE_TYPE_ID(type) \
        static size_t StaticTypeId() { \
            static const size_t id = typeid(type).hash_code(); \
            return id; \
        } \
        size_t GetTypeId() const override { return StaticTypeId(); }

#if _DEBUG
#ifndef AGK_ASSERT
#define AGK_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("Assertion failed: " + std::string(message)); \
        } \
    } while (false)
#endif
#else
#ifndef AGK_ASSERT
#define AGK_ASSERT
#endif
#endif // _DEBUG
#include "Angaraka/Log.hpp"

#endif // ANGARAKA_ENGINE_BASE_HPP