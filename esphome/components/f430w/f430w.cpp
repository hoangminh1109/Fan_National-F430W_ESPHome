#include "f430w.h"
#include "esphome/core/log.h"

#include "esphome/components/remote_base/nec_protocol.h"

#include <vector>
#include <array>
#include <bitset>

namespace esphome
{
  namespace f430wfan
  {

    using NECData  = esphome::remote_base::NECData;
    using NECProtocol  = esphome::remote_base::NECProtocol;
    using Flags = esphome::gpio::Flags;

    void F430WFan::setup()
    {
      last_run_ = millis();

      auto restore = this->restore_state_();
      if (restore.has_value())
      {
        restore->apply(*this);
      }

      for (int i = 0; i < F430W_FAN_LED_COUNT; i++)
      {
        this->pins_[i]->setup();
      }

      // Construct traits
      fan::FanTraits fan_traits(true, true, false, 3);
      fan_traits.set_supported_preset_modes({STR_FANMODE_NORMAL, STR_FANMODE_NATURE});
      this->traits_ = fan_traits;

      this->fan_state_.speed = F430W_FAN_INVALID;
      this->fan_state_.oscillating = F430W_FAN_OSC_INVALID;
      this->fan_state_.timer = F430W_FAN_TIMER_INVALID;
      this->fan_state_.mode = F430W_FAN_MODE_INVALID;

      this->fan_timer_->publish_state(STR_FANTIMER_NONE_STR);
    }

    void F430WFan::dump_config()
    {
      LOG_FAN(TAG, "F430WFan Wall Fan", this);
    }

    void F430WFan::transmit_command(uint8_t cmd)
    {
      std::vector<uint8_t> command = IR_COMMANDS[cmd];

      auto transmit = this->transmitter_->transmit();
      auto *data = transmit.get_data();

      // First frame
      data->mark(F430W_FAN_HEADER_MARK);
      data->space(F430W_FAN_HEADER_SPACE);
      for (uint8_t i_byte = 0; i_byte < command.size(); i_byte++)
      {
        for (uint8_t i_bit = 0; i_bit < 8; i_bit++)
        {
          data->mark(F430W_FAN_BIT_MARK);
          bool bit = command[i_byte] & (1 << i_bit);
          data->space(bit ? F430W_FAN_ONE_SPACE : F430W_FAN_ZERO_SPACE);
        }
      }
      data->mark(F430W_FAN_BIT_MARK);
      data->space(F430W_FAN_FRAME_END);

      // transmit
      transmit.perform();
    }

    bool F430WFan::decode_data(remote_base::RemoteReceiveData data, std::vector<uint8_t>& state_bytes)
    {
#if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE)
          auto raw_data = data.get_raw_data();

          if (!data.expect_item(F430W_FAN_HEADER_MARK, F430W_FAN_HEADER_SPACE))
          {
              ESP_LOGV(TAG, "Invalid data - expected header");  
              return false;
          }

          state_bytes.clear();
          while (data.get_index() + 2 < raw_data.size())
          {
              uint8_t byte = 0;
              for (uint8_t a_bit = 0; a_bit < 8; a_bit++)
              {
                  if (data.expect_item(F430W_FAN_BIT_MARK, F430W_FAN_FRAME_END))
                  {
                      // expect new header if there are remain data
                      if (!data.expect_item(F430W_FAN_HEADER_MARK, F430W_FAN_HEADER_SPACE))
                      {
                          ESP_LOGV(TAG, "Invalid data - expected header at index = %d", data.get_index());
                          return false;
                      }
                  }
                  
                  // bit 1
                  if (data.expect_item(F430W_FAN_BIT_MARK, F430W_FAN_ONE_SPACE))
                  {
                      byte |= 1 << a_bit;
                  }
                  // bit 0
                  else if (data.expect_item(F430W_FAN_BIT_MARK, F430W_FAN_ZERO_SPACE))
                  {
                      // 0 already initialized, hence do nothingg here
                  }
                  else
                  {
                      ESP_LOGV(TAG, "Invalid bit %d of byte %d, index = %d", a_bit, state_bytes.size(), data.get_index());
                      return false;
                  }
              }
              state_bytes.push_back(byte);
          }

          std::string hex_str = "";
          for (uint8_t i = 0; i < state_bytes.size(); i++)
          {
              char buf[6];
              snprintf(buf, sizeof(buf), "%02X ", state_bytes[i]);
              hex_str += buf;
          }
          
          ESP_LOGV(TAG, "Command decoded: len = %d, data = [ %s]", state_bytes.size(), hex_str.c_str());
#endif
        return true;
        
    }    

    bool F430WFan::on_receive(remote_base::RemoteReceiveData data)
    {
#if (ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE)
      auto raw_data = data.get_raw_data();

      ESP_LOGV(TAG, "Received raw data size = %d", raw_data.size());

      for (uint32_t i = 0; i < raw_data.size(); i++)
      {
          ESP_LOGV(TAG, "Raw data index = %d, data = %d", i, raw_data[i]);
      }

      std::vector<uint8_t> state_bytes;
      if (!decode_data(data, state_bytes))
      {
          ESP_LOGV(TAG, "Decode ir data failed");
          return false;
      }      
#endif
      return true;
    }

    void F430WFan::update_state()
    {
      uint32_t now = millis();
      if (now - this->last_run_ >= this->interval_ms_)
      {
        this->last_run_ = now;

        // status leds
        uint8_t led_scan_row[4] = {
                                  this->pins_[F430W_FAN_LED_ROW1]->digital_read(),
                                  this->pins_[F430W_FAN_LED_ROW2]->digital_read(),
                                  this->pins_[F430W_FAN_LED_ROW3]->digital_read(),
                                  this->pins_[F430W_FAN_LED_ROW4]->digital_read()
                                };
        uint8_t led_scan_col[2] = {
                                  this->pins_[F430W_FAN_LED_COL1]->digital_read(),
                                  this->pins_[F430W_FAN_LED_COL2]->digital_read()
                                };

        for (uint8_t r = 0; r < 4; r++)
        {
          if (led_scan_row[r] == 1)
          {
            for (uint8_t c = 0; c < 2; c++)
            {
              if (led_scan_col[c] == 0)
                this->led_on_counts[r][c]++;
            }
          }
        }

        sample_step++;
        if (sample_step >= 16)
        {
          bool led_state[4][2] = {};
          sample_step = 0;
          for (uint8_t r = 0; r < 4; r++)
          {
            for (uint8_t c = 0; c < 2; c++)
            {
              led_state[r][c] = (this->led_on_counts[r][c] > 2);
              this->led_on_counts[r][c] = 0;
            }
          }

          bool state_change = false;

          FanMode mode = F430W_FAN_MODE_INVALID;
          if (!led_state[0][1]) mode = F430W_FAN_MODE_NORMAL;
          else mode = F430W_FAN_MODE_NATURE;

          // fan mode
          if (this->fan_state_.mode != mode)
          {
            this->fan_state_.mode = mode;
            this->set_preset_mode_(FANMODE_STR[static_cast<uint8_t>(this->fan_state_.mode)]);
            if (this->fan_state_.mode == F430W_FAN_MODE_NATURE)
            {
              this->fan_state_.speed = F430W_FAN_OFF;
              this->speed = static_cast<int>(this->fan_state_.speed);
            }
            state_change = true;
          }

          if (this->fan_state_.mode == F430W_FAN_MODE_NORMAL)
          {
            // speed
            FanSpeed speed = F430W_FAN_OFF;
            if ( !(this->pins_[F430W_FAN_LED_HI]->digital_read()) ) speed = F430W_FAN_HIGH;
            if ( !(this->pins_[F430W_FAN_LED_MED]->digital_read()) ) speed = F430W_FAN_MEDIUM;
            if ( !(this->pins_[F430W_FAN_LED_LO]->digital_read()) ) speed = F430W_FAN_LOW;
            // fan speed
            if (this->fan_state_.speed != speed)
            {
              this->fan_state_.speed = speed;
              if (this->fan_state_.speed == F430W_FAN_OFF)
              {
                this->state = false;
              }
              else
              {
                this->state = true;
                this->speed = static_cast<int>(this->fan_state_.speed);
              }
              state_change = true;
            }
          }

          // swing
          FanOscilatting osc = F430W_FAN_OSC_OFF;
          if ( !(this->pins_[F430W_FAN_LED_SWING]->digital_read()) ) osc = F430W_FAN_OSC_ON;

          // fan oscillating
          if (this->fan_state_.oscillating != osc)
          {
            this->fan_state_.oscillating = osc;
            this->oscillating = static_cast<bool>(this->fan_state_.oscillating);
            state_change = true;  
          }

          uint8_t timer = F430W_FAN_TIMER_OFF;
          if (led_state[0][0])
            timer = (F430W_FAN_TIMER_1H);
          if (led_state[2][0])
            timer = (F430W_FAN_TIMER_2H);
          if (led_state[3][0])
            timer = (F430W_FAN_TIMER_4H);
          if (led_state[1][0])
            timer = (F430W_FAN_TIMER_8H);

          // fan timer
          if (this->fan_state_.timer != timer)
          {
            this->fan_state_.timer = timer;
            this->fan_timer_->set_fan_timer(this->fan_state_.timer);
          }


          if (state_change)
          {
            this->publish_state();
          }

        }
      }
    }

    void F430WFan::loop()
    {
      // skip if it is processing a command
      if (this->processing)
      {
        return;
      }

      this->update_state();
    }

    void F430WFan::process_command()
    {
      if (this->command_queue.size() == 0)
      {
        this->set_timeout("resume_operation",
                          50,
                          [this]()
                          {
                            this->update_state();
                            this->processing = false;
                          });
        return;
      }

      uint8_t cmd = this->command_queue.front();
      uint32_t timeout = (cmd == F430W_FAN_CMD_ON ? 3200 : 150); // on startup, the fan starts at MID level for ~3sec and return to LO level
      this->transmit_command(cmd);
      this->command_queue.pop_front();
      if (this->command_queue.size() > 0)
      {
        this->set_timeout("next_cmd",
                          timeout,
                          [this]()
                          {
                            this->process_command();
                          });
      }
      else
      {
        this->process_command();
      }
    }

    void F430WFan::control(const fan::FanCall &call)
    {
      // skip if it is processing a command
      if (this->processing)
      {
        return;
      }

      if (call.get_state().has_value())
      {
        bool newstate = *call.get_state();

        if (!newstate) // off
        {
          if (this->state)
          {
            this->command_queue.push_back(F430W_FAN_CMD_OFF);
          }
        }
        else // on
        {
          if (!this->state) // if currently off, we need a on command first. the fan runs at LO level after start.
          {
            this->command_queue.push_back(F430W_FAN_CMD_ON);
          }

          if (call.get_speed().has_value())
          {
            uint8_t currspeed = (!this->state) ? 1 : this->speed;
            uint8_t newspeed = *call.get_speed();
            newspeed = newspeed < currspeed ? newspeed + 3 : newspeed;
            this->command_queue.insert(this->command_queue.end(), newspeed - currspeed, F430W_FAN_CMD_SPEED);
          }
        }
      }

      if (call.get_oscillating().has_value())
      {
        if (this->oscillating != *call.get_oscillating())
        {
          this->command_queue.push_back(F430W_FAN_CMD_OSCIL);
        }
      }

      if (this->state && (call.has_preset_mode()) && (call.get_preset_mode() != STR_FANMODE_OFF))
      {
        if (call.get_preset_mode() != this->get_preset_mode())
        {
          this->command_queue.push_back(F430W_FAN_CMD_RHYTHIM);
        }
      }

      if (this->command_queue.size() > 0)
      {
        // start processing
        this->processing = true;
        this->process_command();
      }
    }

    void F430WFanTimer::dump_config()
    {
      ESP_LOGCONFIG(TAG, "F430WFanTimer:");
      LOG_TEXT_SENSOR("  Fan Timer: ", "fan_timer", this);
    }

    void F430WFanTimer::setup()
    {
      this->set_icon("mdi:timer-outline");
    }

    void F430WFanTimer::set_fan_timer(uint8_t timer)
    {
      this->publish_state(STR_FANTIMER_STR[timer]);
    }

    void F430WFanSetTimer::dump_config()
    {
      ESP_LOGCONFIG(TAG, "F430WFanSetTimer:");
      LOG_BUTTON("  Fan Set Timer: ", "fan_settimer", this);
    }

    void F430WFanSetTimer::setup()
    {
    }

    void F430WFanSetTimer::press_action()
    {
      if (this->fan_->state)
      {
        this->fan_->command_queue.push_back(F430W_FAN_CMD_TIMER);
        // start processing
        this->fan_->processing = true;
        this->fan_->process_command();
      }
    }

  } // namespace f430wfan
} // namespace esphome
