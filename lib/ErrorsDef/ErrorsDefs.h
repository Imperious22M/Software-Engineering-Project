// This file contains error helper functions for the project
// Mostly C Style functions, used to display error messages on the
// matrix
#include <Adafruit_Protomatter.h>

void errorShow(const char* msg, Adafruit_Protomatter &matrix){
    matrix.fillScreen(matrix.color565(0,0,0));
    matrix.setCursor(0,0);
    matrix.println(msg);
    matrix.show();
}