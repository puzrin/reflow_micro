#pragma once

#include <cstdint>
#include <string>
#include <atomic>
#include <vector>
#include <map>
#include <type_traits>
#include "data_guard.hpp"

// Interface for key-value storage
class IAsyncPreferenceKV {
public:
    virtual auto write(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) -> bool = 0;
    virtual auto read(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) -> bool = 0;
    virtual auto length(const std::string& ns, const std::string& key) -> size_t = 0;
};

namespace async_preference_ns {

// Serializer for trivially copyable types
template <typename T>
struct TrivialSerializer {
    static auto save(IAsyncPreferenceKV& kv, const std::string& ns, const std::string& key, const T& value) -> bool {
        return kv.write(ns, key, reinterpret_cast<uint8_t*>(const_cast<T*>(&value)), sizeof(T));
    }

    static void load(IAsyncPreferenceKV& kv, const std::string& ns, const std::string& key, T& value) {
        const size_t size = kv.length(ns, key);

        if (size == 0) { return; } // Key not exists => nothing to load
        if (size != sizeof(T)) { return; } // Wrong size => broken data, ignore it

        kv.read(ns, key, reinterpret_cast<uint8_t*>(&value), sizeof(T));
    }
};

// Serializer for buffer-like types (with data(), size(), and resize())
// Primary goal is to fit std::string and std::vector
template <typename T>
struct BufferSerializer {
    static auto save(IAsyncPreferenceKV& kv, const std::string& ns, const std::string& key, const T& value) -> bool {
        return kv.write(ns, key, reinterpret_cast<uint8_t*>(const_cast<typename T::value_type*>(value.data())), value.size() * sizeof(typename T::value_type));
    }

    static void load(IAsyncPreferenceKV& kv, const std::string& ns, const std::string& key, T& value) {
        size_t size = kv.length(ns, key);

        if (size == 0) { return; } // Key not exists => nothing to load
        if (size % sizeof(typename T::value_type) != 0) { return; } // Wrong size => broken data, ignore it

        value.resize(size / sizeof(typename T::value_type));
        kv.read(ns, key, reinterpret_cast<uint8_t*>(value.data()), size);
    }
};

// Check if the type has data(), size(), and resize() methods
template <typename T, typename = void>
struct HasBufferTraits : std::false_type {};

template <typename T>
struct HasBufferTraits<T, std::void_t<decltype(std::declval<T>().data()),
                                      decltype(std::declval<T>().size()),
                                      decltype(std::declval<T>().resize(0))>> : std::true_type {};

// Helper template to trigger static_assert with a dependent type
template <typename T>
struct dependent_false : std::false_type {};

}

// Internal interface for AsyncPreference
class AsyncPreferenceTickable {
public:
    virtual void tick() = 0;
    AsyncPreferenceTickable* next_pref = nullptr;
};

class IAsyncPreferenceWriter {
public:
    virtual void add(AsyncPreferenceTickable* pref) = 0;
    virtual void tick() = 0;
};

// Writer for asynchronous preferences
class AsyncPreferenceWriter : public IAsyncPreferenceWriter {
public:
    void add(AsyncPreferenceTickable* pref) override {
        pref->next_pref = head;
        head = pref;
    }

    void tick() override {
        auto ptr = head;
        while (ptr) {
            ptr->tick();
            ptr = ptr->next_pref;
        }
    }

private:
    AsyncPreferenceTickable* head = nullptr;
};

template <typename T, typename Serializer = void>
class AsyncPreference : public AsyncPreferenceTickable {
public:
    AsyncPreference(IAsyncPreferenceWriter& writer, IAsyncPreferenceKV& kv, const std::string& ns, const std::string& key, const T& initial = T()) :
        databox{initial}, kv{kv}, ns{ns}, key{key}, writer{writer}
    {
        writer.add(this);

        // Force immediate preload on create.
        // This will protect from collisions with writer, because such instances
        // are created on program init, before writer starts.
        preload();
    }

    auto get() -> T& {
        preload();
        return databox.value;
    }

    void set(const T& value) {
        // Don't write the same data
        if (is_preloaded && databox.value == value) { return; }

        valueUpdateBegin();
        databox.value = value;
        valueUpdateEnd();
    }

    // Those are public for case, when user wish to modify complex object
    // internals instead of .set() method. Not recommended for direct use.
    void valueUpdateBegin() {
        // This should not usually happen, because user call .get() at the start
        // to restore persistance. But if write is called first, we should
        // disable persistance restore.
        is_preloaded = true;

        databox.beginWrite();
    }
    void valueUpdateEnd() { databox.endWrite(); }

    //
    // Those are for calling by AsyncPreferenceWriter from another thread,
    // to avoid freezes.
    //
    void tick() override {
        using namespace async_preference_ns;

        // Forcing preload in constructor helps to avoid writer collisions.
        // But in theory, there can be different library, writing to nvs from
        // another thread. For this case we check write status and flag repeat
        // on fail.

        // IMPORTANT: don't make new snapshots on pending write. This can create
        // garbage on fail. Instead, write old data first, and postpone snapshot
        // to next call. A bit dirty, but simple and consistent.
        if (has_pending_write || databox.makeSnapshot()) {

            // Save snapshot to storage

            bool succeeded = false;

            if constexpr (!std::is_void_v<Serializer>) {
                // If custom serializer is provided, use it
                succeeded = Serializer::save(kv, ns, key, databox.snapshot);
            } else if constexpr (HasBufferTraits<T>::value) {
                // Use BufferSerializer if type has buffer-like traits
                succeeded = BufferSerializer<T>::save(kv, ns, key, databox.snapshot);
            } else if constexpr (std::is_trivially_copyable_v<T>) {
                // Use TrivialSerializer for trivially copyable types
                succeeded = TrivialSerializer<T>::save(kv, ns, key, databox.snapshot);
            } else {
                static_assert(dependent_false<T>::value, "No suitable serializer found for this type");
            }

            if (!succeeded) { has_pending_write = true; }
        }
    }

private:
    DataGuard<T> databox;
    IAsyncPreferenceKV& kv;
    std::string ns;
    std::string key;
    IAsyncPreferenceWriter& writer;
    bool is_preloaded{false};
    bool has_pending_write{false};

    // Fetch value from storage, if key exists. This is called only once in
    // life cycle. The next reads are always from memory only.
    void preload() {
        using namespace async_preference_ns;

        if (is_preloaded) { return; }

        if (kv.length(ns, key) == 0) { return; } // If key does not exist

        if constexpr (!std::is_void_v<Serializer>) {
            // Use custom serializer if provided
            Serializer::load(kv, ns, key, databox.value);
        } else if constexpr (HasBufferTraits<T>::value) {
            // Use BufferSerializer if type has buffer-like traits
            BufferSerializer<T>::load(kv, ns, key, databox.value);
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            // Use TrivialSerializer for trivially copyable types
            TrivialSerializer<T>::load(kv, ns, key, databox.value);
        } else {
            static_assert(dependent_false<T>::value, "No suitable serializer found for this type");
        }

        is_preloaded = true;
    }
};

template<typename T>
class AsyncPreferenceMap {
public:
    AsyncPreferenceMap(IAsyncPreferenceWriter& writer, IAsyncPreferenceKV& kv,
                        const std::string& ns, const std::string& key,
                        const T& default_value = T())
        : writer{writer}
        , kv{kv}
        , ns{ns}
        , key{key}
        , default_value{default_value}
    {}

   AsyncPreference<T>& operator[](size_t idx) {
       auto it = items.find(idx);
       if (it == items.end()) {
           auto pref = new AsyncPreference<T>(
               writer, kv, ns,
               key + "_" + std::to_string(idx),
               default_value
           );
           items[idx] = pref;
           return *pref;
       }
       return *(it->second);
   }

private:
    std::map<size_t, AsyncPreference<T>*> items{};
    IAsyncPreferenceWriter& writer;
    IAsyncPreferenceKV& kv;
    std::string ns;
    std::string key;
    T default_value;
};
