#pragma once

#include <stdint.h>
#include <etl/algorithm.h>
#include <etl/array.h>
#include <etl/atomic.h>
#include <etl/type_traits.h>

#include "data_guard.hpp"

template<int Channels>
class IBlinkerLED {
public:
    static constexpr int ChannelsCount = Channels;

    using DataType = typename etl::array<uint8_t, Channels>;

    virtual void set(const DataType& value) = 0;
};


template<typename Driver>
class BlinkerEngine {
public:
    using DataType = typename Driver::DataType;
    struct Action {
        DataType value;
        uint32_t period;
        bool isAnimated;

        Action() : value{}, period(0), isAnimated(false) {}

        Action(DataType _value, uint32_t _period, bool _isAnimated = false)
            : value{_value}, period{_period}, isAnimated{_isAnimated} {}

        // Sugar for single channel, to omit brackets
        template<int Channels = Driver::ChannelsCount, typename = etl::enable_if_t<Channels == 1>>
        Action(uint8_t _singleValue, uint32_t _period, bool _isAnimated = false)
            : value{}, period{_period}, isAnimated{_isAnimated} {
            value[0] = _singleValue;
        }
    };

    template<size_t N>
    void loop(const Action (&actions)[N]) { updateSequence(actions, actions + N, true); }

    template<typename Iterable>
    void loop(const Iterable& actions) { updateSequence(actions.begin(), actions.end(), true); }

    template<size_t N>
    void once(const Action (&actions)[N]) { updateSequence(actions, actions + N, false); }

    template<typename Iterable>
    void once(const Iterable& actions) { updateSequence(actions.begin(), actions.end(), false); }

    void background(const DataType& value) {
        backgroundGuard.writeData(value);
    }
    // Sugar for single channel, to omit brackets
    template<int Channels = Driver::ChannelsCount, typename = etl::enable_if_t<Channels == 1>>
    void background(const uint8_t value) {
        DataType val{};
        val[0] = value;
        backgroundGuard.writeData(val);
    }

    void off() { once({ {backgroundValue, 0} }); }

    static auto flowTo(const DataType target, uint32_t duration) -> Action { return {target, duration, true}; }
    // Sugar for single channel, to omit brackets
    template<int Channels = Driver::ChannelsCount, typename = etl::enable_if_t<Channels == 1>>
    static auto flowTo(uint8_t target, uint32_t duration) -> Action { return {target, duration, true}; }

    static inline const Action OFF = { DataType{}, 0 };

    template<typename T>
    auto interpolate(T p1_at, const DataType& p1_value, T p2_at, const DataType& p2_value, T at) const -> DataType {
        DataType result{};
        for (int i = 0; i < Driver::ChannelsCount; i++) {
            int32_t from = p1_value[i];
            int32_t to = p2_value[i];
            int32_t val = from + (to - from) * int32_t(at - p1_at) / int32_t(p2_at - p1_at);
            result[i] = etl::clamp<uint8_t>(val, 0, 255);
        }
        return result;
    }

    void tick(uint32_t msTimestamp) {
        if (prevTickTs == 0) { prevTickTs = msTimestamp; return; }

        uint32_t elapsed = msTimestamp - prevTickTs;
        prevTickTs = msTimestamp;

        if (sequenceGuard.makeSnapshot()) {
            sequence = sequenceGuard.snapshot;
            hasNewJob = true;
        }

        if (hasNewJob) {
            currentActionIdx = 0;
            actionProgress = 0;
            working = true;
            hasNewJob = false;
        }

        if (backgroundGuard.makeSnapshot()) {
            backgroundValue = backgroundGuard.snapshot;
            if (!working) { driver.set(backgroundValue); }
        }

        if (working) {
            const auto& action = sequence.actions[currentActionIdx];
            actionProgress = etl::min(actionProgress + elapsed, action.period);

            // Calculate & set led value
            if (action.isAnimated) {
                auto value = interpolate<uint32_t>(0, prevActionValue, action.period, action.value, actionProgress);
                driver.set(value);
            } else {
                driver.set(action.value);
            }

            // If action end reached, prepare next step
            if (actionProgress >= action.period) {
                prevActionValue = action.value;
                currentActionIdx++;
                actionProgress = 0;

                // If sequence end reached...
                if (currentActionIdx >= sequence.length) {
                    working = false;
                    // If looping, start over
                    if (sequence.looping) { hasNewJob = true; }
                    else { driver.set(backgroundValue); }
                }
            }
        }
    }

private:
    struct Sequence {
        static constexpr size_t MaxActions = 20;
        etl::array<Action, MaxActions> actions;
        size_t length{0};
        bool looping{false};
    };

    template<typename Iterator>
    void updateSequence(Iterator begin, Iterator end, bool looping) {
        Sequence seq{};
        seq.looping = looping;

        size_t index = 0;
        for (Iterator it = begin; it != end && index < Sequence::MaxActions; ++it) {
            seq.actions[index] = *it;
            ++index;
        }

        seq.length = index;
        sequenceGuard.writeData(seq);
    }

    Driver driver{};
    DataGuard<Sequence> sequenceGuard{};
    DataGuard<DataType> backgroundGuard{};

    // Ticker states
    uint32_t prevTickTs{0};
    bool hasNewJob{false};
    bool working{false};
    Sequence sequence{};
    DataType backgroundValue{};
    uint8_t currentActionIdx{0};
    uint32_t actionProgress{0};
    DataType prevActionValue{};
};
