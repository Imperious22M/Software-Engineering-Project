// Helper matrix error display definitions
#include <ErrorsDefs.h>

// Show an error with white letter at the passed brightness [0,255]
void errorShow(const char* msg, Adafruit_Protomatter &matrix,uint8_t brightness){
    const uint8_t maxBright = 255;
    uint8_t curBright = ((float)brightness/maxBright)*maxBright;
    matrix.fillScreen(matrix.color565(0,0,0));
    matrix.setTextColor(matrix.color565(curBright,curBright,curBright));
    matrix.setCursor(0,0);
    matrix.println(msg);
    matrix.show();
}
// Show an error with white letters at close to full brigthness
void errorShow(const char* msg, Adafruit_Protomatter &matrix){
    matrix.fillScreen(matrix.color565(0,0,0));
    matrix.setTextColor(matrix.color565(200,200,200));
    matrix.setCursor(0,0);
    matrix.println(msg);
    matrix.show();
}