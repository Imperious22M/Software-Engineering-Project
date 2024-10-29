
#include <ErrorsDefs.h>

void errorShow(const char* msg, Adafruit_Protomatter &matrix){
    matrix.fillScreen(matrix.color565(0,0,0));
    matrix.setCursor(0,0);
    matrix.println(msg);
    matrix.show();
}