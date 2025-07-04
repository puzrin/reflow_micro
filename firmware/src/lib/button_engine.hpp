#pragma once

#include <cstdint>
#include <functional>

namespace ButtonConstants {

static constexpr uint32_t JITTER_THRESHOLD = 50;
static constexpr uint32_t SHORT_PRESS_THRESHOLD = 500;
static constexpr uint32_t LONG_PRESS_THRESHOLD = 2000;

} // namespace ButtonConstants

class IButtonDriver {
    virtual auto get() -> bool = 0;
};

enum class ButtonEventId {
    //BUTTON_SEQUENCE_START,
    //BUTTON_SEQUENCE_END,
    BUTTON_PRESS_START,
    BUTTON_LONG_PRESS_START,
    BUTTON_LONG_PRESS_FAIL,
    BUTTON_LONG_PRESS,
    BUTTON_PRESSED_1X,
    BUTTON_PRESSED_2X,
    BUTTON_PRESSED_3X,
    BUTTON_PRESSED_4X,
    BUTTON_PRESSED_5X
};

template <typename Driver>
class ButtonEngine {
public:
    template <typename Handler>
    void setEventHandler(Handler&& handler) {
        eventHandler = std::forward<Handler>(handler);
    }

    void tick(uint32_t ms_timestamp) {
        using namespace ButtonConstants;

        // Sync initial timestamps
        if (unfilteredBtnTimestamp == 0 || btnToggleTimestamp == 0) {
            unfilteredBtnTimestamp = ms_timestamp;
            btnToggleTimestamp = ms_timestamp;
            return;
        }

        // Start measure jitter
        if (unfilteredBtn != driver.get()) {
            unfilteredBtn = driver.get();
            unfilteredBtnTimestamp = ms_timestamp;
        }

        // Update filtered values
        if ((ms_timestamp - unfilteredBtnTimestamp) > JITTER_THRESHOLD && (btnPressed != unfilteredBtn)) {
            btnPressed = unfilteredBtn;
            prevPeriod = ms_timestamp - btnToggleTimestamp;
            btnToggleTimestamp = ms_timestamp;
            if (btnPressed) {
                handleEvent(ButtonEventId::BUTTON_PRESS_START);
            }
        }
        currentPeriod = ms_timestamp - btnToggleTimestamp;

        switch (state)
        {
        case START:
            if (btnPressed) {
                //handleEvent(ButtonEventId::BUTTON_SEQUENCE_START);
                state = CHECK_FIRST_PRESS;
                return;
            }
            return;

        case CHECK_FIRST_PRESS:
            if (btnPressed && currentPeriod >= SHORT_PRESS_THRESHOLD) {
                state = WAIT_LONG_PRESS;
                handleEvent(ButtonEventId::BUTTON_LONG_PRESS_START);
                return;
            }

            if (!btnPressed) {
                shortPressesCounter = 1;
                state = WAIT_SHORT_RELEASE;
                return;
            }
            return;

        case WAIT_LONG_PRESS:
            if (btnPressed && currentPeriod > LONG_PRESS_THRESHOLD) {
                state = WAIT_LONG_RELEASE;
                handleEvent(ButtonEventId::BUTTON_LONG_PRESS);
                return;
            }

            // Released too early => terminate
            if (!btnPressed) {
                state = START;
                handleEvent(ButtonEventId::BUTTON_LONG_PRESS_FAIL);
                //handleEvent(ButtonEventId::BUTTON_SEQUENCE_END);
                return;
            }
            return;

        case WAIT_LONG_RELEASE:
            if (!btnPressed) {
                state = START;
                //handleEvent(ButtonEventId::BUTTON_SEQUENCE_END);
                return;
            }
            return;

        case WAIT_SHORT_RELEASE:
            // If button pressed fast again => proceed next click
            if (btnPressed && currentPeriod < SHORT_PRESS_THRESHOLD) {
                state = WAIT_SHORT_PRESS;
                return;
            }

            // Button stays released long enough => sequence finished
            if (currentPeriod >= SHORT_PRESS_THRESHOLD) {
                state = START;
                if (shortPressesCounter <= 5) {
                    handleEvent(static_cast<ButtonEventId>(static_cast<uint8_t>(ButtonEventId::BUTTON_PRESSED_1X) + shortPressesCounter - 1));
                }
                //handleEvent(ButtonEventId::BUTTON_SEQUENCE_END);
                return;
            }
            return;

        case WAIT_SHORT_PRESS:
            // If button pressed too long => bad sequence of short dials
            if (btnPressed && currentPeriod >= SHORT_PRESS_THRESHOLD) {
                state = START;
                //handleEvent(ButtonEventId::BUTTON_SEQUENCE_END);
                return;
            }

            // If button released fast enouth => go to pause measurement
            if (!btnPressed) {
                shortPressesCounter++;
                state = WAIT_SHORT_RELEASE;
                return;
            }
            return;
        }
    }

private:
    enum State {
        START,
        CHECK_FIRST_PRESS,
        WAIT_LONG_PRESS,
        WAIT_LONG_RELEASE,
        WAIT_SHORT_RELEASE,
        WAIT_SHORT_PRESS
    };

    Driver driver{};
    std::function<void(ButtonEventId)> eventHandler{nullptr};

    State state{START};
    bool unfilteredBtn{false};
    bool btnPressed{false};
    uint32_t unfilteredBtnTimestamp{0};
    uint32_t btnToggleTimestamp{0};
    uint32_t prevPeriod{0};
    uint32_t currentPeriod{0};
    uint8_t shortPressesCounter{0};

    void handleEvent(ButtonEventId event) {
        if (eventHandler) eventHandler(event);
    }
};
