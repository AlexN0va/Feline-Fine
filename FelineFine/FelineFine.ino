#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <LiquidCrystal_I2C.h>


MAX30105 particleSensor;


#define debug Serial


const int numReadings = 10;  // Number of readings to average
long irReadings[numReadings]; // Array to hold IR readings
long redReadings[numReadings]; // Array to hold Red readings
int readIndex = 0;  // Current index
long totalIR = 0;  // Running total
long totalRed = 0; // Running total


long lastBeatTime = 0; // Time of the last beat
const int beatThreshold = 20000; // Adjust threshold for peak detection
const int noiseThreshold = 5000; // Threshold to check for valid finger placement


bool placeFingerOnScreen = false;


LiquidCrystal_I2C lcd(0x27, 16, 2);// set up the LCD screen object


void setup() {
  debug.begin(115200);
  debug.println("Initializing...");


  // Initialize the sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    debug.println("MAX30105 not found. Please check wiring/power.");
    while (1); // Stop if sensor not found
  }


  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); // Turn on the Red LED for SpO2 calculations
  particleSensor.setPulseAmplitudeGreen(0); // Turn off Green LED (not needed)


  // Initialize arrays with zeroes
  for (int i = 0; i < numReadings; i++) {
    irReadings[i] = 0;
    redReadings[i] = 0;
  }


  lcd.init();            // Initialize the LCD
  lcd.backlight();       // Turn on the backlight
 
 
 /*  
  lcd.setCursor(5, 0);   // Set cursor to the first column, first row
  lcd.print("Team:");        // Print a single character to the LCD
  lcd.setCursor(3, 1);
  lcd.print("MediCAL!!!");
*/




  debug.println("Sensor initialized successfully.");
}


void loop() {
  // Subtract the last reading
  totalIR -= irReadings[readIndex];
  totalRed -= redReadings[readIndex];


  // Read from the sensor
  irReadings[readIndex] = particleSensor.getIR();
  redReadings[readIndex] = particleSensor.getRed();


  // Add the reading to the total
  totalIR += irReadings[readIndex];
  totalRed += redReadings[readIndex];


  // Advance to the next position in the array
  readIndex = (readIndex + 1) % numReadings; // Wrap around using modulo operator


  // Calculate the average
  long averageIR = totalIR / numReadings;
  long averageRed = totalRed / numReadings;


  // Print IR and Red values for debugging
  debug.print("IR Value: ");
  debug.print(averageIR);
  debug.print(" Red Value: ");
  debug.println(averageRed);


  // Check if finger is properly placed
  if (averageIR > noiseThreshold && averageRed > noiseThreshold) {
    // Detect beats
    long currentTime = millis();
    bool beatDetected = false;


    if (averageIR > beatThreshold) {
      if (currentTime - lastBeatTime > 500) { // Minimum time between beats (500 ms)
        beatDetected = true;
        long lastBeatInterval = currentTime - lastBeatTime;
        lastBeatTime = currentTime;


        debug.print("Beat detected at: ");
        debug.println(currentTime);


        if (lastBeatInterval > 0) {
          float heartRate = 60000.0 / lastBeatInterval; // Convert time from ms to minutes
          debug.print("Heart Rate: ");
          debug.print(heartRate);
          debug.println(" BPM");


          lcd.setCursor(1, 0);
          lcd.print("BPM:");
          lcd.setCursor(1, 1);
          lcd.print(heartRate);
          lcd.print("BPM");


        } else {
          debug.println("No heart rate data");


          placeFingerOnScreen = false;
        }
      }
    }


    // Calculate SpO2
    if (averageIR > 0) { // Avoid division by zero
      float ratio = (float)averageRed / averageIR;
      float spo2 = 104 - 17 * ratio;


      if (placeFingerOnScreen){
        lcd.clear();
      }


      placeFingerOnScreen = false;


      debug.print("SpO2: ");
      debug.print(spo2);
      debug.println(" %");


      lcd.setCursor(10, 0);
      lcd.print("SpO2:");
      lcd.setCursor(10, 1);
      lcd.print(spo2);
      lcd.print("%");


     
    } else {
      debug.println("SpO2: Invalid (IR Value is zero)");
    }
  } else {
    debug.println("Finger not detected properly. Please place your finger on the sensor.");


    if (!(placeFingerOnScreen)){
      lcd.clear();
    }


    placeFingerOnScreen = true;
 
    lcd.setCursor(0, 0);
    lcd.print("Place Finger !!!");
    lcd.setCursor(5,1);
    lcd.print("Please");
  }


  // Add a delay for readability and reduce noise
  delay(1000);
}
