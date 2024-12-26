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
    virtual void write(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) = 0;
    virtual void read(const std::string& ns, const std::string& key, uint8_t* buffer, size_t length) = 0;
    virtual size_t length(const std::string& ns, const std::string& key) = 0;
};

namespace async_preference_ns {

// Serializer for trivially copyable types
template <typename T>
struct TrivialSerializer {
    static void save(IAsyncPreferenceKV& kv, const std::string& ns, const std::string& key, const T& value) {
        kv.write(ns, key, reinterpret_cast<uint8_t*>(const_cast<T*>(&value)), sizeof(T));
    }

    static void load(IAsyncPreferenceKV& kv, const std::string& ns, const std::string& key, T& value) {
        const size_t size = kv.length(ns, key);

        if (size == 0) return; // Key not exists => nothing to load
        if (size != sizeof(T)) return; // Wrong size => broken data, ignore it

        kv.read(ns, key, reinterpret_cast<uint8_t*>(&value), sizeof(T));
    }
};

// Serializer for buffer-like types (with data(), size(), and resize())
// Primary goal is to fit std::string and std::vector
template <typename T>
struct BufferSerializer {
    static void save(IAsyncPreferenceKV& kv, const std::string& ns, const std::string& key, const T& value) {
        kv.write(ns, key, reinterpret_cast<uint8_t*>(const_cast<typename T::value_type*>(value.data())), value.size() * sizeof(typename T::value_type));
    }

    static void load(IAsyncPreferenceKV& kv, const std::string& ns, const std::string& key, T& value) {
        size_t size = kv.length(ns, key);

        if (size == 0) return; // Key not exists => nothing to load
        if (size % sizeof(typename T::value_type) != 0) return; // Wrong size => broken data, ignore it

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

// Writer for asynchronous preferences
class AsyncPreferenceWriter {
public:
    void add(AsyncPreferenceTickable* pref) {
        pref->next_pref = head;
        head = pref;
    }

    void tick() {
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
    AsyncPreference(AsyncPreferenceWriter* writer, IAsyncPreferenceKV& kv, const std::string& ns, const std::string& key, T initial = T()) :
        databox{initial}, kv{kv}, ns(ns), key(key), is_preloaded(false), is_writer_active(false), writer{writer} {}

    T& get() {
        preload();
        return databox.value;
    }

    void set(const T& value) {
        // Don't write the same data
        if (is_preloaded && databox.value == value) return;

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
        if (!is_preloaded) is_preloaded = true;

        // Lazily register in writer. This is done here, to allow use stitic classes,
        // where initialization order is not defined.
        if (!is_writer_active && writer) {
            is_writer_active = true;
            writer->add(this);
        }

        databox.beginWrite();
    }
    void valueUpdateEnd() { databox.endWrite(); }

    //
    // Those are for calling by AsyncPreferenceWriter from another thread,
    // to avoid freezes.
    //
    void tick() override {
        using namespace async_preference_ns;

        if (!databox.makeSnapshot()) return;

        // Save snapshot to storage
        if constexpr (!std::is_void_v<Serializer>) {
            // If custom serializer is provided, use it
            Serializer::save(kv, ns, key, databox.snapshot);
        } else if constexpr (HasBufferTraits<T>::value) {
            // Use BufferSerializer if type has buffer-like traits
            BufferSerializer<T>::save(kv, ns, key, databox.snapshot);
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            // Use TrivialSerializer for trivially copyable types
            TrivialSerializer<T>::save(kv, ns, key, databox.snapshot);
        } else {
            static_assert(dependent_false<T>::value, "No suitable serializer found for this type");
        }
    }

private:
    DataGuard<T> databox;
    IAsyncPreferenceKV& kv;
    std::string ns;
    std::string key;
    bool is_preloaded;
    bool is_writer_active;
    AsyncPreferenceWriter* writer;

    // Fetch value from storage, if key exists. This is called only once in
    // life cycle. The next reads are always from memory only.
    void preload() {
        using namespace async_preference_ns;

        if (is_preloaded) return;

        if (kv.length(ns, key) == 0) return; // If key does not exist

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
    AsyncPreferenceMap(AsyncPreferenceWriter* writer, IAsyncPreferenceKV& kv,
                        const std::string& ns, const std::string& key,
                        T default_value = T())
        : writer(writer)
        , kv(kv)
        , ns(ns)
        , key(key)
        , default_value(default_value)
    {}

    class PreferenceProxy {
    public:
        PreferenceProxy(AsyncPreference<T>* pref) : pref(pref) {}

        operator T&() const = delete;
        operator const T&() { return pref->get(); }

        PreferenceProxy& operator=(const T& value) { pref->set(value); return *this; }

        bool operator==(const T& other) const { return pref->get() == other; }
        friend bool operator==(const T& value, const PreferenceProxy& proxy) { return proxy == value; }

    private:
        AsyncPreference<T>* pref;
    };

    PreferenceProxy operator[](size_t idx) {
        auto it = items.find(idx);
        if (it == items.end()) {
            auto pref = new AsyncPreference<T>(
                writer, kv, ns,
                key + "_" + std::to_string(idx),
                default_value
            );
            items[idx] = pref;
            return PreferenceProxy(pref);
        }
        return PreferenceProxy(it->second);
    }

private:
    std::map<size_t, AsyncPreference<T>*> items;
    AsyncPreferenceWriter* writer;
    IAsyncPreferenceKV& kv;
    std::string ns;
    std::string key;
    T default_value;
};
