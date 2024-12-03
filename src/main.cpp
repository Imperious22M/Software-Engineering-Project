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

// Include the wifi library and cyw43 library for running
// the wifi hardware.
#include <pico/cyw43_arch.h> // critical that we include this
#include <SPI.h>
#include <AsyncWebServer_RP2040W.h>

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
const uint8_t powerButton = 14; // GPIO of the power button
const uint8_t modeButton = 15; // GPIO of the mode button
// GPIO 26 is unconnected and used as the seed for randomInit()

// Definition of control settings for the LED matrix
String matrixId = "IMP0001"; // Unique string identifier for the matrix
uint8_t matrixBrigthness = 50; // should only be from 0 to 255 inclusive
uint8_t matrixMode = 1; // int representation of the current mode, 1:bitmap,2:animation,3:simulation


// SD card variables and instantiation
// We will be using the FAT16/FAT32 and exFAT class for higher compatibility
SdFat32 SD;         // SD card filesystem
File32 file;
File32 dir;
const uint8_t SD_CS_PIN = 17; // For the our purposes GP 17 is the correct pin
// File locations to be used for storing files
String animationsFilePath = "animations";
String bitmapFilePath =  "bitmaps";
String jpegsFilepath =  "jpegs";
// SD card setup and pin definitions
// The SD card is connected to the default SPI0 pins (16:?,17:CS,18:?,19:?)
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(8)) // Can be changed up to 25 Mhz

// Wifi variable definitions
// Sets the value of the static IP address of the pico.
IPAddress gatewayIP(192,168,128,1);
// Set AsyncServer to serve listen on port 80
AsyncWebServer    server(80);
// Set the Buffer size for files sent and read by the async
// wifi routines
#define BUFFER_SIZE         64
// Set the gateway SSID and password (hardcoded for now)
const char* gatewaySSID = "Imp's Matrix";
const char* gatewayPassword = "matrix12345";
// HTML test form for file upload, for now only support bitmaps
const char uploadPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <body>
    <h2>Upload a File</h2>
    <form method="POST" action="/bitmaps" enctype="multipart/form-data">
      <input type="file" name="file" /><br><br>
      <input type="submit" value="Upload Bitmap">
    </form>
  </body>
</html>
)rawliteral";

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

//~~~~~~~~~~ Wifi Server Helper Functions~~~~~~~~~~~~~~~~~~~~~
void printHttpHeaders(AsyncWebServerRequest *request){
    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }
}

//~~~~~~~~~~ Declaration of WiFi callback functions~~~~~~~~~~~~

// Handles when "/" is requested
void handleRoot(AsyncWebServerRequest *request)
{
	request->send(200, "text/html", uploadPage);
}

// Handles ALL the HTTP API calls for controlling the matrix
void handleAPIMatrixId(AsyncWebServerRequest *request){
    
    // print out headers
    printHttpHeaders(request);
    // Filter out GET requests (data being sent to client)
    if(request->method() == WebRequestMethod::HTTP_GET){
      char strBuff[50];
      matrixId.toCharArray(strBuff,50);
      request->send(200,"text/plain",strBuff);
    }

}

// Handles 404 errors
void handleNotFound(AsyncWebServerRequest *request)
{
	String message = "File Not Found\n\n";

	message += "URI: ";
	//message += server.uri();
	message += request->url();
	message += "\nMethod: ";
	message += (request->method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += request->args();
	message += "\n";
	for (uint8_t i = 0; i < request->args(); i++)
	{
		message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
	}
	request->send(404, "text/plain", message);
}

// Handles when "/upload" is requested
// This function is CRITICAL for file upload
void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  //Handle upload
  Serial.println("~~~~");
  Serial.println(request->url());
	Serial.println(filename);
	Serial.println(index);
	Serial.println(len);
	Serial.println(final);
	Serial.println("");
  Serial.println("~~~~");

  // Folder to store files in
  String fileFolder = "/";

  File32 file;// = SD.open("/" + filename, O_WRONLY|O_CREAT);
	//file.close();
  
  // Determine which folder the upload needs to go in
  String requestUrl = String(request->url());
  if(requestUrl.equals("/"+bitmapFilePath)){
    fileFolder.concat(bitmapFilePath);
  }
  if(requestUrl.equals("/"+animationsFilePath)){
    fileFolder.concat(animationsFilePath);
  }
  if(requestUrl.equals("/"+jpegsFilepath)){
    fileFolder.concat(jpegsFilepath);
  }
  Serial.println(fileFolder);
  Serial.println(fileFolder+"/"+filename);

	file.open((fileFolder+"/"+filename).c_str(),O_RDWR|O_CREAT);
	if(file.isOpen()){
		file.seek(index);
		file.write(data,len);
		file.close();
	}else{
		file.close();
		Serial.println("File failed to open");
		request->send(500,"text/plain","File failed to be opened");
		return;
	}

	if(final == true){
		Serial.println("File finished uploading!");
		request->send(200,"text/plain","File succesfully uploaded");
		return;
	}
}
//~~~~~~~~~~~End of WiFI callback functions~~~~~~~~~~~~~~~~~~~~



// Initial setup
void setup(void) {

  // Start the serial monitor
  Serial.begin(9600);
  
  // Set unconnected GPIO26 (ADC0) as input for random seed if needed
  pinMode(26,INPUT); 
  randomSeed(analogRead(26));

  // Peripheral button initialization as default high with 
  // internal pullup resistor
  pinMode(powerButton,INPUT_PULLUP); 
  pinMode(modeButton,INPUT_PULLUP);

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
    matrix.println("SD Card Failure!");
    matrix.show();
    while(true){
      Serial.println(F("SD begin() failed"));
      delay(1000);
    // See the Adafruit fork of the SD library for error definiions
    }
  }

  // Any function that has color must use matrix.color(uint8_t r,g,b) call to obtain a
  //16-bit integer with the desired color, which is passed as the color argument.
  matrix.setTextSize(1);
  matrix.println("Imp's LED Matrix!"); // Default text color is white
  matrix.show();

  // Initialiaze the pico as a soft wifi access point
  // and set a static IP for the access point
  WiFi.mode(WIFI_AP);
  // Set the gateway (pico W) IP address
  // to the static IP specified by getwayIP (192.168.128.1)
  WiFi.config(gatewayIP);
  WiFi.begin(gatewaySSID,gatewayPassword);

  // Set WiFi server root ("/") callback
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
	{
		handleRoot(request);
	});

	// Set WiFi server "/upload" callback
  // This is the most important callback 
	server.on("/bitmaps", HTTP_POST, [](AsyncWebServerRequest *request){
	  request->send(200);
	 }, onUpload);

  // Set all HTTP URL API callbacks 
  server.on("/API/id", HTTP_GET,handleAPIMatrixId);
  //server.on("/API", HTTP_PUT,handleAPIRequests);
  //server.on("/API", HTTP_GET,handleAPIRequests);

  // Set Wifi server default handler if request address is not found
	server.onNotFound(handleNotFound);

  // Start listening for connections
	server.begin();

}

// Run forever!
void loop(void) {

  // FYI the WiFi server routines are run in the background
  // no need to poll them here
  // Same goes for the LED matrix image displaying (protomatter)
  // routines

  SD.ls(LS_R);
  cout<<"\n";

  // Open every bitmap image in the "bitmaps" folder
  // Open bitmaps directory
  char strBuffer[100]; // buffer to store file paths
  bitmapFilePath.toCharArray(strBuffer,100);
  //cout<<strBuffer<<"\n";
  if (!dir.open(strBuffer)){
    errorShow("Bitmap dir didn't open",matrix);
    dir.close(); // close in case there is a floating file resource
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
    //cout<<strBuffer<<"\n";
    bmpImageDisplay.displayImage(strBuffer,matrix,matrixBrigthness);
    
    delay(5000);
  }
  
  dir.close();

}
