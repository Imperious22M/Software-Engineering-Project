// This is the initial implementation of Imp's LED Matrix
// on a HUB75 32x64 LED matrix using a Raspberry Pi Pico
// Requires the maxgerhardt Arduino core for this
// project to work in PlatformIO. 
// We will be using Adafruit's Protomatter library and 
// their GFX library for this project.
#include <Arduino.h>
#include <Adafruit_Protomatter.h>
#include <sdios.h>
#include <string.h>
#include <SdFat.h> // Adafruit's Fork of SD

// Error definition library
#include <ErrorsDefs.h>

// Bitmap reader and display library
#include <bmpMatrixDisp.h>

// C definitions for the LED matrix and the simulation
#define matrix_chain_width 64 // total matrix chain width (width of the array)
#define bit_depth 6 // Number of bit depth of the color plane, higher = greater color fidelity
#define address_lines_num 4 // Number of address lines of the LED matrix
#define double_buffered true // Makes animation smother if true, at the cost of twice the RAM usage
// See Adafruits documentation for more details

// Arrays for the Raspberry Pi pinouts.
// These are in GP number format, which is different from
// the silkscreen numbers, please see the picos pinout diagram.
// Adafruit has wonderful documentation on the pin layout
// of the LCD matrix.
// See: https://learn.adafruit.com/32x16-32x32-rgb-led-matrix/connecting-with-jumper-wires
// NOTE: If you are using the ribbon cable to connect, pins are mirrored relative
// to their order on the PCB in the Y axis. Please see Adafruit documentation
// for further details.
uint8_t rgbPins[]  = {0, 1, 2, 3, 4, 5}; //LED matrix: R1, G1, B1, R2, G2, B2
uint8_t addrPins[] = {6, 7, 8, 9}; // LED matrix: A,B,C,D
uint8_t clockPin   = 11; // LED matrix: CLK
uint8_t latchPin   = 12; // LED matrix: LAT
uint8_t oePin      = 13; // LED matrix: OE
uint8_t powerButton = 14; // GPIO of the power button
uint8_t modeButton = 15; // GPIO of the mode button

// GPIO 26 is unconnected and used as the seed for randomInit()

// SD card variables and instantiation
// We will be using the FAT16/FAT32 and exFAT class for higher compatibility
SdFat32  SD;         // SD card filesystem
File32 file;
File32 dir;
// File locations to be used for storing files
String animationsFilePath = "animations";
String bitmapFilePath =  "bitmaps";
String jpegsFilepath =  "jpegs";

// SD card setup and pin definitions
// The SD card is connected to the default SPI0 pins (16:?,17:CS,18:?,19:?)
const uint8_t SD_CS_PIN = 17; // For our board, this is the correct definition 
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(8)) // Can be changed up to 25 Mhz

// For details on the constructor arguments please see:
// https://learn.adafruit.com/adafruit-matrixportal-m4/protomatter-arduino-library
Adafruit_Protomatter matrix(
  matrix_chain_width, bit_depth, 1, rgbPins, 
  address_lines_num, addrPins, clockPin, latchPin, 
  oePin, double_buffered);

// Instantiate Bitmap reader class
bmpImageDisp bmpImageDisplay(&SD,false);

// Create a Serial output stream.
ArduinoOutStream cout(Serial);

// Initial setup
void setup(void) {

  Serial.begin(9600);
  pinMode(26,INPUT); // Set unconnected GPIO26 (ADC0) as input for random seed if needed
  randomSeed(analogRead(26));

  // Peripheral button initialization
  pinMode(powerButton,INPUT); 
  pinMode(modeButton,INPUT);


  // Initialize protolib (matrix control)
  ProtomatterStatus status = matrix.begin();
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if(status == PROTOMATTER_ERR_PINS) {
    while(true){
      Serial.println("RGB and clock pins are not on the same PORT!\n");
    }
  }else if(status != PROTOMATTER_OK){
    while(true){
      // If we get here, please see:
      // https://learn.adafruit.com/adafruit-matrixportal-m4/protomatter-arduino-library
      // for error state definitions and troubleshooting info
      Serial.println("Error initializing the matrix!");
    }
  }

  // Initialize the SD Card with the config defined earlier
  // If we are ever finished with the SD Card use SD.close
  if(!SD.begin(SD_CONFIG)) { 
    Serial.println(F("SD begin() failed"));
    matrix.println("SD Card Failure!");
    matrix.show();
    // See the Adafruit fork of the SD library for error definiions
    for(;;); // Fatal error, do not continue
  }

  // Any function that has color must use matrix.color(uint8_t r,g,b) call to obtain a
  //16-bit integer with the desired color, which is passed as the color argument.
  matrix.setTextSize(1);
  matrix.println("Imp's LED Matrix!"); // Default text color is white

  matrix.show();
  delay(1000);

}

// Run forever!
void loop(void) {
  SD.ls(LS_R);
  cout<<"\n";

  // Open every bitmap image in the "bitmaps" folder
  // Open bitmaps directory
  char strBuffer[100]; // buffer to store file paths
  bitmapFilePath.toCharArray(strBuffer,100);
  cout<<strBuffer<<"\n";
  if (!dir.open(strBuffer)){
    errorShow("Bitmap dir didn't open",matrix);
  }
  // Go through every bitmap and display it
  while(file.openNext(&dir,O_RDONLY)){
    // Get the file path name
    file.getName(strBuffer,100);
    String path = bitmapFilePath+"/"+strBuffer;
    if(file.isDir()){
      // We do not display directories!
      file.close();
      break;
    }

    file.close();
    path.toCharArray(strBuffer,100);
    cout<<strBuffer<<"\n";
    bmpImageDisplay.displayImage(strBuffer,matrix);
    
    delay(5000);
  }
  
  dir.close();
  delay(2000); 
}
