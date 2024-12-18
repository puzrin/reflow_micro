/*

Usage example:

#include <string>
#include "msgpack_rpc_dispatcher.hpp"

std::string concat(std::string a, std::string b) { return a + b; }

int main() {
    MsgpackRpcDispatcher dispatcher;

    // Add methods
    dispatcher.addMethod("add", [](int a, int b) { return a + b; });
    dispatcher.addMethod("concat", concat);

    // Example usage
    std::vector<uint8_t> input;

    input = encode_to_msgp(R"({"method": "add", "args": [1, 2]})");
    std::cout << dispatcher.dispatch(input) << std::endl;

    input = encode_to_msgp(R"({"method": "concat", "args": ["hello ", "world"]})");
    std::cout << dispatcher.dispatch(input) << std::endl;

    input = encode_to_msgp(R"({"method": "unknown", "args": []})");
    std::cout << dispatcher.dispatch(input) << std::endl;

    return 0;
}

*/

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <tuple>
#include <type_traits>
#include <ArduinoJson.h>
#include <cstdint>
#include <vector>

namespace msgpack_rpc_dispatcher {

//
// Local implementation of some `std` features, for c++11 support and
// for esp32 toolchain compatibility
//

// ::index_sequence && ::make_index_sequence
template<std::size_t... Is>
struct index_sequence {};

template<std::size_t N, std::size_t... Is>
struct make_index_sequence : make_index_sequence<N-1, N-1, Is...> {};

template<std::size_t... Is>
struct make_index_sequence<0, Is...> : index_sequence<Is...> {};

// ::tuple_element_t
template <std::size_t I, typename Tuple>
struct tuple_element;

template <std::size_t I, typename Head, typename... Tail>
struct tuple_element<I, std::tuple<Head, Tail...>> : tuple_element<I-1, std::tuple<Tail...>> {};

template <typename Head, typename... Tail>
struct tuple_element<0, std::tuple<Head, Tail...>> {
    using type = Head;
};

template <std::size_t I, typename Tuple>
using tuple_element_t = typename tuple_element<I, Tuple>::type;

// ::apply
template<typename F, typename Tuple, std::size_t... Is>
auto apply_impl(F&& f, Tuple&& t, index_sequence<Is...>) -> decltype(f(std::get<Is>(std::forward<Tuple>(t))...)) {
    return f(std::get<Is>(std::forward<Tuple>(t))...);
}

template<typename F, typename Tuple>
auto apply(F&& f, Tuple&& t) -> decltype(apply_impl(std::forward<F>(f), std::forward<Tuple>(t), make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{})) {
    constexpr auto Size = std::tuple_size<typename std::decay<Tuple>::type>::value;
    return apply_impl(std::forward<F>(f), std::forward<Tuple>(t), make_index_sequence<Size>{});
}

//
// We can support only certain types for arguments and return values in
// RPC methods. This is a compile-time check for that.
//

// Define a type trait to check if a type of arguments / return are supported
template <typename T>
struct is_supported_type : std::false_type {};

#define SUPPORTED_TYPE(type) template <> struct is_supported_type<type> : std::true_type {}

SUPPORTED_TYPE(int8_t);
SUPPORTED_TYPE(uint8_t);
SUPPORTED_TYPE(int16_t);
SUPPORTED_TYPE(uint16_t);
SUPPORTED_TYPE(int32_t);
SUPPORTED_TYPE(uint32_t);
SUPPORTED_TYPE(int64_t);
SUPPORTED_TYPE(uint64_t);
SUPPORTED_TYPE(float);
SUPPORTED_TYPE(double);
SUPPORTED_TYPE(std::string);
SUPPORTED_TYPE(bool);

#undef SUPPORTED_TYPE

template <typename... Args>
struct are_all_types_supported;

template <>
struct are_all_types_supported<> : std::true_type {};

template <typename T, typename... Rest>
struct are_all_types_supported<T, Rest...>
    : std::integral_constant<bool, is_supported_type<T>::value && are_all_types_supported<Rest...>::value> {};

template <typename Tuple, std::size_t... Is>
constexpr bool check_all_types_supported_impl(index_sequence<Is...>) {
    return are_all_types_supported<tuple_element_t<Is, Tuple>...>::value;
}

template <typename Tuple>
constexpr bool check_all_types_supported() {
    return check_all_types_supported_impl<Tuple>(make_index_sequence<std::tuple_size<Tuple>::value>{});
}


template<typename ArgsTuple, std::size_t... Is>
ArgsTuple from_msgp_impl(const JsonArray& j, index_sequence<Is...>) {
    return std::make_tuple(j[Is].template as<typename std::tuple_element<Is, ArgsTuple>::type>()...);
}

template<typename ArgsTuple>
ArgsTuple from_msgp(const JsonArray& j) {
    return from_msgp_impl<ArgsTuple>(j, make_index_sequence<std::tuple_size<ArgsTuple>::value>{});
}

// Extract argument types from function
template<typename Func>
struct function_traits;

template<typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> {
    using return_type = Ret;
    using argument_types = std::tuple<Args...>;
};

template<typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>> {
    using return_type = Ret;
    using argument_types = std::tuple<Args...>;
};

// Specialization for lambdas and functors
template<typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};

// Specialization for member function pointers
template<typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret(ClassType::*)(Args...) const> {
    using return_type = Ret;
    using argument_types = std::tuple<Args...>;
};

// Helper to create response object
template<typename T>
const JsonDocument create_response(bool status, const T& result) {
    JsonDocument doc;
    doc["ok"] = status;
    doc["result"] = result;
    return doc;
}

inline void serialize_to(const JsonDocument& doc, std::string& output) {
    serializeMsgPack(doc, output);
}

inline void serialize_to(const JsonDocument& doc, std::vector<uint8_t>& output) {
    size_t size = measureMsgPack(doc);
    output.resize(size);
    serializeMsgPack(doc, output.data(), size);
}

inline DeserializationError deserialize_from(const std::string& input, JsonDocument& doc) {
    return deserializeMsgPack(doc, input);
}

inline DeserializationError deserialize_from(const std::vector<uint8_t>& input, JsonDocument& doc) {
    return deserializeMsgPack(doc, input.data(), input.size());
}

// Helper struct to check argument types
template<typename ArgsTuple, std::size_t I = 0, std::size_t N = std::tuple_size<ArgsTuple>::value>
struct TypeChecker {
    static bool check(const JsonArray& args) {
        if (!args[I].template is<typename std::tuple_element<I, ArgsTuple>::type>()) return false;
        return TypeChecker<ArgsTuple, I + 1, N>::check(args);
    }
};

template<typename ArgsTuple, std::size_t N>
struct TypeChecker<ArgsTuple, N, N> {
    static bool check(const JsonArray&) { return true; }
};

} // namespace jrcpd

class MsgpackRpcDispatcher {
public:
    MsgpackRpcDispatcher() = default;

    // For functions with arguments
    template<typename Func>
    void addMethod(const std::string& name, Func func) {
        namespace rpc_ns = msgpack_rpc_dispatcher;

        using traits = rpc_ns::function_traits<decltype(func)>;
        using Ret = typename traits::return_type;
        using ArgsTuple = typename traits::argument_types;

        static_assert(!std::is_same<Ret, void>::value, "void functions not supported");

        static_assert(rpc_ns::is_supported_type<Ret>::value, "Return type is not allowed");
        static_assert(rpc_ns::check_all_types_supported<ArgsTuple>(), "Argument type is not allowed");

        functions[name] = [func](const JsonArray& args) -> JsonDocument {
            try {
                if (args.size() != std::tuple_size<ArgsTuple>::value) {
                    throw std::runtime_error("Number of arguments mismatch");
                }

                // Check if each argument in JsonArray can hold the appropriate type from ArgsTuple
                if (!rpc_ns::TypeChecker<ArgsTuple>::check(args)) {
                    throw std::runtime_error("Argument type mismatch");
                }

                auto tpl_args = rpc_ns::from_msgp<ArgsTuple>(args);
                Ret result = rpc_ns::apply(func, tpl_args);
                return rpc_ns::create_response(true, result);
            } catch (const std::exception& e) {
                return rpc_ns::create_response(false, std::string(e.what()));
            }
        };
    }

    template<typename T>
    T dispatch(const T& input) {
        T output;
        dispatch(input, output);
        return output;
    }

    template<typename T>
    void dispatch(const T& input, T& output) {
        namespace rpc_ns = msgpack_rpc_dispatcher;

        JsonDocument doc;
        auto error = rpc_ns::deserialize_from(input, doc);
        if (error) {
            rpc_ns::serialize_to(rpc_ns::create_response(false, error.c_str()), output);
            return;
        }

        std::string method = doc["method"].as<std::string>();
        JsonArray args = doc["args"].as<JsonArray>();

        auto it = functions.find(method);
        if (it != functions.end()) {
            rpc_ns::serialize_to(it->second(args), output);
            return;
        } else {
            rpc_ns::serialize_to(rpc_ns::create_response(false, "Method not found"), output);
            return;
        }
    }

private:
    std::unordered_map<std::string, std::function<JsonDocument(const JsonArray&)>> functions;
};
