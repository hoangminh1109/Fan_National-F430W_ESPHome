/*
 * Copyright 2025 Hoang Minh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <deque>
#include <map>

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/button/button.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_transmitter/remote_transmitter.h"
#include "esphome/core/gpio.h"

#ifndef USE_ESP32
#error "F430W fan component only supports ESP32"
#endif

namespace esphome
{
  namespace f430wfan
  {

    static const char *const TAG = "f430wfan.fan";

    enum FanSpeed
    {
      F430W_FAN_OFF = 0,
      F430W_FAN_LOW = 1,
      F430W_FAN_MEDIUM = 2,
      F430W_FAN_HIGH = 3,
      F430W_FAN_INVALID = 100
    };

    enum FanOscilatting
    {
      F430W_FAN_OSC_OFF = 0,
      F430W_FAN_OSC_ON = 1,
      F430W_FAN_OSC_INVALID = 100,
    };

    #define F430W_FAN_TIMER_OFF 0
    #define F430W_FAN_TIMER_1H 1
    #define F430W_FAN_TIMER_2H 2
    #define F430W_FAN_TIMER_4H 3
    #define F430W_FAN_TIMER_8H 4
    #define F430W_FAN_TIMER_INVALID 255

    enum FanMode {
      F430W_FAN_MODE_OFF = 0,
      F430W_FAN_MODE_NORMAL = 1,
      F430W_FAN_MODE_NATURE = 2,
      F430W_FAN_MODE_INVALID = 100,
    };


    struct FanState
    {
      FanSpeed speed;
      FanOscilatting oscillating;
      FanMode mode;
      uint8_t timer;
    };

    #define STR_FANMODE_OFF     "---"
    #define STR_FANMODE_NORMAL  "🌬️ Normal"
    #define STR_FANMODE_NATURE  "🍃 Nature"

    static std::map<uint8_t, std::string> FANMODE_STR = {
      {F430W_FAN_MODE_OFF, STR_FANMODE_OFF},
      {F430W_FAN_MODE_NORMAL, STR_FANMODE_NORMAL},
      {F430W_FAN_MODE_NATURE, STR_FANMODE_NATURE},
    };

    static std::map<std::string, uint8_t> FANMODE_ID = {
      {STR_FANMODE_OFF, F430W_FAN_MODE_OFF},
      {STR_FANMODE_NORMAL, F430W_FAN_MODE_NORMAL},
      {STR_FANMODE_NATURE, F430W_FAN_MODE_NATURE},
    };

    #define STR_FANTIMER_NONE_STR "---"
    #define STR_FANTIMER_1_0H_STR "1.0h"
    #define STR_FANTIMER_2_0H_STR "2.0h"
    #define STR_FANTIMER_4_0H_STR "4.0h"
    #define STR_FANTIMER_8_0H_STR "8.0h"

    // Array using the defines
    static const char *STR_FANTIMER_STR[] = {
        STR_FANTIMER_NONE_STR,
        STR_FANTIMER_1_0H_STR,
        STR_FANTIMER_2_0H_STR,
        STR_FANTIMER_4_0H_STR,
        STR_FANTIMER_8_0H_STR,
    };

    // led bit position
    #define F430W_FAN_LED_LO          0
    #define F430W_FAN_LED_MED         1 
    #define F430W_FAN_LED_HI          2
    #define F430W_FAN_LED_SWING       3
    #define F430W_FAN_LED_ROW1        4
    #define F430W_FAN_LED_ROW2        5
    #define F430W_FAN_LED_ROW3        6
    #define F430W_FAN_LED_ROW4        7
    #define F430W_FAN_LED_COL1        8
    #define F430W_FAN_LED_COL2        9
    #define F430W_FAN_LED_COUNT       10

    // Pulse parameters in usec
    const uint16_t F430W_FAN_BIT_MARK = 700;
    const uint16_t F430W_FAN_ONE_SPACE = 1550;
    const uint16_t F430W_FAN_ZERO_SPACE = 450;
    const uint16_t F430W_FAN_HEADER_MARK = 8500;
    const uint16_t F430W_FAN_HEADER_SPACE = 4440;
    const uint16_t F430W_FAN_FRAME_END = 10000;

    static std::array<std::vector<uint8_t>, 6> IR_COMMANDS = {{
        {0X00, 0xFF, 0X11, 0xEE}, // OFF
        {0X00, 0xFF, 0X0D, 0xF2}, // ON
        {0X00, 0xFF, 0X0D, 0xF2}, // SPEED
        {0X00, 0xFF, 0X05, 0xFA}, // OSCIL
        {0X00, 0xFF, 0X09, 0xF6}, // TIMER
        {0X00, 0xFF, 0X01, 0xFE}  // RHYTHIM
    }};

    // command
    #define F430W_FAN_CMD_OFF     0
    #define F430W_FAN_CMD_ON      1
    #define F430W_FAN_CMD_SPEED   2
    #define F430W_FAN_CMD_OSCIL   3
    #define F430W_FAN_CMD_TIMER   4
    #define F430W_FAN_CMD_RHYTHIM 5

    class F430WFan;

    class F430WFanTimer : public text_sensor::TextSensor, public Component
    {
    public:
      void setup() override;
      void dump_config() override;
      void set_parent_fan(F430WFan *fan) { this->fan_ = fan; }
      void set_fan_timer(uint8_t timer);

    private:
      F430WFan *fan_{nullptr};
    };

    class F430WFanSetTimer : public button::Button, public Component
    {
    public:
      void setup() override;
      void dump_config() override;
      void press_action() override;
      void set_parent_fan(F430WFan *fan) { this->fan_ = fan; }

    private:
      F430WFan *fan_{nullptr};
    };

    class F430WFan : public Component,
                    public fan::Fan,
                    public remote_base::RemoteReceiverListener,
                    public remote_base::RemoteTransmittable
    {
    public:
      F430WFan() {}
      void setup() override;
      void dump_config() override;
      void loop() override;
      void set_interval_ms(int interval_ms) { this->interval_ms_ = interval_ms; }
      void set_fan_timer(F430WFanTimer *fan_timer) { this->fan_timer_ = fan_timer; }
      void set_fan_settimer(F430WFanSetTimer *fan_settimer) { this->fan_settimer_ = fan_settimer; }
      void set_pin_idx(uint8_t pin_idx, InternalGPIOPin *pin) { pins_[pin_idx] = pin; }
      fan::FanTraits get_traits() override { return this->traits_; }
      std::deque<uint8_t> command_queue{};
      void process_command();
      bool processing{false};

    protected:
      bool on_receive(remote_base::RemoteReceiveData data) override;
      void control(const fan::FanCall &call) override;

    private:
      fan::FanTraits traits_;
      
      uint32_t last_run_{0};
      uint8_t sample_step{0};
      uint8_t led_on_counts[4][2] = {};
      FanState fan_state_;

      int interval_ms_{0};

      F430WFanTimer *fan_timer_{nullptr};
      F430WFanSetTimer *fan_settimer_{nullptr};

      InternalGPIOPin *pins_[F430W_FAN_LED_COUNT];

      void update_state();
      bool decode_data(remote_base::RemoteReceiveData data, std::vector<uint8_t> &state_bytes);
      void transmit_command(uint8_t cmd);
    };

  } // namespace f430wfan
} // namespace esphome
