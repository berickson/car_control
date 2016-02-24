#define TEENSY_LAYOUT_2

#ifdef TEENSY_LAYOUT_2

#define PIN_RX_SPEED 4
#define PIN_RX_STEER 3

#define PIN_U_SPEED 6
#define PIN_U_STEER 5
#define PIN_PING_TRIG 17
#define PIN_PING_ECHO 32

#define PIN_ODOMOTER_SENSOR_A 25
#define PIN_ODOMOTER_SENSOR_B 8

#define PIN_MOTOR_RPM 29

#define PIN_SPEAKER   30
#define PIN_LED 13


#else
#define PIN_RX_SPEED 1
#define PIN_RX_STEER 2

#define PIN_U_SPEED 3
#define PIN_U_STEER 4
#define PIN_PING_TRIG 6
#define PIN_PING_ECHO 23

#define PIN_ODOMOTER_SENSOR_A 22
#define PIN_ODOMOTER_SENSOR_B 7

#define PIN_MOTOR_RPM 21

#define PIN_SPEAKER   9
#define PIN_LED 13

#endif
