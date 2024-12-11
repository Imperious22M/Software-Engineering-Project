// Contains the class that is responsable of managing the settings file in the matrix
#include <Arduino.h>
#include <SdFat.h> // Adafruit's Fork of SD

// Provides support for reading and mantaining the settings files
// Functions should only be called after the SD card has been initialiazed
class settingsManager{
    private:
        String rootDir = ""; // Default is root dir
        String settingsFilename = "settings.txt";
        String settings = "";
        File32 settingsFile;
    public:
        int createSettingsFile(String name, String dir);
        int saveArray();
        int getArrayIndexVal(int index);
    private:
        int saveSettingsFile();

};

// Create the saving file in the SD Card
int settingsManager::createSettingsFile(String name, String dir){
   rootDir = dir;
   settingsFilename = name;
    char buff[50];
    String fullPath = rootDir+"/"+settingsFilename;
    Serial.println(fullPath);
    fullPath.toCharArray(buff,50);
    if(!settingsFile.exists(buff)){
        Serial.println("File created");
        settingsFile.open(buff,O_WRITE|O_CREAT);
        settingsFile.close();
    }
    return 0;
}

// Save all the settings to the SD Card file
int settingsManager::saveSettingsFile(){
    
    // TODO: Implement save functionality!

    return 0;
}

// The settings in the card are treated as an array, this
// saves the array in the correct format to the SD Card file
// TODO: Finish implementation
int settingsManager::saveArray(){
    char buff[50];
    String fullPath = rootDir+"/"+settingsFilename;
    Serial.println(fullPath);
    fullPath.toCharArray(buff,50);
    // Try to open the settings file if it exists
    if(!settingsFile.open(buff,O_RDWR)){       
        // Brightness was not saved
        Serial.println("Settings file failed to be opened");
        settingsFile.close();
        return 1;
    }
    return 0;
}

// Get's a value setting based on the index of the array it
// is stored in
// TODO: Finish implementation
int settingsManager::getArrayIndexVal(int val){
    char buff[50];
    String fullPath = rootDir+"/"+settingsFilename;
    Serial.println(fullPath);
    fullPath.toCharArray(buff,50);
    // Try to open the settings file if it exists
    if(!settingsFile.open(buff,O_RDWR)){       
        // Brightness was not saved
        Serial.println("Settings file failed to be opened");
        settingsFile.close();
        return 1;
    }
    while(settingsFile.available()){
        Serial.print((char)settingsFile.read());
    }
    
    Serial.println();
    settingsFile.close();
    return 0;
}