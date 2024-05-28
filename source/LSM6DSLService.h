/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
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

#ifndef __BLE_LSM6DSL_SERVICE_H__
#define __BLE_LSM6DSL_SERVICE_H__

class LSM6DSLService
{
public:
    const static uint16_t LSM6DSL_SERVICE_UUID = 0xA002;
    const static uint16_t LSM6DSL_STATE_CHARACTERISTIC_UUID = 0xA003;

    union SensorValue
    {
        struct
        {
            int16_t px;
            int16_t py;
            int16_t pz;
            float gx;
            float gy;
            float gz;
        };
        uint8_t row[20];

        SensorValue(int16_t *pDataXYZ, float *gDataXYZ)
        {
            px = pDataXYZ[0];
            py = pDataXYZ[1];
            pz = pDataXYZ[2];
            gx = gDataXYZ[0];
            gy = gDataXYZ[1];
            gz = gDataXYZ[2];
        }

        SensorValue() : px(0), py(0), pz(0), gx(0), gy(0), gz(0) {}

        void updateSensorValue(int16_t *pDataXYZ, float *gDataXYZ)
        {
            px = pDataXYZ[0];
            py = pDataXYZ[1];
            pz = pDataXYZ[2];
            gx = gDataXYZ[0];
            gy = gDataXYZ[1];
            gz = gDataXYZ[2];
            // px = 1;
            // py = 2;
            // pz = 3;
            // gx = (float)4.0;
            // gy = (float)5.0;
            // gz = (float)6.0;
        }

        uint8_t *getPointer()
        {
            return row;
        }

        const uint8_t *getPointer() const
        {
            return row;
        }

        unsigned getNumValueBytes() const
        {
            return sizeof(SensorValue);
        }
    };

    LSM6DSLService(
        BLE &_ble,
        int16_t *pDataXYZ,
        float *gDataXYZ) : ble(_ble),
                           sensorData(pDataXYZ, gDataXYZ),
                           LSM6DSLState(
                               LSM6DSLService::LSM6DSL_STATE_CHARACTERISTIC_UUID,
                               //    reinterpret_cast<SensorValue *>(&sensorData),
                               sensorData.getPointer(),
                               sensorData.getNumValueBytes(),
                               sensorData.getNumValueBytes(),
                               GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)
    {
        GattCharacteristic *charTable[] = {&LSM6DSLState};
        GattService myService(
            LSM6DSLService::LSM6DSL_SERVICE_UUID,
            charTable,
            sizeof(charTable) / sizeof(charTable[0]));
        ble.gattServer().addService(myService);
    }

    void updateLSM6DSLState(int16_t *pDataXYZ, float *gDataXYZ)
    {
        sensorData.updateSensorValue(pDataXYZ, gDataXYZ);
        ble.gattServer().write(
            LSM6DSLState.getValueHandle(),
            sensorData.getPointer(),
            sensorData.getNumValueBytes());
    }

private:
    BLE &ble;
    SensorValue sensorData;
    GattCharacteristic LSM6DSLState;
};

#endif /* #ifndef __BLE_LSM6DSL_SERVICE_H__ */
