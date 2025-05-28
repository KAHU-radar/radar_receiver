/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register              bdbcat@yahoo.com *
 *   Copyright (C) 2012-2013 by Dave Cowell                                *
 *   Copyright (C) 2012-2016 by Kees Verruijt         canboat@verruijt.net *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

#ifndef _RADARPI_H_
#define _RADARPI_H_

#define MY_API_VERSION_MAJOR 1
#define MY_API_VERSION_MINOR 16 // Needed for PluginAISDrawGL().

#include <algorithm>
#include <vector>

#include "RadarControlItem.h"
#include "RadarLocationInfo.h"
#include "config.h"
#include "drawutil.h"
/*
#include "nmea0183.h"
#include "nmea0183.hpp"
*/
#include "pi_common.h"
#include "../deps/radar_pi/include/raymarine/RaymarineLocate.h"
#include "socketutil.h"
//#include "wx/jsonreader.h"

// Load the ocpn_plugin. On OS X this generates many warnings, suppress these.
#ifdef __WXOSX__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif
//#include "ocpn_plugin.h"
#ifdef __WXOSX__
#pragma clang diagnostic pop
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))

PLUGIN_BEGIN_NAMESPACE
//using ::NMEA0183;

// Define the following to make sure we have no race conditions during thread
// stop. #define TEST_THREAD_RACES

//    Forward definitions
class GuardZone;
class RadarInfo;

class ControlsDialog;
//class MessageBox;
class OptionsDialog;
class RadarReceive;
class RadarControl;
class radar_pi;
//class GuardZoneBogey;
class Arpa;
class GPSKalmanFilter;
class RaymarineLocate;
class NavicoLocate;

#define MAX_CHART_CANVAS (2) // How many canvases OpenCPN supports
#define RADARS                                                                 \
    (4) // Arbitrary limit, anyone running this many is already crazy!
#define GUARD_ZONES (2) // Could be increased if wanted
#define BEARING_LINES (2) // And these as well
#define NO_TRANSMIT_ZONES                                                      \
    (4) // Max that any radar supports, currently xHD=1 HALO=4

#define CANVAS_COUNT (1) // wxMin(MAX_CHART_CANVAS, GetCanvasCount()))

static const int SECONDS_PER_TIMED_IDLE_SETTING
    = 60; // Can't change this anymore, has to be same as Garmin hardware
static const int SECONDS_PER_TIMED_RUN_SETTING = 60;

#define OPENGL_ROTATION (-90.0) // Difference between 'up' and OpenGL 'up'...

typedef int SpokeBearing; // A value from 0 -- LINES_PER_ROTATION indicating a
                          // bearing (? = North, +ve = clockwise)

typedef int AngleDegrees; // An angle relative to North or HeadUp. Generally
                          // [0..359> or [-180,180]

// Use the above to convert from 'raw' headings sent by the radar (0..4095) into
// classical degrees (0..359) and back

// OLD NAVICO
// #define SCALE_RAW_TO_DEGREES2048(raw) ((raw) * (double)DEGREES_PER_ROTATION /
// LINES_PER_ROTATION) #define SCALE_DEGREES_TO_RAW2048(angle) ((int)((angle) *
//(double)LINES_PER_ROTATION / DEGREES_PER_ROTATION)) #define MOD_ROTATION(raw)
//(((raw) + 2 * SPOKES) % SPOKES)

// NEW GENERIC
#define SCALE_DEGREES_TO_SPOKES(angle)                                         \
    ((angle) * (m_ri->m_spokes) / DEGREES_PER_ROTATION)
#define SCALE_SPOKES_TO_DEGREES(raw)                                           \
    ((raw) * (double)DEGREES_PER_ROTATION / m_ri->m_spokes)
#define MOD_SPOKES(raw) (((raw) + 2 * m_ri->m_spokes) % m_ri->m_spokes)
#define MOD_DEGREES(angle)                                                     \
    (((angle) + 2 * DEGREES_PER_ROTATION) % DEGREES_PER_ROTATION)
#define MOD_DEGREES_FLOAT(angle)                                               \
    (fmod((double)(angle) + 2 * DEGREES_PER_ROTATION, DEGREES_PER_ROTATION))
#define MOD_DEGREES_180(angle) (((int)(angle) + 900) % 360 - 180)

#define WATCHDOG_TIMEOUT                                                       \
    (10) // After 10s assume GPS and heading data is invalid

#define TIMED_OUT(t, timeout) (t >= timeout)
#define NOT_TIMED_OUT(t, timeout) (!TIMED_OUT(t, timeout))

#define VALID_GEO(x) (!isnan(x) && x >= -360.0 && x <= +360.0)

#ifndef M_SETTINGS
#define M_SETTINGS m_pi->m_settings
#define M_PLUGIN m_pi->
#else
#define M_PLUGIN
#endif

extern int g_verbose;
#define LOGLEVEL_INFO 0
#define LOGLEVEL_VERBOSE 1
#define LOGLEVEL_DIALOG 2
#define LOGLEVEL_TRANSMIT 4
#define LOGLEVEL_RECEIVE 8
#define LOGLEVEL_GUARD 16
#define LOGLEVEL_ARPA 32
#define LOGLEVEL_REPORTS 64
#define LOGLEVEL_INTER 128
#define IF_LOG_AT_LEVEL(x) if ((g_verbose & (x)) != 0)

#define IF_LOG_AT(x, y)                                                        \
    do {                                                                       \
        IF_LOG_AT_LEVEL(x) { y; }                                              \
    } while (0)
#define LOG_INFO wxLogMessage
#define LOG_VERBOSE IF_LOG_AT_LEVEL(LOGLEVEL_VERBOSE) wxLogMessage
#define LOG_DIALOG IF_LOG_AT_LEVEL(LOGLEVEL_DIALOG) wxLogMessage
#define LOG_TRANSMIT IF_LOG_AT_LEVEL(LOGLEVEL_TRANSMIT) wxLogMessage
#define LOG_RECEIVE IF_LOG_AT_LEVEL(LOGLEVEL_RECEIVE) wxLogMessage
#define LOG_GUARD IF_LOG_AT_LEVEL(LOGLEVEL_GUARD) wxLogMessage
#define LOG_ARPA IF_LOG_AT_LEVEL(LOGLEVEL_ARPA) wxLogMessage
#define LOG_REPORTS IF_LOG_AT_LEVEL(LOGLEVEL_REPORTS) wxLogMessage
#define LOG_INTER IF_LOG_AT_LEVEL(LOGLEVEL_INTER) wxLogMessage
#define LOG_BINARY_VERBOSE(what, data, size)                                   \
    IF_LOG_AT_LEVEL(LOGLEVEL_VERBOSE)                                          \
    {                                                                          \
        M_PLUGIN logBinaryData(what, data, size);                              \
    }
#define LOG_BINARY_DIALOG(what, data, size)                                    \
    IF_LOG_AT_LEVEL(LOGLEVEL_DIALOG)                                           \
    {                                                                          \
        M_PLUGIN logBinaryData(what, data, size);                              \
    }
#define LOG_BINARY_TRANSMIT(what, data, size)                                  \
    IF_LOG_AT_LEVEL(LOGLEVEL_TRANSMIT)                                         \
    {                                                                          \
        M_PLUGIN logBinaryData(what, data, size);                              \
    }
#define LOG_BINARY_RECEIVE(what, data, size)                                   \
    IF_LOG_AT_LEVEL(LOGLEVEL_RECEIVE)                                          \
    {                                                                          \
        M_PLUGIN logBinaryData(what, data, size);                              \
    }
#define LOG_BINARY_GUARD(what, data, size)                                     \
    IF_LOG_AT_LEVEL(LOGLEVEL_GUARD)                                            \
    {                                                                          \
        M_PLUGIN logBinaryData(what, data, size);                              \
    }
#define LOG_BINARY_ARPA(what, data, size)                                      \
    IF_LOG_AT_LEVEL(LOGLEVEL_ARPA) { M_PLUGIN logBinaryData(what, data, size); }
#define LOG_BINARY_REPORTS(what, data, size)                                   \
    IF_LOG_AT_LEVEL(LOGLEVEL_REPORTS)                                          \
    {                                                                          \
        M_PLUGIN logBinaryData(what, data, size);                              \
    }
#define LOG_BINARY_INTER(what, data, size)                                     \
    IF_LOG_AT_LEVEL(LOGLEVEL_INTER)                                            \
    {                                                                          \
        M_PLUGIN logBinaryData(what, data, size);                              \
    }

enum {
    BM_ID_RED,
    BM_ID_RED_SLAVE,
    BM_ID_GREEN,
    BM_ID_GREEN_SLAVE,
    BM_ID_AMBER,
    BM_ID_AMBER_SLAVE,
    BM_ID_BLANK,
    BM_ID_BLANK_SLAVE
};

// Arranged from low to high priority:
enum HeadingSource {
    HEADING_NONE,
    HEADING_FIX_COG,
    HEADING_FIX_HDM,
    HEADING_FIX_HDT,
    HEADING_FIXED,
    HEADING_NMEA_HDM,
    HEADING_NMEA_HDT,
    HEADING_RADAR_HDM,
    HEADING_RADAR_HDT
};

enum ToolbarIconColor {
    TB_NONE,
    TB_HIDDEN,
    TB_SEARCHING,
    TB_SEEN,
    TB_STANDBY,
    TB_ACTIVE
};

//
// The order of these is used for deciding what the 'best' representation is
// of the radar icon in the OpenCPN toolbar. The 'highest' (last) in the enum
// across all radar states is what is shown.
// If you add a RadarState also add an entry to g_toolbarIconColor...
//
enum RadarState {
    RADAR_OFF,
    RADAR_STANDBY,
    RADAR_WARMING_UP,
    RADAR_TIMED_IDLE,
    RADAR_STOPPING,
    RADAR_SPINNING_DOWN,
    RADAR_STARTING,
    RADAR_SPINNING_UP,
    RADAR_TRANSMIT
};

#ifdef RADAR_PI_GLOBALS
static ToolbarIconColor g_toolbarIconColor[9] = { TB_SEARCHING, TB_STANDBY,
    TB_SEEN, TB_SEEN, TB_SEEN, TB_SEEN, TB_ACTIVE, TB_ACTIVE, TB_ACTIVE };
#endif

typedef enum ModeType {
    MODE_CUSTOM,
    MODE_HARBOR,
    MODE_OFFSHORE,
    MODE_UNUSED,
    MODE_WEATHER,
    MODE_BIRD
} ModeType;

struct receive_statistics {
    int packets;
    int broken_packets;
    int spokes;
    int broken_spokes;
    int missing_spokes;
};

typedef enum GuardZoneType { GZ_ARC, GZ_CIRCLE } GuardZoneType;

typedef enum RadarType {
#define DEFINE_RADAR(t, n, s, l, a, b, c, d) t,
#include "RadarType.h"
    RT_MAX
} RadarType;

const size_t RadarSpokes[RT_MAX] = {
#define DEFINE_RADAR(t, n, s, l, a, b, c, d) s,
#include "RadarType.h"
};

const size_t RadarSpokeLenMax[RT_MAX] = {
#define DEFINE_RADAR(t, n, s, l, a, b, c, d) l,
#include "RadarType.h"
};

const static int RadarOrder[RT_MAX] = {
#define DEFINE_RADAR(t, x, s, l, a, b, c, d) d,
#include "RadarType.h"
};

extern const wchar_t* RadarTypeName[RT_MAX];

typedef enum ControlType {
    CT_NONE,
#define CONTROL_TYPE(x, y) x,
#include "ControlType.inc"
#undef CONTROL_TYPE
    CT_MAX
} ControlType;

// We used to use wxColour(), but its implementation is surprisingly
// complicated in some ports of wxWidgets, in particular wxMAC, so
// use our own BareBones version. This has a surprising effect on
// performance on those ports!

class PixelColour {
public:
    PixelColour()
    {
        red = 0;
        green = 0;
        blue = 0;
    }

    PixelColour(uint8_t r, uint8_t g, uint8_t b)
    {
        red = r;
        green = g;
        blue = b;
    }

    uint8_t Red() const { return red; }

    uint8_t Green() const { return green; }

    uint8_t Blue() const { return blue; }

    PixelColour operator=(const PixelColour& other)
    {
        if (this != &other) {
            this->red = other.red;
            this->green = other.green;
            this->blue = other.blue;
        }
        return *this;
    }

    PixelColour operator=(const wxColour& other)
    {
        this->red = other.Red();
        this->green = other.Green();
        this->blue = other.Blue();

        return *this;
    }

private:
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

enum BlobColour {
    BLOB_NONE,
    BLOB_HISTORY_0,
    BLOB_HISTORY_1,
    BLOB_HISTORY_2,
    BLOB_HISTORY_3,
    BLOB_HISTORY_4,
    BLOB_HISTORY_5,
    BLOB_HISTORY_6,
    BLOB_HISTORY_7,
    BLOB_HISTORY_8,
    BLOB_HISTORY_9,
    BLOB_HISTORY_10,
    BLOB_HISTORY_11,
    BLOB_HISTORY_12,
    BLOB_HISTORY_13,
    BLOB_HISTORY_14,
    BLOB_HISTORY_15,
    BLOB_HISTORY_16,
    BLOB_HISTORY_17,
    BLOB_HISTORY_18,
    BLOB_HISTORY_19,
    BLOB_HISTORY_20,
    BLOB_HISTORY_21,
    BLOB_HISTORY_22,
    BLOB_HISTORY_23,
    BLOB_HISTORY_24,
    BLOB_HISTORY_25,
    BLOB_HISTORY_26,
    BLOB_HISTORY_27,
    BLOB_HISTORY_28,
    BLOB_HISTORY_29,
    BLOB_HISTORY_30,
    BLOB_HISTORY_31,
    BLOB_WEAK,
    BLOB_INTERMEDIATE,
    BLOB_STRONG,
    BLOB_DOPPLER_RECEDING,
    BLOB_DOPPLER_APPROACHING
};
#define BLOB_HISTORY_MAX BLOB_HISTORY_31
#define BLOB_HISTORY_COLOURS (BLOB_HISTORY_MAX - BLOB_NONE)
#define BLOB_COLOURS (BLOB_DOPPLER_APPROACHING + 1)

extern const char* convertRadarToString(int range_meters, int units, int index);
extern double local_distance(GeoPosition pos1, GeoPosition pos2);
extern double local_bearing(GeoPosition pos1, GeoPosition pos2);
extern GeoPosition local_position(
    GeoPosition& pos, double distance, double bearing);

enum DisplayModeType { DM_CHART_OVERLAY, DM_CHART_NONE };
enum VariationSource {
    VARIATION_SOURCE_NONE,
    VARIATION_SOURCE_NMEA,
    VARIATION_SOURCE_FIX,
    VARIATION_SOURCE_WMM
};
enum OpenGLMode { OPENGL_UNKOWN, OPENGL_OFF, OPENGL_ON };

static const bool HasBitCount2[8] = {
    false, // 000
    false, // 001
    false, // 010
    true, // 011
    false, // 100
    true, // 101
    true, // 110
    true, // 111
};

#define DEFAULT_OVERLAY_TRANSPARENCY (5)
#define MIN_OVERLAY_TRANSPARENCY (0)
#define MAX_OVERLAY_TRANSPARENCY (90)
#define MIN_AGE (4)
#define MAX_AGE (12)

enum RangeUnits {
    RANGE_MIXED,
    RANGE_METRIC,
    RANGE_NAUTIC,
    RANGE_UNITS_UNDEFINED
};
static const int RangeUnitsToMeters[3] = { 1852, 1000, 1852 };
static const wxString RangeUnitDescriptions[3]
    = { wxT("kn"), wxT("kph"), wxT("kn") };

/**
 * The data that is stored in the opencpn.ini file. Most of this is set in the
 * OptionsDialog, some of it is 'secret' and can only be set by manipulating the
 * ini file directly.
 */
struct PersistentSettings {
    size_t radar_count; // How many radars we have
    RadarControlItem overlay_transparency; // How transparent is the radar
                                           // picture over the chart
    int range_index; // index into range array, see RadarInfo.cpp
    int verbose; // Loglevel 0..4.
    int guard_zone_threshold; // How many blobs must be sent by radar before we
                              // fire alarm
    int guard_zone_render_style; // 0 = Shading, 1 = Outline, 2 = Shading +
                                 // Outline
    int guard_zone_timeout; // How long before we warn again when bogeys are
                            // found
    bool guard_zone_on_overlay; // Show the guard zone on chart overlay?
    bool trails_on_overlay; // Show radar trails on chart overlay?
    bool overlay_on_standby; // Show guard zone when radar is in standby?
    int guard_zone_debug_inc; // Value to add on every cycle to guard zone
                              // bearings, for testing.
    double skew_factor; // Set to -1 or other value to correct skewing
    RangeUnits range_units; // See enum
    int max_age; // Scans older than this in seconds will be removed
    RadarControlItem refreshrate; // How quickly to refresh the display
    int menu_auto_hide; // 0 = none, 1 = 10s, 2 = 30s
    int drawing_method; // VertexBuffer, Shader, etc.
    bool developer_mode; // Readonly from config, allows head up mode
    bool show; // whether to show any radar (overlay or window)
    bool show_radar[RADARS]; // whether to show radar window
    bool dock_radar[RADARS]; // whether to dock radar window
    bool show_radar_control[RADARS]; // whether to show radar menu (control)
                                     // window
    int dock_size; // size of the docked radar
    bool transmit_radar[RADARS]; // whether radar should be transmitting
                                 // (persistent)
    bool pass_heading_to_opencpn; // Pass heading coming from radar as NMEA data
                                  // to OpenCPN
    bool enable_cog_heading; // Allow COG as heading. Should be taken out back
                             // and shot.
    bool ignore_radar_heading; // For testing purposes
    bool reverse_zoom; // false = normal, true = reverse
    bool show_extreme_range; // Show red ring at extreme range and center
    bool reset_radars; // True on exit of OptionsDialog when reset of radars is
                       // pressed
    int threshold_red; // Radar data has to be this strong to show as STRONG
    int threshold_green; // Radar data has to be this strong to show as
                         // INTERMEDIATE
    int threshold_blue; // Radar data has to be this strong to show as WEAK
    int threshold_multi_sweep; // Radar data has to be this strong not to be
                               // ignored in multisweep
    int threshold; // Radar pixels below this value are ignored (0..100% in
                   // steps of 10%)
    int type_detection_method; // 0 = default, 1 = ignore reports
    int AISatARPAoffset; // Rectangle side where to search AIS targets at ARPA
                         // position
    wxPoint control_pos[RADARS]; // Saved position of control menu windows
    wxPoint window_pos[RADARS]; // Saved position of radar windows, when
                                // floating and not docked
    wxPoint alarm_pos; // Saved position of alarm window
    wxString alert_audio_file; // Filepath of alarm audio file. Must be WAV.
    wxColour trail_start_colour; // Starting colour of a trail
    wxColour trail_end_colour; // Ending colour of a trail
    wxColour
        doppler_approaching_colour; // Colour for Doppler Approaching returns
    wxColour doppler_receding_colour; // Colour for Doppler Receding returns
    wxColour strong_colour; // Colour for STRONG returns
    wxColour intermediate_colour; // Colour for INTERMEDIATE returns
    wxColour weak_colour; // Colour for WEAK returns
    wxColour arpa_colour; // Colour for ARPA edges
    wxColour ais_text_colour; // Colour for AIS texts
    wxColour
        ppi_background_colour; // Colour for PPI background (normally very dark)
    double fixed_heading_value;
    bool fixed_heading;
    bool pos_is_fixed;
    GeoPosition fixed_pos;
    wxString radar_description_text;
    NetworkAddress target_mixer_address;
};

// Table for AIS targets inside ARPA zone
struct AisArpa {
    long ais_mmsi;
    time_t ais_time_upd;
    double ais_lat;
    double ais_lon;

    AisArpa()
        : ais_mmsi(0)
        , ais_time_upd()
        , ais_lat()
        , ais_lon()
    {
    }
};

//----------------------------------------------------------------------------------------------------------
//    The PlugIn Class Definition
//----------------------------------------------------------------------------------------------------------

#define RADAR_TOOL_POSITION -1 // Request default positioning of toolbar tool

#define PLUGIN_OPTIONS                                                         \
    (WANTS_DYNAMIC_OPENGL_OVERLAY_CALLBACK | WANTS_OPENGL_OVERLAY_CALLBACK     \
        | WANTS_OVERLAY_CALLBACK | WANTS_TOOLBAR_CALLBACK                      \
        | INSTALLS_TOOLBAR_TOOL | USES_AUI_MANAGER | WANTS_CONFIG              \
        | WANTS_NMEA_EVENTS | WANTS_NMEA_SENTENCES | WANTS_PREFERENCES         \
        | WANTS_PLUGIN_MESSAGING | WANTS_CURSOR_LATLON | WANTS_MOUSE_EVENTS    \
        | INSTALLS_CONTEXTMENU_ITEMS)

#include <functional>

typedef std::function<void(
           int target_id,           // 1 target id
           double distance,         // 2 Targ distance
           double bearing,          // 3 Bearing fr own ship.
           std::string bear_unit,   // 4 Brearing unit
           double speed,            // 5 Target speed in knots
           double course,           // 6 Target Course.
           std::string course_unit, // 7 Course ref T // 8 CPA Not used // 9 TCPA Not used
           std::string dist_unit,   // 10 S/D Unit N = knots/Nm or 
           std::string target_name, // 11 Target name
           std::string status       // 12 Target Status L/Q/T // 13 Ref N/A
)> ProcessArpaTargetFN;

class radar_pi /*: public opencpn_plugin_116, public wxEvtHandler */ {
public:
    radar_pi(void* ppimgr);
    ~radar_pi();
    // void PrepareRadarImage(int angle); remove?

    //    The required PlugIn Methods
    int Init(void);
    bool DeInit(void);
    void logBinaryData(const wxString& what, const uint8_t* data, int size);
    void StartRadarLocators(size_t r);
    void StopRadarLocators();

    void UpdateAllControlStates(bool all);
    bool LoadConfig();
    bool SaveConfig();
    void SetRadarHeading(double heading = nan(""), bool isTrue = false);
    double GetHeadingTrue()
    {
        wxCriticalSectionLocker lock(m_exclusive);
        return m_hdt;
    }
    double GetCOG()
    {
        wxCriticalSectionLocker lock(m_exclusive);
        return m_cog;
    }
    HeadingSource GetHeadingSource() { return m_heading_source; }
    bool IsBoatPositionValid()
    {
        wxCriticalSectionLocker lock(m_exclusive);
        return m_bpos_set;
    }
    wxLongLong GetBootMillis() { return m_boot_time; }
    bool m_guard_bogey_confirmed;
    bool m_guard_bogey_seen; // Saw guardzone bogeys on last check
    PersistentSettings m_settings;
    RadarInfo* m_radar[RADARS];
    NavicoLocate* m_navico_locator;
    RaymarineLocate* m_raymarine_locator;
    void UpdateState(void);
    void TimedControlUpdate();
    void TimedUpdate();
    wxCriticalSection
        m_exclusive; // protects callbacks that come from multiple radars

    double m_hdt; // this is the heading that the pi is using for all heading
                  // operations, in degrees. m_hdt will come from the radar if
                  // available else from the NMEA stream.
    time_t m_hdt_timeout; // When we consider heading is lost
    double m_hdm; // Last magnetic heading obtained
    time_t m_hdm_timeout; // When we consider heading is lost
public:
    HeadingSource m_heading_source;
    bool m_bpos_set;

    // Variation. Used to convert magnetic into true heading.
    // Can come from SetPositionFixEx, which may hail from the WMM plugin
    // and is thus to be preferred, or GPS or a NMEA sentence. The latter will
    // probably have an outdated variation model, so is less preferred. Besides,
    // some devices transmit invalid (zero) values. So we also let non-zero
    // values prevail.
    double m_var; // local magnetic variation, in degrees
    VariationSource m_var_source;

    wxFileConfig* m_pconfig;
#define HEADING_TIMEOUT (5)
    double m_cog; // Value of m_COGAvg at rotation time
    double m_vp_rotation; // Last seen vp->rotation
    GeoPosition m_ownship;
    bool m_initialized; // True if Init() succeeded and DeInit() not called yet.
    bool m_first_init; // True in first Init() call.
    wxLongLong m_boot_time; // millis when started
    ProcessArpaTargetFN* process_arpa_target_fn;
    void SetProcessArpaTargetFN(ProcessArpaTargetFN* fn) { process_arpa_target_fn = fn; };
    int m_frame_counter;
};

PLUGIN_END_NAMESPACE

#endif /* _RADAR_PI_H_ */
