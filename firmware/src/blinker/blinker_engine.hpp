#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <algorithm>
#include <type_traits>
#include <atomic>

template<int Channels>
class IBlinkerLED {
public:
    static constexpr int ChannelsCount = Channels;

    using DataType = typename std::array<uint8_t, Channels>;

    virtual void set(const DataType& value) = 0;
};


template <typename T>
class BlinkerSimpleQueue {
private:
    T buffer;
    std::atomic<uint32_t> versionCounter;
    std::atomic<bool> writerActive;
    uint32_t lastReadVersion;

public:
    BlinkerSimpleQueue() : versionCounter(0), writerActive(false), lastReadVersion(0) {}

    bool write(const T& value) {
        // Disable parallel writes
        bool expected = false;
        if (!writerActive.compare_exchange_strong(expected, true)) return false;

        versionCounter.fetch_add(1, std::memory_order_acquire); // Odd => write in progress
        buffer = value;
        versionCounter.fetch_add(1, std::memory_order_release); // Even => write completed

        writerActive.store(false, std::memory_order_release);
        return true;
    }

    bool read(T& output) {
        uint32_t versionBefore = versionCounter.load(std::memory_order_acquire);

        if (versionBefore == lastReadVersion) return false; // No new data

        if (versionBefore % 2 != 0) return false; // Override in progress

        T tempBuffer = buffer;

        // Re-read version to make sure data not changed
        uint32_t versionAfter = versionCounter.load(std::memory_order_acquire);
        if (versionAfter != versionBefore) return false;

        output = tempBuffer;
        lastReadVersion = versionBefore;
        return true;
    }
};

template<typename Driver>
class BlinkerEngine {
public:
    struct Action {
        typename Driver::DataType value;
        uint32_t period;
        bool isAnimated;

        Action() : value{}, period(0), isAnimated(false) {}

        Action(typename Driver::DataType value, uint32_t period, bool isAnimated = false)
            : value(value), period(period), isAnimated(isAnimated) {}

        // Sugar for single channel, to omit brackets
        template<int Channels = Driver::ChannelsCount, typename = std::enable_if_t<Channels == 1>>
        Action(uint8_t singleValue, uint32_t period, bool isAnimated = false)
            : value{std::array<uint8_t, 1>{singleValue}}, period(period), isAnimated(isAnimated) {}
    };

    BlinkerEngine() : driver(), sequenceQueue{}, backgroundQueue{}, prevTickTs(0), hasNewJob(false), working(false),
        sequence{}, backgroundValue{}, currentActionIdx(0), actionProgress(0), prevActionValue{} {}

    void loop(const std::initializer_list<Action>& actions) { updateSequence(actions, true); }

    void once(const std::initializer_list<Action>& actions) { updateSequence(actions, false); }

    void background(const typename Driver::DataType& value) {
        const typename Driver::DataType val = std::move(value);
        backgroundQueue.write(val);
    }
    // Sugar for single channel, to omit brackets
    template<int Channels = Driver::ChannelsCount, typename = std::enable_if_t<Channels == 1>>
    void background(const uint8_t value) {
        typename Driver::DataType val = {value};
        backgroundQueue.write(val);
    }

    void off() { once({ {backgroundValue, 0} }); }

    static Action flowTo(const typename Driver::DataType target, uint32_t duration) { return {target, duration, true}; }
    // Sugar for single channel, to omit brackets
    template<int Channels = Driver::ChannelsCount, typename = std::enable_if_t<Channels == 1>>
    static Action flowTo(uint8_t target, uint32_t duration) { return {target, duration, true}; }

    static inline const Action OFF = { typename Driver::DataType{}, 0 };

    void tick(uint32_t msTimestamp) {
        if (prevTickTs == 0) { prevTickTs = msTimestamp; return; }

        uint32_t elapsed = msTimestamp - prevTickTs;
        prevTickTs = msTimestamp;

        if (sequenceQueue.read(sequence)) hasNewJob = true;

        if (hasNewJob) {
            currentActionIdx = 0;
            actionProgress = 0;
            working = true;
            hasNewJob = false;
        }

        if (backgroundQueue.read(backgroundValue) && !working) driver.set(backgroundValue);

        if (working) {
            const auto& action = sequence.actions[currentActionIdx];
            actionProgress = std::min(actionProgress + elapsed, action.period);

            // Calculate & set led value
            if (action.isAnimated) {
                typename Driver::DataType value;
                for (int i = 0; i < Driver::ChannelsCount; i++) {
                    int32_t from = prevActionValue[i];
                    int32_t to = action.value[i];
                    int32_t val = from + (to - from) * int32_t(actionProgress) / int32_t(action.period);
                    value[i] = val < 0 ? 0 : val > 255 ? 255 : val;
                }
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
                    if (sequence.looping) hasNewJob = true;
                    else driver.set(backgroundValue);
                }
            }
        }
    }

private:
    struct Sequence {
        std::array<Action, 20> actions;
        size_t length;
        bool looping;
    };

    void updateSequence(const std::initializer_list<Action>& actionsList, bool looping) {
        Sequence sequence;
        sequence.looping = looping;
        std::copy(actionsList.begin(), actionsList.end(), sequence.actions.begin());
        sequence.length = actionsList.size();

        sequenceQueue.write(sequence);
    }

    Driver driver;
    BlinkerSimpleQueue<Sequence> sequenceQueue;
    BlinkerSimpleQueue<typename Driver::DataType> backgroundQueue;

    // Ticker states
    uint32_t prevTickTs;
    bool hasNewJob;
    bool working;
    Sequence sequence;
    typename Driver::DataType backgroundValue;
    uint8_t currentActionIdx;
    uint32_t actionProgress;
    typename Driver::DataType prevActionValue;
};
