// #include "mbed.h"
// #include "stm32l475e_iot01_accelero.h"
// #include "stm32l475e_iot01_gyro.h"
// #include "stm32l475e_iot01.h"
// #include <math.h>
// #include <stdio.h>
// #include "mbed_events.h"

// #define PI 3.14159265
// Thread buttonThread;
// Semaphore button_sem(1);
// DigitalOut led1(LED1);

// typedef struct Vector3D {
//     double x = 0;
//     double y = 0;
//     double z = 0;
// } Vector3D;

// void printF(double num1, double num2, double num3) {
//     printf("{ %d, %d, %d }\n", int(num1*1000), int(num2*1000), int(num3*1000));
// }

// void button_thread(void const *name) {
//     button_sem.acquire();
//     HAL_Delay(1000);
//     button_sem.release();
// }

// void button_pressed() {
//     led1=!led1;
// }

// void button_released() {
//     led1=!led1;
// }

// EventQueue queue;
// InterruptIn button1(BUTTON1);

// int main()
// {
//     button1.fall(&button_pressed);
//     button1.rise(&button_released);
//     const int a1 = 1;
//     buttonThread.start(callback(button_thread, (void *)&a1));

//     int sample_num = 0;
//     int16_t pDataXYZ[3];
//     double now_total;
//     float gDataXYZ[3];
//     Vector3D now;
//     int tmp[3];
//     int size;
//     int SCALE_MULTIPLIER = 1;
//     int count = 0;
//     BSP_ACCELERO_Init();
//     BSP_GYRO_Init();
//     while ( true ){
//         BSP_ACCELERO_AccGetXYZ(pDataXYZ);
//         now_total = sqrt( pDataXYZ[0] * pDataXYZ[0] + pDataXYZ[1] * pDataXYZ[1] + pDataXYZ[2] * pDataXYZ[2] );
//         now.x += pDataXYZ[0] / now_total / 10;
//         now.y += pDataXYZ[1] / now_total / 10;
//         now.z += pDataXYZ[2] / now_total / 10;
//         count++;
//         if (count%10==0){
//             if (now.x > 0.4){
//                 if (now.y > 0.3){
//                     printf("Up & Left ");
//                     printF(now.x, now.y, now.z);
//                 }else if (now.y < -0.3){
//                     printf("Up & Right ");
//                     printF(now.x, now.y, now.z);
//                 }else{
//                     printf("Up ");
//                     printF(now.x, now.y, now.z);
//                 }
//             }else if (now.x < -0.4){
//                 if (now.y > 0.3){
//                     printf("Down & Left ");
//                     printF(now.x, now.y, now.z);
//                 }else if (now.y < -0.3){
//                     printf("Down & Right ");
//                     printF(now.x, now.y, now.z);
//                 }else{
//                     printf("Down ");
//                     printF(now.x, now.y, now.z);
//                 }
//             }else{
//                 if (now.y > 0.4){
//                     printf("Left ");
//                     printF(now.x, now.y, now.z);
//                 }
//                 else if (now.y < -0.4){
//                     printf("Right ");
//                     printF(now.x, now.y, now.z);
//                 }
//                 else{
//                     printf("Flat ");
//                     printF(now.x, now.y, now.z);
//                 }

//             }
//             now.x = 0;
//             now.y = 0;
//             now.z = 0;
//         }
//         HAL_Delay(10);
//     }
// }
