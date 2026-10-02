#pragma once
#include <Arduino.h>

struct PrinterStatus {
    String state = "CONNECTING";
    int stage = -1;            // stg_cur: what the printer is doing within the job
    String printerName = "";   // e.g. "Bambu Lab X2D", from the printer's get_version reply
    String jobName = "";
    String taskId = "";
    String gcodeFile = "";

    String filamentMaterial = "";
    float filamentGrams = -1;
    unsigned filamentCount = 0;

    int progress = -1;
    int layer = -1;
    int totalLayers = -1;
    int remainingMinutes = -1;

    float chamberTemp = NAN;
    float bedTemp = NAN;
    float leftNozzleTemp = NAN;
    float rightNozzleTemp = NAN;

    float amsTemp = NAN;
    int amsHumidity = -1;
    int amsHumidityRaw = -1;

    String printerWifi = "";
    unsigned long lastMessage = 0;
};


inline bool isActiveState(const String& state) {
    return state == "PRINTING" || state == "PREPARING" || state == "PAUSED";
}

