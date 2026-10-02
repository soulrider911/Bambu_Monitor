#pragma once
#include "printer_status.h"

// The hold starts after the physical refresh, not when FINISH arrives.
struct WeactCompletion {
    PrinterStatus finished;
    String observedState;
    bool pending = false;
    bool shown = false;
    bool expired = false;
    unsigned long shownAt = 0;

    void observe(const PrinterStatus& s) {
        if (isActiveState(s.state) || s.state == "FAILED" || s.state == "ERROR") {
            pending = shown = expired = false;
        } else if (s.state == "FINISHED" && observedState != "FINISHED" && !pending && !expired) {
            finished = s;
            pending = true;
            shown = expired = false;
        }
        observedState = s.state;
        if (pending && s.state == "FINISHED") finished = s;
    }
    bool tick(unsigned long now, unsigned long interval) {
        if (pending && shown && (unsigned long)(now - shownAt) >= interval) {
            pending = false;
            expired = true;
            return true;
        }
        return false;
    }
    void displayed(unsigned long now) {
        if (pending && !shown) { shown = true; shownAt = now; }
    }
    PrinterStatus displayStatus(const PrinterStatus& live) const {
        PrinterStatus result = live;
        if (pending) {
            result.state = "FINISHED";
            result.jobName = finished.jobName;
            result.taskId = finished.taskId;
            result.filamentMaterial = finished.filamentMaterial;
            result.filamentGrams = finished.filamentGrams;
            result.filamentCount = finished.filamentCount;
        } else if (expired && live.state == "FINISHED") {
            result.state = "IDLE";
        }
        return result;
    }
};
