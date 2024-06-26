/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
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

#include <events/mbed_events.h>
#include "ble/BLE.h"
#include "ble/gap/Gap.h"
// #include "ble/services/HeartRateService.h"
#include "stm32l475e_iot01_accelero.h"
#include "stm32l475e_iot01_gyro.h"
#include "stm32l475e_iot01.h"
#include "ButtonService.h"
#include "LSM6DSLService.h"
#include "pretty_printer.h"
#include "mbed-trace/mbed_trace.h"

using namespace std::literals::chrono_literals;

const static char DEVICE_NAME[] = "Group1";

static events::EventQueue event_queue(/* event count */ 32 * EVENTS_EVENT_SIZE);

static int count_press = 0;
int idle_time = 0;
bool isConnected = false;
Thread t;

class ControlBLE : ble::Gap::EventHandler
{
public:
    ControlBLE(
        BLE &ble,
        events::EventQueue &event_queue,
        int count) : _ble(ble),
                                            _event_queue(event_queue),
                                            _count(count),
                                            _led1(LED1, 1),
                                            _button(BLE_BUTTON_PIN_NAME, BLE_BUTTON_PIN_PULL),
                                            _button_service(NULL),
                                            _button_uuid(ButtonService::BUTTON_SERVICE_UUID),
                                            _pDataXYZ(),
                                            _lsm6dsl_service(ble, _pDataXYZ),
                                            _lsm6dsl_uuid(LSM6DSLService::LSM6DSL_SERVICE_UUID),

                                            _adv_data_builder(_adv_buffer)
    {
        // TODO: Fill the buffer 
        // while not full pDataX, pDataY => fill it
    }

    void start()
    {
        _ble.init(this, &ControlBLE::on_init_complete);

        _event_queue.dispatch_forever();
    }

private:
    /** Callback triggered when the ble initialization process has finished */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *params)
    {
        if (params->error != BLE_ERROR_NONE)
        {
            printf("Ble initialization failed.");
            return;
        }

        print_mac_address();

        /* this allows us to receive events like onConnectionComplete() */
        _ble.gap().setEventHandler(this);

        /* heart rate value updated every second */
        _event_queue.call_every(
            100ms,
            [this]
            {
                update_sensor_value();
            });

        _button_service = new ButtonService(_ble, 0 /* initial value for button pressed */);
        // _lsm6dsl_service = new LSM6DSLService(_ble, _pDataXYZ, _gDataXYZ);

        _button.fall(Callback<void()>(this, &ControlBLE::button_pressed));
        _button.rise(Callback<void()>(this, &ControlBLE::button_released));

        start_advertising();
    }

    void start_advertising()
    {
        /* Create advertising parameters and payload */

        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(100)));

        _adv_data_builder.setFlags();
        _adv_data_builder.setLocalServiceList(mbed::make_Span(&_button_uuid, 1));
        _adv_data_builder.setLocalServiceList({&_lsm6dsl_uuid, 1});
        _adv_data_builder.setName(DEVICE_NAME);

        /* Setup advertising */

        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters);

        if (error)
        {
            printf("_ble.gap().setAdvertisingParameters() failed\r\n");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            _adv_data_builder.getAdvertisingData());

        if (error)
        {
            printf("_ble.gap().setAdvertisingPayload() failed\r\n");
            return;
        }

        /* Start advertising */

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error)
        {
            printf("_ble.gap().startAdvertising() failed\r\n");
            return;
        }

        printf("LSM6DSL sensor advertising, please connect\r\n");
    }

    void update_sensor_value()
    {
        if(isConnected){
            if(idle_time <= 50 || idle_time%10 == 0){
                BSP_ACCELERO_AccGetXYZ(_pDataXYZ);
                if (!(_pDataXYZ[0] < 300 && _pDataXYZ[0] > -300 && _pDataXYZ[1] < 300 && _pDataXYZ[1] > -300)){
                    idle_time = 0;
                    _lsm6dsl_service.updateLSM6DSLState(_pDataXYZ);
                    
                }else{
                    idle_time = idle_time + 1;
                }
            }
            else{
                idle_time = idle_time + 1;
            }
        }
    }

    /* these implement ble::Gap::EventHandler */
private:
    /* when we connect we stop advertising, restart advertising so others can connect */
    virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
    {
        if (event.getStatus() == ble_error_t::BLE_ERROR_NONE)
        {
            printf("Client connected, you may now subscribe to updates\r\n");
            isConnected = true;
        }
    }

    /* when we connect we stop advertising, restart advertising so others can connect */
    virtual void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
    {
        printf("Client disconnected, restarting advertising\r\n");
        isConnected = false;

        ble_error_t error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error)
        {
            printf("_ble.gap().startAdvertising() failed\r\n");
            return;
        }
    }

    void button_pressed(void)
    {
        _led1 = true;
        _count = (_count + 1) % 2;
        int ret = _count * 2 + 1;
        _event_queue.call(Callback<void(int)>(_button_service, &ButtonService::updateButtonState), ret);
    }

    void button_released(void)
    {
        _led1 = false;
        int ret = _count * 2;
        _event_queue.call(Callback<void(int)>(_button_service, &ButtonService::updateButtonState), ret);
    }

private:
    BLE &_ble;
    events::EventQueue &_event_queue;
    int _count;

    int16_t _pDataXYZ[3];
    LSM6DSLService _lsm6dsl_service;
    UUID _lsm6dsl_uuid;

    DigitalOut _led1;
    InterruptIn _button;
    ButtonService *_button_service;
    UUID _button_uuid;

    uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
    ble::AdvertisingDataBuilder _adv_data_builder;
};

/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    mbed_trace_init();
    BSP_ACCELERO_Init();

    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(schedule_ble_events);

    ControlBLE demo(ble, event_queue, count_press);
    demo.start();

    return 0;
}
