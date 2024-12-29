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
#include <variant>
#include <unordered_map>
#include <functional>
#include <tuple>
#include <type_traits>
#include <ArduinoJson.h>
#include <cstdint>
#include <vector>

namespace msgpack_rpc_dispatcher {

//
// We can support only certain types for arguments and return values in
// RPC methods. This is a compile-time check for that.
//

using SupportedTypes = std::variant<
    int8_t, uint8_t,
    int16_t, uint16_t,
    int32_t, uint32_t,
    int64_t, uint64_t,
    float, double,
    std::string,
    bool,
    std::vector<uint8_t>
>;

template<typename T>
constexpr bool is_supported_type = std::is_constructible_v<SupportedTypes, T>;

template <typename Tuple>
constexpr bool check_tuple_types = []<std::size_t... Is>(std::index_sequence<Is...>) {
    return (is_supported_type<std::tuple_element_t<Is, Tuple>> && ...);
}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});


template<typename T>
auto convert_from_msgp(const JsonVariant& value) -> T {
    if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
        auto binary = value.as<MsgPackBinary>();
        return std::vector<uint8_t>(
            static_cast<const uint8_t*>(binary.data()),
            static_cast<const uint8_t*>(binary.data()) + binary.size()
        );
    } else {
        return value.as<T>();
    }
}

template<typename ArgsTuple, std::size_t... Is>
auto from_msgp_impl(const JsonArray& j, std::index_sequence<Is...>) -> ArgsTuple {
    return std::make_tuple(convert_from_msgp<std::tuple_element_t<Is, ArgsTuple>>(j[Is])...);
}

template<typename ArgsTuple>
auto from_msgp(const JsonArray& j) -> ArgsTuple {
    return from_msgp_impl<ArgsTuple>(j, std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
}

// Extract argument types from function
template<typename Func>
struct function_traits : function_traits<decltype(&Func::operator())> {};

// Regular functions
template<typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> {
    using return_type = Ret;
    using argument_types = std::tuple<Args...>;
};

// Lambdas and functors
template<typename ClassType, typename Ret, typename... Args>
struct function_traits<Ret(ClassType::*)(Args...) const> {
    using return_type = Ret;
    using argument_types = std::tuple<Args...>;
};

// Helper to create response object
template<typename T>
auto create_response(bool status, const T& result) -> const JsonDocument {
    JsonDocument doc;
    doc["ok"] = status;

    if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
        doc["result"] = MsgPackBinary(result.data(), result.size());
    } else {
        doc["result"] = result;
    }

    return doc;
}

inline void serialize_to(const JsonDocument& doc, std::vector<uint8_t>& output) {
    size_t size = measureMsgPack(doc);
    output.resize(size);
    serializeMsgPack(doc, output.data(), size);
}

inline auto deserialize_from(const std::vector<uint8_t>& input, JsonDocument& doc) -> DeserializationError {
    return deserializeMsgPack(doc, input.data(), input.size());
}

template<typename T>
auto check_type(const JsonVariant& value) -> bool {
    if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
        return value.is<MsgPackBinary>();
    } else {
        return value.is<T>();
    }
}

template<typename ArgsTuple>
auto runtime_type_check(const JsonArray& args) -> bool {
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return ((check_type<std::tuple_element_t<Is, ArgsTuple>>(args[Is])) && ...);
    }(std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
}

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

        static_assert(!std::is_same_v<Ret, void>, "void functions not supported");

        static_assert(rpc_ns::is_supported_type<Ret>, "Return type is not allowed");
        static_assert(rpc_ns::check_tuple_types<ArgsTuple>, "Argument type is not allowed");

        functions[name] = [func](const JsonArray& args) -> JsonDocument {
            try {
                if (args.size() != std::tuple_size_v<ArgsTuple>) {
                    throw std::runtime_error("Number of arguments mismatch");
                }

                // Check if each argument in JsonArray can hold the appropriate type from ArgsTuple
                if (!rpc_ns::runtime_type_check<ArgsTuple>(args)) {
                    throw std::runtime_error("Argument type mismatch");
                }

                auto tpl_args = rpc_ns::from_msgp<ArgsTuple>(args);
                Ret result = std::apply(func, tpl_args);
                return rpc_ns::create_response(true, result);
            } catch (const std::exception& e) {
                return rpc_ns::create_response(false, std::string(e.what()));
            }
        };
    }

    void dispatch(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
        namespace rpc_ns = msgpack_rpc_dispatcher;

        JsonDocument doc;
        auto error = rpc_ns::deserialize_from(input, doc);
        if (error) {
            rpc_ns::serialize_to(rpc_ns::create_response(false, error.c_str()), output);
            return;
        }

        std::string method = doc["method"].as<std::string>();
        JsonArray args = doc["args"].as<JsonArray>();

        if (auto it = functions.find(method); it != functions.end()) {
            rpc_ns::serialize_to(it->second(args), output);
        } else {
            rpc_ns::serialize_to(rpc_ns::create_response(false, "Method not found"), output);
        }
    }

private:
    std::unordered_map<std::string, std::function<JsonDocument(const JsonArray&)>> functions{};
};
