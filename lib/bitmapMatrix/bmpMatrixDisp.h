/*
 This is a single header library for display a bitmap image stored
 in an SD card on a matrix display using the Adafruit Protomatter library
 and the Adafruit SDFat fork library
*/
#include <Arduino.h>
#include <sdios.h>
#include <Adafruit_Protomatter.h>
#include <SdFat.h> // Adafruit's Fork of SD
#include <Adafruit_Protomatter.h>
#include <ErrorsDefs.h> // Show the runtime errors on the matrix

// Variables for decoding the BMP image
// from the SD card and display them on the matrix
char fileBuffer[512]={};

// Struct represnting the file header information of the
// bitmap file. 
struct bmpFile{
  // Bitmap File Header
  uint16_t fileType;
  uint64_t  fileSize;
  uint32_t reserved; // 4 bytes of reserved area
  uint64_t  pixelDataOffset;
  // End File Header
  //Bitmap Info Header
  // These fields are for the BITMAPINFOHEADER type of BMP header
  // If it is a newer type, then it will ignore the rest of the fields
  uint64_t  headerSize;
  uint64_t  imageWidth;
  uint64_t  imageHeight;
  uint32_t  planes;
  uint32_t  bitsPerPixel;
  uint64_t  compression;
  uint64_t  imageSize;
  uint64_t  xPixelsPerMeter;
  uint64_t  yPixeslPerMeter;
  uint64_t  totalColors;
  uint64_t  importantColors;
  // Ignore all other fields up until headerSize-40
} bmpFile;

// Struct representing the file format for 32 bits per pixel 
// bitmap images
// NOTE: 16bpp and 32bpp images are always stored uncompressed
union bpp32Format{
  uint32_t pixelBytes;
  struct {
  // Ordering is weird due to Adafruit's
  // read function "undoing" the LE encoding 
  // of the bytes when read from the SD card
  uint8_t blue;
  uint8_t green;
  uint8_t red;
  uint8_t alpha;
  }fields;
} bpp32Format;
// Struct representing the entry in the color table of each color
// Each entry in the color table will have this
// as its the 8-bit RLE way of representing color
typedef union bpp8Format {
  uint64_t pixelBytes;
  struct {
  uint8_t blue;
  uint8_t green;
  uint8_t red;
  uint8_t alpha;

  } fields;
  
} bpp8Format;

// Class representing the image reader
// The SD card MUST be initialized before instantiating this class
class bmpImageDisp{
  private:
    SdFat32 *SDCard; // The filesystem of the SD Card
    File32 image;  // Bitmap file to open and display
    char buffer[100]; // Buffer to store messages to be printed out
    bool debugFlg  = false;

  public: 
    bmpImageDisp(SdFat32 *SDOpen, bool debugFlg_in);
    bool imageExists(char *imgPath);
    int displayImage(char *imgPath,Adafruit_Protomatter &matrix);

};

// Only valid constructor for now
bmpImageDisp::bmpImageDisp(SdFat32 *SDOpen, bool debugFlg_in){
  debugFlg = debugFlg_in;
  SDCard = SDOpen;
}

// Check if the image exists, return true if it does, otherwise false.
bool bmpImageDisp::imageExists(char *imgPath){
  if (!SDCard->exists(imgPath)) {
    return false;
  }
  return true;
}

// Reads the image passed and displays it on the protomatter matrix passed.
// Can read bitmap images that have a bit depth of 24 and 8 bits, bitfield 
// encoded for the 24 bit images and RLE encoded for the 8 bit images.
int bmpImageDisp::displayImage(char *imgPath,Adafruit_Protomatter &matrix){

  // Check if the file exists
  if(!imageExists(imgPath)){
    errorShow("BMP image does not exist",matrix);
    return 1;
  }
  
  // Open the image, exist on fail
  if(!image.open(imgPath,O_RDONLY)){
    errorShow("BMP image did not open!",matrix);
    return 1;
  }

  // The read function of adafruit's SD read implementation
  // automatically accounts for little endian formatting int he 
  // FAT32 file structure in multi-byte reads.
  // Read the image header
  image.read(&bmpFile.fileType,2); // FileType (will be backwards)
  sprintf(buffer,"\nfile Header: %x\n",bmpFile.fileType);
  image.read(&bmpFile.fileSize,4); // File Size
  sprintf(buffer,"\nfile Size: %i\n",bmpFile.fileSize);
  image.read(&bmpFile.reserved,4); // Skip the reserved area
  sprintf(buffer,"resrved area (4 bytes)\n");
  image.read(&bmpFile.pixelDataOffset,4); // Offset
  sprintf(buffer,"pixel Data Offset: %i\n",bmpFile.pixelDataOffset);
  // read the info header
  image.read(&bmpFile.headerSize,4); // Header Size
  sprintf(buffer,"header Size: %i\n",bmpFile.headerSize);
  if(bmpFile.headerSize<40){
    //cout<<"BMP File not supported, please use a BMP version";
  }
  image.read(&bmpFile.imageWidth,4); // Image Width
  sprintf(buffer,"image Width: %i\n",bmpFile.imageWidth);
  image.read(&bmpFile.imageHeight,4); // Image Height
  sprintf(buffer,"image Height: %i\n",bmpFile.imageHeight);
  image.read(&bmpFile.planes,2); // Planes
  sprintf(buffer,"planes: %i\n",bmpFile.planes);
  image.read(&bmpFile.bitsPerPixel,2); // Bits per Pixel
  sprintf(buffer,"bits Per Pixel: %i\n",bmpFile.bitsPerPixel);
  image.read(&bmpFile.compression,4); // Compression
  sprintf(buffer,"compression: %i\n",bmpFile.compression);
  image.read(&bmpFile.imageSize,4); // Image Size
  sprintf(buffer,"image Size: %i\n",bmpFile.imageSize);
  image.read(&bmpFile.xPixelsPerMeter,4); // XPixelsPerMeter
  sprintf(buffer,"xPixelsPerMeter: %i\n",bmpFile.xPixelsPerMeter);
  image.read(&bmpFile.yPixeslPerMeter,4); // YPixelsPerMeter
  sprintf(buffer,"yPixelesPerMeter: %i\n",bmpFile.yPixeslPerMeter);
  image.read(&bmpFile.totalColors,4); // Total Colors
  sprintf(buffer,"total Colors: %i\n",bmpFile.totalColors);
  image.read(&bmpFile.importantColors,4); // Important Colors
  sprintf(buffer,"important Colors: %i\n",bmpFile.importantColors);
  // Skip the rest of the header (contains bit fields and other information)
  // Start reading the pixel data until the end of the file
  matrix.setCursor(0,0); 
  if(bmpFile.bitsPerPixel==32){
    image.seek(bmpFile.pixelDataOffset);
    // Image width must be divisible by 4!
    for(int y=bmpFile.imageHeight-1;y>=0;y--){
        for(int x=0;x<bmpFile.imageWidth;x++){
          // Get the number of bytes for each pixel, this only works for 
          // bpp 32.
          int bytesRead = image.read(&bpp32Format.pixelBytes,bmpFile.bitsPerPixel/8);
          //cout << bytesRead<<"\n";
          //sprintf(buffer,"Alpha: %x red: %x green: %x blue: %x \n",bpp32Format.fields.alpha,bpp32Format.fields.red,
          //                              bpp32Format.fields.green,bpp32Format.fields.blue);
          //cout<<buffer;
          matrix.drawPixel(x,y,matrix.color565(bpp32Format.fields.red,bpp32Format.fields.green,bpp32Format.fields.blue));
        }
    }
  } else if(bmpFile.bitsPerPixel==8 && bmpFile.compression==1){
      // Implementation of 8 bit pixel depth, 8 bit rle compression
      // No field masks based on spec!
      //cout<<"implementation of rle\n";
      // Color table offset includes the size of the bitmap file headr (14 bytes)
      int colorTableOffset = bmpFile.headerSize+14; 
      //cout<<colorTableOffset<<"\n";
      // Build the color table:
      bpp8Format colorTable[bmpFile.totalColors];
      bpp8Format tableEntry;
      image.seek(colorTableOffset);
      // Read the color Table
      for(int x=0;x<bmpFile.totalColors;x++){
          // For the majority of applications each color in the table is 4 bytes long
          // Documentation is a bit confusing on this...
          // Plus the alpha channel may always be 0x00
          // but as its not used here we don't worry about it
          int bytesRead = image.read(&tableEntry.pixelBytes,4);
          colorTable[x] = tableEntry;
          //cout << bytesRead<<"\n";
          //sprintf(buffer,"Alpha: %x red: %x green: %x blue: %x \n",colorTable[x].fields.alpha,colorTable[x].fields.red,
          //                              colorTable[x].fields.green,colorTable[x].fields.blue);
          //cout<<buffer;
          //delay(10000);
      }

      // Now go the pixel portion of the, read it and display it
      image.seek(bmpFile.pixelDataOffset);
      // Decode the 8-Bit RLE compression
      // first byte is the number of times that the pixel will show
      // the second byte is the index in the table
      // 0x00 0x00 defines the end of lines and
      // 0x00 0x01 defines the end of the bitmap
      union rle8BitEntry{
        uint16_t pixelBytes;
        struct{
          uint8_t reps;
          uint8_t index;
          //uint8_t index;
          //uint8_t reps;
        } fields;
      } rle8BitEntry;
      if(!bmpFile.imageSize==0){
        
        // For now we need to make sure the max size
        // of the bitmap is no greater than the matrix 
        // size
        int x_cord = 0;
        int y_cord = bmpFile.imageHeight-1;

        // variable to exit when we are done reading the
        // matrix
        bool exitFlag = 1;
        while(exitFlag){
         if(y_cord==0){
          exitFlag;
         }
         int bytesRead = image.read(&rle8BitEntry.pixelBytes,2);
         //cout<<(int)rle8BitEntry.fields.reps<<"\n";
         //cout<<(int)rle8BitEntry.fields.index<<"\n";
         //delay(2000);
         uint8_t reps = rle8BitEntry.fields.reps;
         uint8_t index = rle8BitEntry.fields.index;
         for(int x=0;x<reps;x++){
            matrix.drawPixel(x_cord,y_cord,matrix.color565(colorTable[index].fields.red,colorTable[index].fields.green,colorTable[index].fields.blue));
            x_cord++;
            if(x_cord==bmpFile.imageWidth){
              x_cord=0;
            }
         }

         if(reps==0x00 && index==0x00){
          y_cord--;
         }
         if(reps==0x00 && index==0x01){
          exitFlag=0;
         }

        }

      }
  
  } else {
      errorShow("bpp not supported!",matrix);
  }

  image.close();
  matrix.show();
  return 0;
}

