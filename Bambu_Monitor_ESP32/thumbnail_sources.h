#pragma once

// Explicit mappings for jobs whose printer-reported name differs from their
// stored archive. Paths refer to the printer's FTP storage, not this computer.
// Add entries here when the archive metadata also lacks the reported job name.
struct ThumbnailSource {
    const char* jobName;
    const char* archivePath;
};
static const ThumbnailSource THUMBNAIL_SOURCES[] = {
    {"1 set 4 Pieces (New version)", "/Filament_Clip.gcode.3mf"},
};
