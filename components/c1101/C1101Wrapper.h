#pragma once
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h"
#include <Arduino.h>
#include <Preferences.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

namespace esphome {
namespace c1101 {

class C1101Wrapper : public Component, public sensor::Sensor
{

private:
    int index;
    float freq = 433.42; // MHz;

public:
    void setFrequency(float _freq)
    {
        freq = _freq;
    }

    void setup() override
    {

        ESP_LOGD("C1101Wrapper.h", "Cover %d", index);

        // This will be called by App.setup()
        ESP_LOGD("C1101Wrapper.h", "Starting Device");
        ESP_LOGD("C1101Wrapper.h", "C1101 ESPHome v1.00");
        ESP_LOGD("C1101Wrapper.h", "Initialize remote device");

        if (ELECHOUSE_cc1101.getCC1101()) {
            ESP_LOGD("C1101Wrapper.h", "Communication established with the CC1101 module");
        }
        else {
            ESP_LOGD("C1101Wrapper.h", "Error: Could not establish communication with the CC1101 module");
        }

        ELECHOUSE_cc1101.Init();
        ELECHOUSE_cc1101.setMHZ(freq);
    }

    void startTransmit()
    {
        ELECHOUSE_cc1101.SetTx();
    }

    void stopTransmit()
    {
        ELECHOUSE_cc1101.setSidle();
    }
};

}  // namespace c1101
}  // namespace esphome