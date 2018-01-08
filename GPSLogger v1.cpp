SYSTEM_THREAD(ENABLED)
//SYSTEM_MODE(SEMI_AUTOMATIC);


#include <Adafruit_GPS.h>
Adafruit_GPS GPS(&Serial1);
#define GPSECHO  true                   // Leaving on until migrated/tested with Electron and it's sleep functions


// Particle Cloud Variables
    uint32_t freemem = System.freeMemory();         // Reports freemem.  
    uint32_t reportIntervalPC = 30000;              // Reporting Interval to the Particle Cloud
    uint32_t reportInfoPC = 0;                       // Flag for reporting GPS Informational Messages to the Particle Cloud
    uint32_t reportLocPC = 1;                        // Flag for reporting GPS Location Messages to the Particle Cloud
    uint32_t logStatus = 0;                         // Flag for the modules internal flash logger
    char publishBuffer[100];                        // Declare the buffer used to send Particle.publish info


void setup() {
  Serial.begin(115200);

  // Cloud Functions
    Particle.function("clearFlash", clearFlash);                        //  Clears flash logs
    Particle.function("enableFlash", enableFlash);                        //  Pass 1 to enable, Pass 0 to disable
    Particle.function("setInterval", setInterval);                    //  Set the reporting speed to the Particle Cloud
    Particle.function("writeSent", writeSent);                        //  Sends NMEA Sentence directly to GPS serial for processing
    Particle.function("setPCLog", setPCLog);                          // Configures what messages are sent to the particle cloud
  
  // Cloud Variables
    Particle.variable("freemem", freemem);                            // For keeping tracking of availible memory.  Runs at the beginning of every loop.
    Particle.variable("reportInfo", reportInfoPC);                  // Toggle informational messages
    Particle.variable("reportLoc", reportLocPC);                    // Toggle location messages
    Particle.variable("logStatus", logStatus);                      // Toggle internal flash logging

  GPS.begin(9600);

  // GPS Output Settings
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);                             // PMTK_SET_NMEA_OUTPUT_ALLDATA PMTK_SET_NMEA_OUTPUT_OFF
  // This section sets the update frequency from the GPS Unit.      
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    
    GPS.sendCommand(PMTK_API_SET_FIX_CTL_1HZ);
  // Enable auxillary location services to increase accuracy
    GPS.sendCommand(PMTK_ENABLE_SBAS);
    GPS.sendCommand(PMTK_ENABLE_WAAS);
  // Report external antenna status
    GPS.sendCommand(PGCMD_ANTENNA);  
    if (logStatus) GPS.sendCommand(PMTK_LOCUS_STARTLOG);

  useInterrupt(true);

}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
void handleSysTick(void* data) {
    char c = GPS.read();
    if (GPSECHO && c) {
        Serial.write(c);
    }
}

void useInterrupt(boolean v) {
    static HAL_InterruptCallback callback;
    static HAL_InterruptCallback previous;
    callback.handler = handleSysTick;
    HAL_Set_System_Interrupt_Handler(SysInterrupt_SysTick, &callback, &previous, nullptr);
}

//// Particle Cloud Functions and Variables

int setPCLog(String value) {
    // First value is Location Logging, Second value is Info Logging
    if(value == "1,1") {
        reportLocPC = 1;
        reportInfoPC = 1;
        Particle.publish("GPS.INFO", "PC Loc:  TRUE, PC Info: TRUE");
    } else if (value == "1,0") {
        reportLocPC = 1;
        reportInfoPC = 0;
        Particle.publish("GPS.INFO", "PC Loc:  TRUE, PC Info: FALSE");
    } else if (value == "0,1") {
        reportLocPC = 0;
        reportInfoPC = 1;
        Particle.publish("GPS.INFO", "PC Loc:  FALSE, PC Info: TRUE");
    } else if (value == "0,0") {
        reportLocPC = 0;
        reportInfoPC = 0;
        Particle.publish("GPS.INFO", "PC Loc:  FALSE, PC Info: FALSE");
    } else Particle.publish("GPS.INFO", "Error changing Particle Cloud Logging Options.  No changes made.");
  return 1;  
}

// Clear the log data being stored in flash
int clearFlash(String value) {
    GPS.sendCommand(PMTK_LOCUS_ERASE_FLASH);
    Particle.publish("GPS.INFO", "Cleared historical data stored in Flash Memory.", PRIVATE);
    return 1;
}

// Function to enable/disable logging
int enableFlash(String value) {
    if (value == "1") {
        GPS.LOCUS_StartLogger();
        logStatus = 1;
        Particle.publish("GPS.INFO", "Starting Internal Flash Logging....", PRIVATE);
        return 1;
    } 
    if (value == "0") {
        GPS.LOCUS_StopLogger();
        logStatus = 0;
        Particle.publish("GPS.INFO", "Stopping Internal Flash Logging....", PRIVATE);
        return 1;
    }
    
}

// Function to set the Particle Cloud reporting interval
int setInterval(String value) {
    reportIntervalPC = value.toInt();
    memset(&publishBuffer[0], 0, sizeof(publishBuffer));
    sprintf(publishBuffer, "Reporting time reset to %d milliseconds", reportIntervalPC);
    Particle.publish("GPS.INFO", publishBuffer, PRIVATE);
    return 1;
}

// Function to write NMEA sentence to GPS module directly
int writeSent(String value) {
    Serial1.println(value);
    return 1;
}


// Variables used for reporting and functioning logic

uint8_t lognum = GPS.LOCUS_serial;              // Current log iteration (GPS unit powercycles since last log clear)
uint8_t overwrite = GPS.LOCUS_type;             // Useless variable-  this setting is configured at the factory to stop logging when flash is full.
uint8_t distance = GPS.LOCUS_distance;          // Inaccurate value based on comparison of previous coordinates.
uint8_t used = GPS.LOCUS_percent;               // Percentage used of flash memory for logging
uint32_t records = GPS.LOCUS_records;           // Number of invidiual records
uint8_t active = GPS.LOCUS_status;              // Flash logging status
uint8_t sat = GPS.satellites;                   // Number of satellites being used for fix data
uint8_t fixquality = GPS.fixquality;            // Values 0, 1 and 2.  0 means no valid fix.  1 means "normal precision" (3D fix) 2 means "increased precision" by using supplemental services (WAAS, SBAS, etc)
float latDeg = GPS.latitudeDegrees;             // Lat in Google Maps format
float lonDeg = GPS.longitudeDegrees;            // Lon in Google Maps format
float altitude  = GPS.altitude;                 // Mostly accurate
float angle = GPS.angle;                        // Innacurrate value based on comparison of previous coordinates.  Standing drift can have a radius of about 100 feet.
float speed = GPS.speed;                        // Innacurrate value based on comparison of previous coordinates   Standing drift can have a radius of about 100 feet.
float HDOP = GPS.HDOP;                          // Horizontal Dilution of Precision  -   Lower values better (1 the best).  Theoreticlly possible to get lower than 1.



// Function to update the flash logging system status variables
void flashStatus() {
    GPS.LOCUS_ReadStatus();  //  This must be run to query the status of the internal flash logger
    lognum = GPS.LOCUS_serial;
    distance = GPS.LOCUS_distance;
    used = GPS.LOCUS_percent;
    records = GPS.LOCUS_records;
    active = GPS.LOCUS_status;
    memset(&publishBuffer[0], 0, sizeof(publishBuffer));
    sprintf(publishBuffer, "Logging: %s, Logs: %d,  Records: %d,  Percentage Used: %d,  Distance: %d", (GPS.LOCUS_status)?"TRUE":"FALSE", lognum, records, used, distance);
    Particle.publish("GPS.INFO", publishBuffer, PRIVATE);
}

// Report fix status and signal quality
void signalStatus() {
    HDOP = GPS.HDOP;
    sat = GPS.satellites;
    fixquality = GPS.fixquality;
    memset(&publishBuffer[0], 0, sizeof(publishBuffer));
    sprintf(publishBuffer, "Fix: %s,  Sat: %d, HDOP: %1.2f, Quality: %d", (GPS.fix)?"TRUE":"FALSE", sat, HDOP, fixquality);
    Particle.publish("GPS.INFO", publishBuffer, PRIVATE);
}

// Report GPS Coords, Calculated Vector and Speed
void locationStatus() {
    latDeg = GPS.latitudeDegrees;             
    lonDeg = GPS.longitudeDegrees;            
    memset(&publishBuffer[0], 0, sizeof(publishBuffer));
    sprintf(publishBuffer, "Lat: %3.5f,  Lon: %3.5f", latDeg, lonDeg);
    Particle.publish("GPS.COORDS", publishBuffer, PRIVATE);
}

// Timer Variables for main loop.  Two timers to split the Particle.publish limits
uint32_t timer = millis();

void loop() {
    // Update Particle Variables 
    freemem = System.freeMemory();

    // Update object properties with most recent data
    if(GPS.newNMEAreceived()) {
        GPS.parse(GPS.lastNMEA());    
    }

    // Reporting Timer
    if (millis() - timer > reportIntervalPC) {
        timer = millis(); // reset the timer
        // If there is a fix, publish Lat, Lon, Alt, Vector and Speed.   Lat and Lon are in Google Map's format.  Update fix value.
        if (GPS.fix) { 
            locationStatus();
        }
    }

    // Informational Messages
    if (reportInfoPC) {
        // Update LOCUS variables and publish basic status information to the Particle Cloud
        flashStatus();
         // Publish basic info regarding the status of a location fix, fix quality, and the number of satellites
        signalStatus();
    }
}