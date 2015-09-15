#include "Mpu9150.h"

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
  mpuInterrupt = true;
}

void Mpu9150::setup() {
  Wire.begin();
  //Serial.begin(115200);

  // initialize device
  Serial.println(F("Initializing I2C MPU devices..."));
  mpu.initialize();

  // verify connection
  Serial.println(F("Testing device connections..."));
  Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));


  // load and configure the DMP
  Serial.println(F("Initializing DMP..."));
  devStatus = mpu.dmpInitialize();

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP, now that it's ready
    Serial.println(F("Enabling DMP..."));
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    Serial.print(F("Enabling interrupt detection (Arduino external interrupt ")); Serial.println(INTERRUPT_NUMBER);
    pinMode(INTERRUPT_PIN, INPUT);
    attachInterrupt(INTERRUPT_NUMBER,dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    Serial.println(F("DMP ready! Waiting for first interrupt..."));
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)
    Serial.print(F("DMP Initialization failed (code "));
    Serial.print(devStatus);
    Serial.println(F(")"));
  }

}

double Mpu9150::ground_angle() {
  if(!initialReading) return 0.0;

  Quaternion qdiff = diff(q,q0);
  double angle = rads2degrees(normal_angle(qdiff,gravity));
  return angle;
}


void Mpu9150::trace_status() {

  Serial.print(readingCount);
  Serial.print("\t");
  Serial.print("angle");
  Serial.print("\t");
  Serial.print(ground_angle());
  Serial.print("\t");
  Serial.print("quat\t");
  Serial.print(q.w);
  Serial.print("\t");
  Serial.print(q.x);
  Serial.print("\t");
  Serial.print(q.y);
  Serial.print("\t");
  Serial.print(q.z);
  Serial.print("\t");
  Serial.print("gravity\t");
  Serial.print(gravity.x);
  Serial.print("\t");
  Serial.print(gravity.y);
  Serial.print("\t");
  Serial.print(gravity.z);
  Serial.print("\t");
  Serial.print("aa\t");
  Serial.print(aa.x);
  Serial.print("\t");
  Serial.print(aa.y);
  Serial.print("\t");
  Serial.print(aa.z);
  Serial.print("\t");
  Serial.print("mag\t");
  Serial.print(mag.x);
  Serial.print("\t");
  Serial.print(mag.y);
  Serial.print("\t");
  Serial.print(mag.z);
  Serial.println();
}


void Mpu9150::execute(){
  // wait for MPU interrupt or extra packet(s) available
  if (!mpuInterrupt && fifoCount < packetSize)
  return;

  // reset interrupt flag and get INT_STATUS byte
  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();

  // get current FIFO count
  fifoCount = mpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
    // reset so we can continue cleanly
    mpu.resetFIFO();
    Serial.println(F("FIFO overflow!"));

    // otherwise, check for DMP data ready interrupt (this should happen frequently)
  } else if (mpuIntStatus & 0x01) {
    // wait for correct available data length, should be a VERY short wait
    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

    // read a packet from FIFO
    mpu.getFIFOBytes(fifoBuffer, packetSize);

    // track FIFO count here in case there is > 1 packet available
    // (this lets us immediately read more without waiting for an interrupt)
    fifoCount -= packetSize;

    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetMag(&mag, fifoBuffer);
    mpu.dmpGetAccel(&aa, fifoBuffer);
    readingCount++;

    // get initial quaternion and gravity
    if(!initialReading) {
      zero();
      initialReading = true;
    }
  }
}

void Mpu9150::zero() {
  mpu.dmpGetAccel(&a0, fifoBuffer);
}
