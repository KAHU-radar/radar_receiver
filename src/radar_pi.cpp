/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
 *           Sean D'Epagnier
 *           Andrei Bankovs: Raymarine radars
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

#define RADAR_PI_GLOBALS

#include "radar_pi.h"

#include "Arpa.h"
#include "GuardZone.h"
#include "RadarInfo.h"
#include "navico/NavicoLocate.h"
#include "raymarine/RaymarineLocate.h"

namespace RadarPlugin {
int g_verbose;
#undef M_SETTINGS
#define M_SETTINGS m_settings

// the class factories, used to create and destroy instances of the PlugIn

//extern "C" DECL_EXP opencpn_plugin *create_pi(void *ppimgr) { return new radar_pi(ppimgr); }

//extern "C" DECL_EXP void destroy_pi(opencpn_plugin *p) { delete p; }

/********************************************************************************************************/
//   Distance measurement for simple sphere
/********************************************************************************************************/

double local_distance(GeoPosition pos1, GeoPosition pos2) {
  double s1 = deg2rad(pos1.lat);
  double l1 = deg2rad(pos1.lon);
  double s2 = deg2rad(pos2.lat);
  double l2 = deg2rad(pos2.lon);
  double theta = l2 - l1;

  // Spherical Law of Cosines
  double dist = acos(sin(s1) * sin(s2) + cos(s1) * cos(s2) * cos(theta));

  dist = fabs(rad2deg(dist)) * 60;  // nautical miles/degree
  return dist;
}

double local_bearing(GeoPosition pos1, GeoPosition pos2) {
  double s1 = deg2rad(pos1.lat);
  double l1 = deg2rad(pos1.lon);
  double s2 = deg2rad(pos2.lat);
  double l2 = deg2rad(pos2.lon);
  double theta = l2 - l1;

  double y = sin(theta) * cos(s2);
  double x = cos(s1) * sin(s2) - sin(s1) * cos(s2) * cos(theta);

  double brg = MOD_DEGREES_FLOAT(rad2deg(atan2(y, x)));
  return brg;
}

static double radar_distance(GeoPosition pos1, GeoPosition pos2, char unit) {
  double dist = local_distance(pos1, pos2);

  switch (unit) {
    case 'M':  // statute miles
      dist = dist * 1.1515;
      break;
    case 'K':  // kilometers
      dist = dist * 1.852;
      break;
    case 'm':  // meters
      dist = dist * 1852.0;
      break;
    case 'N':  // nautical miles
      break;
  }
  return dist;
}

/*
 * Given a geo position and distance and bearing, what is the geo position
 * of that indicated point?
 *
 * Distance in m, bearing in radians
 */
GeoPosition local_position(GeoPosition &pos, double distance, double bearing) {
  const double R = 6378100.;  // Radius of the Earth
  double lat = deg2rad(pos.lat);
  double lon = deg2rad(pos.lon);

  lat = asin(sin(lat) * cos(distance / R) + cos(lat) * sin(distance / R) * cos(bearing));
  lon += atan2(sin(bearing) * sin(distance / R) * cos(lat), cos(distance / R) - sin(lat) * sin(lat));

  GeoPosition r;

  r.lat = rad2deg(lat);
  r.lon = rad2deg(lon);
  return r;
}

//---------------------------------------------------------------------------------------------------------
//
//    Radar PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

radar_pi::radar_pi(void *ppimgr) /* : opencpn_plugin_116(ppimgr), m_raymarine_locator(0) */ {
  m_boot_time = wxGetUTCTimeMillis();
  m_initialized = false;
//  m_predicted_position_initialised = false;

  M_SETTINGS = {0};
  m_frame_counter = 0;
  process_arpa_target_fn = 0;
 }

radar_pi::~radar_pi() {}

/*
 * Init() is called -every- time that the plugin is enabled. If a user is being nasty
 * they can enable/disable multiple times in the overview. Grrr!
 *
 */
int radar_pi::Init(void) {
  if (m_initialized) {
    // Whoops, shouldn't happen
    return 0; //PLUGIN_OPTIONS;
  }

  time_t now = time(0);

  m_hdt = 0.0;
  m_hdt_timeout = now + WATCHDOG_TIMEOUT;
  m_hdm_timeout = now + WATCHDOG_TIMEOUT;
  m_var = 0.0;
  m_var_source = VARIATION_SOURCE_NONE;
  m_bpos_set = false;
  m_ownship.lat = nan("");
  m_ownship.lon = nan("");

  m_guard_bogey_seen = false;
  m_guard_bogey_confirmed = false;

  // Set default settings before we load config. Prevents random behavior on uninitalized behavior.
  // For instance, LOG_XXX messages before config is loaded.
  m_settings.show = 1; // Without this guard zones are inactive
  m_settings.verbose = 0;
  m_settings.overlay_transparency = DEFAULT_OVERLAY_TRANSPARENCY;
  m_settings.refreshrate = 1;
  m_settings.threshold_blue = 255;
  m_settings.threshold_red = 255;
  m_settings.threshold_green = 255;
  m_settings.enable_cog_heading = false;
  m_settings.AISatARPAoffset = 50;
  m_settings.range_units = RANGE_METRIC;

  m_navico_locator = 0;
  m_raymarine_locator = 0;

  // Create objects before config, so config can set data in it
  // This does not start any threads or generate any UI.
 for (size_t r = 0; r < RADARS; r++) {
   m_radar[r] = new RadarInfo(this, r);
   m_settings.show_radar[r] = true;
   m_settings.dock_radar[r] = false;

   // #206: Discussion on whether at this point the contour length
   // is really 6. The assert() proves this in debug (alpha) releases.
   assert(m_radar[r]->m_min_contour_length == 6);
 }

  //    And load the configuration items
 if (LoadConfig()) {
   LOG_INFO(wxT("Configuration file values initialised"));
   LOG_INFO(wxT("Log verbosity = %d. To modify, set VerboseLog to sum of:"), g_verbose);
   LOG_INFO(wxT("VERBOSE  = %d"), LOGLEVEL_VERBOSE);
   LOG_INFO(wxT("DIALOG   = %d"), LOGLEVEL_DIALOG);
   LOG_INFO(wxT("TRANSMIT = %d"), LOGLEVEL_TRANSMIT);
   LOG_INFO(wxT("RECEIVE  = %d"), LOGLEVEL_RECEIVE);
   LOG_INFO(wxT("GUARD    = %d"), LOGLEVEL_GUARD);
   LOG_INFO(wxT("ARPA     = %d"), LOGLEVEL_ARPA);
   LOG_INFO(wxT("REPORTS  = %d"), LOGLEVEL_REPORTS);
   LOG_VERBOSE(wxT("VERBOSE  log is enabled"));
   LOG_DIALOG(wxT("DIALOG   log is enabled"));
   LOG_TRANSMIT(wxT("TRANSMIT log is enabled"));
   LOG_RECEIVE(wxT("RECEIVE  log is enabled"));
   LOG_GUARD(wxT("GUARD    log is enabled"));
   LOG_ARPA(wxT("ARPA     log is enabled"));
   LOG_REPORTS(wxT("REPORTS  log is enabled"));
 } else {
   wxLogError(wxT("configuration file values initialisation failed"));
   return 0;  // give up
 }

  // CacheSetToolbarToolBitmaps(BM_ID_RED, BM_ID_BLANK);
  // Now that the settings are made we can initialize the RadarInfos
  for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
    m_radar[r]->Init();
    StartRadarLocators(r);
  }

  m_initialized = true;

  return 0;
}

void radar_pi::StartRadarLocators(size_t r) {
  if ((m_radar[r]->m_radar_type == RT_3G || m_radar[r]->m_radar_type == RT_4GA || m_radar[r]->m_radar_type == RT_HaloA ||
       m_radar[r]->m_radar_type == RT_HaloB) &&
      m_navico_locator == NULL) {
    m_navico_locator = new NavicoLocate(this);
    if (m_navico_locator->Run() != wxTHREAD_NO_ERROR) {
      wxLogError(wxT("unable to start Navico Radar Locator thread"));
    }
  }
  if ((m_radar[r]->m_radar_type == RM_E120 || m_radar[r]->m_radar_type == RM_QUANTUM) && m_raymarine_locator == NULL) {
    m_raymarine_locator = new RaymarineLocate(this);
    if (m_raymarine_locator->Run() != wxTHREAD_NO_ERROR) {
      wxLogError(wxT("unable to start Raymarine Radar Locator thread"));
    } else {
      LOG_INFO(wxT("radar_pi Raymarine locator started"));
    }
  }
}

void radar_pi::StopRadarLocators() {
  if (m_navico_locator) {
    m_navico_locator->Shutdown();
    m_navico_locator->Wait();
    delete m_navico_locator;
    m_navico_locator = 0;
  }

  if (m_raymarine_locator) {
    m_raymarine_locator->Shutdown();
    m_raymarine_locator->Wait();
    delete m_raymarine_locator;
    m_raymarine_locator = 0;
  }
}

bool radar_pi::DeInit(void) {
  if (!m_initialized) {
    return false;
  }

  StopRadarLocators();

  // Stop processing in all radars.
  // This waits for the receive threads to stop and removes the dialog, so that its settings
  // can be saved.
  for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
    m_radar[r]->Shutdown();
  }

  StopRadarLocators();
  return true;
}

 void radar_pi::SetRadarHeading(double heading, bool isTrue) {
  wxCriticalSectionLocker lock(m_exclusive);
  time_t now = time(0);
  if (!wxIsNaN(heading)) {
    if (m_heading_source == HEADING_FIXED) {
      m_hdt = heading;
      return;
    }
    if (isTrue) {
      m_heading_source = HEADING_RADAR_HDT;
      m_hdt = heading;
      m_hdt_timeout = now + HEADING_TIMEOUT;
    } else {
      m_heading_source = HEADING_RADAR_HDM;
      m_hdm = heading;
      m_hdt = heading + m_var;
      m_hdm_timeout = now + HEADING_TIMEOUT;
    }
  } else if (m_heading_source == HEADING_RADAR_HDM || m_heading_source == HEADING_RADAR_HDT) {
    // no heading on radar and heading source is still radar
    m_heading_source = HEADING_NONE;
  }
}
 
// Called between 1 and 10 times per second by RenderGLOverlay call
void radar_pi::TimedControlUpdate() {
  if (m_heading_source == HEADING_FIXED) {
    while (m_settings.fixed_heading_value >= 360) m_settings.fixed_heading_value -= 360;
    while (m_settings.fixed_heading_value < -180) m_settings.fixed_heading_value += 360;
    SetRadarHeading(m_settings.fixed_heading_value);
  }

  bool updateAllControls = true; //m_notify_control_dialog;
  // Always reset the counters, so they don't show huge numbers after IsShown changes
  for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
    wxCriticalSectionLocker lock(m_radar[r]->m_exclusive);

    m_radar[r]->m_statistics.broken_packets = 0;
    m_radar[r]->m_statistics.broken_spokes = 0;
    m_radar[r]->m_statistics.missing_spokes = 0;
    m_radar[r]->m_statistics.packets = 0;
    m_radar[r]->m_statistics.spokes = 0;
  }

  wxString info;
  switch (m_heading_source) {
    case HEADING_NONE:
      break;
    case HEADING_FIX_HDM:
    case HEADING_NMEA_HDM:
    case HEADING_RADAR_HDM:
      info = wxT(" ");
      break;
    case HEADING_FIX_COG:
      info = _("COG");
      break;
    case HEADING_FIXED:
      info = _("Fixed");
      break;
    case HEADING_FIX_HDT:
    case HEADING_NMEA_HDT:
      info = _("HDT");
      break;
    case HEADING_RADAR_HDT:
      info = _("RADAR");
      break;
  }
  if (info.Len() > 0 && !wxIsNaN(m_hdt)) {
    info << wxString::Format(wxT(" %3.1f"), m_hdt);
  }
  //m_pMessageBox->SetTrueHeadingInfo(info);
  info = _("");
  switch (m_heading_source) {
    case HEADING_FIXED:
      break;
    case HEADING_NONE:
    case HEADING_FIX_COG:
    case HEADING_FIX_HDT:
    case HEADING_NMEA_HDT:
    case HEADING_RADAR_HDT:
      info = wxT(" ");
      break;
    case HEADING_FIX_HDM:
    case HEADING_NMEA_HDM:
      info = _("HDM");
      if (info.Len() > 0 && !wxIsNaN(m_hdm)) {
        info << wxString::Format(wxT(" %3.1f"), m_hdm);
      }
      break;
    case HEADING_RADAR_HDM:
      info = _("RADAR");
      break;
  }

  UpdateAllControlStates(updateAllControls);
  UpdateState();
}

 
void dump_frame(RadarInfo* ri, int m_frame_counter) {
    std::ofstream outfile("frame_" + std::to_string(m_frame_counter) + ".dump",
                          std::ios::out | std::ios::binary);
    if (!outfile) {
        std::cerr << "Error opening frame file" << std::endl;
        return;
    }
    for (int i = 0; i < 2048; ++i) {
        outfile.write(reinterpret_cast<const char*>(ri->m_history[i].line), 1024);
    }
    outfile.close();
}

void radar_pi::TimedUpdate() {
  // Started in Init(), running every 500 ms
  // No screen output in this thread

  // Check if initialized
  if (!m_initialized) {
    return;
  }
  
  // refresh ARPA targets
  for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
    bool arpa_on = false;
    if (m_radar[r]) {
      wxCriticalSectionLocker lock(m_radar[r]->m_exclusive);
      if (m_radar[r]->m_arpa) {
        for (int i = 0; i < GUARD_ZONES; i++) {
          if (m_radar[r]->m_guard_zone[i]->m_arpa_on) {
            arpa_on = true;
          }
        }
        if (m_radar[r]->m_arpa->GetTargetCount() > 0) {
          arpa_on = true;
        }
      }
      if (m_radar[r]->m_doppler.GetValue() > 0 && m_radar[r]->m_autotrack_doppler.GetValue() > 0) {
        arpa_on = true;
      }
      if (arpa_on) {
        m_radar[r]->m_arpa->RefreshArpaTargets();
      }
    }
  }

  // Check the age of "radar_seen", if too old radar_seen = false
  bool any_data_seen = false;
  for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
    if (m_radar[r]) {
      wxCriticalSectionLocker lock(m_radar[r]->m_exclusive);
      int state = m_radar[r]->m_state.GetValue();  // Safe, protected by lock
      if (state == RADAR_TRANSMIT) {
        any_data_seen = true;
      }
      if (!m_settings.show            // No radar shown
          || state != RADAR_TRANSMIT  // Radar not transmitting
          || !m_bpos_set) {           // No overlay possible (yet)
                                      // Conditions for ARPA not fulfilled, delete all targets
        m_radar[r]->m_arpa->RadarLost();
      }
      m_radar[r]->UpdateTransmitState();
    }
  }

}

void radar_pi::UpdateAllControlStates(bool all) {
  for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
    m_radar[r]->UpdateControlState(all);
  }
}
 
void radar_pi::UpdateState(void) {
  for (size_t r = 0; r < M_SETTINGS.radar_count; r++) {
    m_radar[r]->CheckTimedTransmit();
  }
}

bool radar_pi::LoadConfig(void) {
  wxFileConfig *pConf = m_pconfig;
  int v, x, y, state;
  wxString s;

  if (pConf) {
    pConf->SetPath(wxT("/Settings"));
    pConf->Read(wxT("ConfigVersionString"), &s, "");
    if (sscanf(s.c_str(), "Version %d.%d.%d", &v, &x, &y) == 3) {
      wxLogInfo(wxT("Detected OpenCPN Version %d.%d.%d"), v, x, y);
      if (v == 5 && x == 8 && y <= 4) {
        wxLogError(wxT("Version %d.%d.%d cannot draw AIS targets on PPI; disabling this feature\n"), v, x, y);
//        m_ais_drawgl_broken = true;
      }
    }

    pConf->SetPath(wxT("/Plugins/Radar"));

    // Valgrind: This needs to be set before we set range, since that uses this
    pConf->Read(wxT("RangeUnits"), &v, RANGE_NAUTIC);
    m_settings.range_units = (RangeUnits)wxMax(wxMin(v, 2), 0);

    pConf->Read(wxT("VerboseLog"), &g_verbose, 0);

    pConf->Read(wxT("DockSize"), &v, 0);
    m_settings.dock_size = v;

    pConf->Read(wxT("FixedHeading"), &m_settings.fixed_heading, 0);
    if (m_settings.fixed_heading == 1) {
      m_heading_source = HEADING_FIXED;
    }
    pConf->Read(wxT("FixedHeadingValue"), &m_settings.fixed_heading_value, 0);
    if (m_settings.fixed_heading == 1) {
      while (m_settings.fixed_heading_value >= 360.) m_settings.fixed_heading_value -= 360.;
      while (m_settings.fixed_heading_value < -180.) m_settings.fixed_heading_value += 360.;
      SetRadarHeading(m_settings.fixed_heading_value);
    }
    pConf->Read(wxT("FixedPosition"), &m_settings.pos_is_fixed, 0);
    pConf->Read(wxT("FixedLatValue"), &m_settings.fixed_pos.lat, 0);
    pConf->Read(wxT("FixedLonValue"), &m_settings.fixed_pos.lon, 0);
    pConf->Read(wxT("RadarDescription"), &m_settings.radar_description_text, _("empty"));

    size_t n = 0;
    for (int r = 0; r < RADARS; r++) {
      RadarInfo *ri = m_radar[n];
      if (ri == NULL) {
        wxLogError(wxT("Cannot load radar %d as the object is not initialised"), r + 1);
        continue;
      }
      pConf->Read(wxString::Format(wxT("Radar%dType"), r), &s, "unknown");
      ri->m_radar_type = RT_MAX;  // = not used
      for (int i = 0; i < RT_MAX; i++) {
        if (s.IsSameAs(RadarTypeName[i])) {
          ri->m_radar_type = (RadarType)i;
          break;
        }
      }
      if (ri->m_radar_type == RT_MAX) {
        continue;  // This happens if someone changed the name in the config file or
                   // we drop support for a type or rename it.
      }

      pConf->Read(wxString::Format(wxT("Radar%dInterface"), r), &s, "0.0.0.0");
      radar_inet_aton(s.c_str(), &ri->m_radar_interface_address.addr);
      ri->m_radar_interface_address.port = 0;
      pConf->Read(wxString::Format(wxT("Radar%dAddress"), r), &s, "0.0.0.0");
      radar_inet_aton(s.c_str(), &ri->m_radar_address.addr);
      ri->m_radar_address.port = htons(RadarOrder[ri->m_radar_type]);
      pConf->Read(wxString::Format(wxT("Radar%dLocationInfo"), r), &s, " ");
      ri->SetRadarLocationInfo(RadarLocationInfo(s));

      pConf->Read(wxString::Format(wxT("Radar%dRange"), r), &v, 2000);
      ri->m_range.Update(v);
      pConf->Read(wxString::Format(wxT("Radar%dRotation"), r), &v, 0);
      if (v == ORIENTATION_HEAD_UP) {
        v = ORIENTATION_STABILIZED_UP;
      }
      ri->m_orientation.Update(v);

      pConf->Read(wxString::Format(wxT("Radar%dTransmit"), r), &v, 0);
      ri->m_boot_state.Update(v);
      pConf->Read(wxString::Format(wxT("Radar%dMinContourLength"), r), &ri->m_min_contour_length, 6);
      if (ri->m_min_contour_length > 10) ri->m_min_contour_length = 6;  // Prevent user and system error
      pConf->Read(wxString::Format(wxT("Radar%dDopplerAutoTrack"), r), &v, 0);
      ri->m_autotrack_doppler.Update(v);
      pConf->Read(wxString::Format(wxT("Radar%dThreshold"), r), &v, 0);
      ri->m_threshold.Update(v);

      pConf->Read(wxString::Format(wxT("Radar%dTrailsState"), r), &state, RCS_OFF);
      pConf->Read(wxString::Format(wxT("Radar%dTrails"), r), &v, 0);
      ri->m_target_trails.Update(v, (RadarControlState)state);
      LOG_VERBOSE(wxT("Radar %d Target trails value %d state %d read from ini file"), r, v, state);
      pConf->Read(wxString::Format(wxT("Radar%dTrueTrailsMotion"), r), &v, 1);
      ri->m_trails_motion.Update(v);
      pConf->Read(wxString::Format(wxT("Radar%dMainBangSize"), r), &v, 0);
      ri->m_main_bang_size.Update(v);
      pConf->Read(wxString::Format(wxT("Radar%dAntennaForward"), r), &v, 0);
      ri->m_antenna_forward.Update(v);
      pConf->Read(wxString::Format(wxT("Radar%dAntennaStarboard"), r), &v, 0);
      ri->m_antenna_starboard.Update(v);
      pConf->Read(wxString::Format(wxT("Radar%dRangeAdjustment"), r), &v, 0);
      ri->m_range_adjustment.Update(v);
      pConf->Read(wxString::Format(wxT("Radar%dRunTimeOnIdle"), r), &v, 1);
      ri->m_timed_run.Update(v);

      for (int i = 0; i < MAX_CHART_CANVAS; i++) {
        pConf->Read(wxString::Format(wxT("Radar%dOverlayCanvas%d"), r, i), &v, 0);
        ri->m_overlay_canvas[i].Update(v);
      }

      pConf->Read(wxString::Format(wxT("Radar%dWindowShow"), r), &m_settings.show_radar[n], true);
      pConf->Read(wxString::Format(wxT("Radar%dWindowDock"), r), &m_settings.dock_radar[n], false);
      pConf->Read(wxString::Format(wxT("Radar%dControlShow"), r), &m_settings.show_radar_control[n], false);
      pConf->Read(wxString::Format(wxT("Radar%dTargetShow"), r), &v, true);
      ri->m_target_on_ppi.Update(v);

      LOG_DIALOG(wxT("LoadConfig: show_radar[%d]=%d control=%d,%d"), n, v, x, y);
      for (int i = 0; i < GUARD_ZONES; i++) {
        pConf->Read(wxString::Format(wxT("Radar%dZone%dStartBearing"), r, i), &ri->m_guard_zone[i]->m_start_bearing, 0);
        pConf->Read(wxString::Format(wxT("Radar%dZone%dEndBearing"), r, i), &ri->m_guard_zone[i]->m_end_bearing, 0);
        pConf->Read(wxString::Format(wxT("Radar%dZone%dOuterRange"), r, i), &ri->m_guard_zone[i]->m_outer_range, 0);
        pConf->Read(wxString::Format(wxT("Radar%dZone%dInnerRange"), r, i), &ri->m_guard_zone[i]->m_inner_range, 0);
        pConf->Read(wxString::Format(wxT("Radar%dZone%dType"), r, i), &v, 0);
        pConf->Read(wxString::Format(wxT("Radar%dZone%dAlarmOn"), r, i), &ri->m_guard_zone[i]->m_alarm_on, 0);
        pConf->Read(wxString::Format(wxT("Radar%dZone%dArpaOn"), r, i), &ri->m_guard_zone[i]->m_arpa_on, 0);
        ri->m_guard_zone[i]->SetType((GuardZoneType)v);
      }
      pConf->Read(wxT("EnableCOGHeading"), &m_settings.enable_cog_heading, false);
      pConf->Read(wxT("AISatARPAoffset"), &m_settings.AISatARPAoffset, 50);
      if (m_settings.AISatARPAoffset < 10 || m_settings.AISatARPAoffset > 300) m_settings.AISatARPAoffset = 50;

      n++;
    }
    m_settings.radar_count = n;

    pConf->Read(wxT("ColourStrong"), &s, "red");
    m_settings.strong_colour = wxColour(s);
    pConf->Read(wxT("ColourIntermediate"), &s, "green");
    m_settings.intermediate_colour = wxColour(s);
    pConf->Read(wxT("ColourWeak"), &s, "blue");
    m_settings.weak_colour = wxColour(s);
    pConf->Read(wxT("ColourArpaEdge"), &s, "white");
    m_settings.arpa_colour = wxColour(s);
    pConf->Read(wxT("ColourAISText"), &s, "rgb(100,100,100)");
    m_settings.ais_text_colour = wxColour(s);
    pConf->Read(wxT("ColourPPIBackground"), &s, "rgb(0,0,50)");
    m_settings.ppi_background_colour = wxColour(s);
    pConf->Read(wxT("ColourDopplerApproaching"), &s, "yellow");
    m_settings.doppler_approaching_colour = wxColour(s);
    pConf->Read(wxT("ColourDopplerReceding"), &s, "cyan");
    m_settings.doppler_receding_colour = wxColour(s);
    pConf->Read(wxT("DeveloperMode"), &m_settings.developer_mode, false);
    pConf->Read(wxT("DrawingMethod"), &m_settings.drawing_method, 1);
    pConf->Read(wxT("GuardZoneDebugInc"), &m_settings.guard_zone_debug_inc, 0);
    pConf->Read(wxT("GuardZoneOnOverlay"), &m_settings.guard_zone_on_overlay, true);
    pConf->Read(wxT("OverlayStandby"), &m_settings.overlay_on_standby, true);
    pConf->Read(wxT("GuardZoneTimeout"), &m_settings.guard_zone_timeout, 30);
    pConf->Read(wxT("GuardZonesRenderStyle"), &m_settings.guard_zone_render_style, 0);
    pConf->Read(wxT("GuardZonesThreshold"), &m_settings.guard_zone_threshold, 5L);
    pConf->Read(wxT("IgnoreRadarHeading"), &m_settings.ignore_radar_heading, 0);
    pConf->Read(wxT("ShowExtremeRange"), &m_settings.show_extreme_range, false);
    pConf->Read(wxT("MenuAutoHide"), &m_settings.menu_auto_hide, 0);
    pConf->Read(wxT("PassHeadingToOCPN"), &m_settings.pass_heading_to_opencpn, false);
    pConf->Read(wxT("Refreshrate"), &v, 3);
    m_settings.refreshrate.Update(v);
    pConf->Read(wxT("ReverseZoom"), &m_settings.reverse_zoom, false);
    pConf->Read(wxT("ScanMaxAge"), &m_settings.max_age, 6);
    pConf->Read(wxT("Show"), &m_settings.show, true);
    pConf->Read(wxT("SkewFactor"), &m_settings.skew_factor, 1);
    pConf->Read(wxT("ThresholdBlue"), &m_settings.threshold_blue, 32);
    // Make room for BLOB_HISTORY_MAX history values
    m_settings.threshold_blue = MAX(m_settings.threshold_blue, BLOB_HISTORY_MAX + 1);
    pConf->Read(wxT("ThresholdGreen"), &m_settings.threshold_green, 100);
    pConf->Read(wxT("ThresholdRed"), &m_settings.threshold_red, 200);
    pConf->Read(wxT("TrailColourStart"), &s, "rgb(255,255,255)");
    m_settings.trail_start_colour = wxColour(s);
    pConf->Read(wxT("TrailColourEnd"), &s, "rgb(63,63,63)");
    m_settings.trail_end_colour = wxColour(s);
    pConf->Read(wxT("TrailsOnOverlay"), &m_settings.trails_on_overlay, false);
    pConf->Read(wxT("Transparency"), &v, DEFAULT_OVERLAY_TRANSPARENCY);
    m_settings.overlay_transparency.Update(v);

    m_settings.max_age = wxMax(wxMin(m_settings.max_age, MAX_AGE), MIN_AGE);

    SaveConfig();
    return true;
  }
  return false;
}

bool radar_pi::SaveConfig(void) {
  wxFileConfig *pConf = m_pconfig;
  if (pConf) {
    pConf->DeleteGroup(wxT("/Plugins/Radar"));
    pConf->SetPath(wxT("/Plugins/Radar"));

    pConf->Write(wxT("AlertAudioFile"), m_settings.alert_audio_file);
    pConf->Write(wxT("DeveloperMode"), m_settings.developer_mode);
    pConf->Write(wxT("DrawingMethod"), m_settings.drawing_method);
    pConf->Write(wxT("EnableCOGHeading"), m_settings.enable_cog_heading);
    pConf->Write(wxT("GuardZoneDebugInc"), m_settings.guard_zone_debug_inc);
    pConf->Write(wxT("GuardZoneOnOverlay"), m_settings.guard_zone_on_overlay);
    pConf->Write(wxT("OverlayStandby"), m_settings.overlay_on_standby);
    pConf->Write(wxT("GuardZoneTimeout"), m_settings.guard_zone_timeout);
    pConf->Write(wxT("GuardZonesRenderStyle"), m_settings.guard_zone_render_style);
    pConf->Write(wxT("GuardZonesThreshold"), m_settings.guard_zone_threshold);
    pConf->Write(wxT("IgnoreRadarHeading"), m_settings.ignore_radar_heading);
    pConf->Write(wxT("ShowExtremeRange"), m_settings.show_extreme_range);
    pConf->Write(wxT("MenuAutoHide"), m_settings.menu_auto_hide);
    pConf->Write(wxT("PassHeadingToOCPN"), m_settings.pass_heading_to_opencpn);
    pConf->Write(wxT("RangeUnits"), (int)m_settings.range_units);
    pConf->Write(wxT("Refreshrate"), m_settings.refreshrate.GetValue());
    pConf->Write(wxT("ReverseZoom"), m_settings.reverse_zoom);
    pConf->Write(wxT("ScanMaxAge"), m_settings.max_age);
    pConf->Write(wxT("Show"), m_settings.show);
    pConf->Write(wxT("SkewFactor"), m_settings.skew_factor);
    pConf->Write(wxT("ThresholdBlue"), m_settings.threshold_blue);
    pConf->Write(wxT("ThresholdGreen"), m_settings.threshold_green);
    pConf->Write(wxT("ThresholdRed"), m_settings.threshold_red);
    pConf->Write(wxT("TrailColourStart"), m_settings.trail_start_colour.GetAsString());
    pConf->Write(wxT("TrailColourEnd"), m_settings.trail_end_colour.GetAsString());
    pConf->Write(wxT("TrailsOnOverlay"), m_settings.trails_on_overlay);
    pConf->Write(wxT("Transparency"), m_settings.overlay_transparency.GetValue());
    pConf->Write(wxT("VerboseLog"), g_verbose);
    pConf->Write(wxT("AISatARPAoffset"), m_settings.AISatARPAoffset);
    pConf->Write(wxT("ColourStrong"), m_settings.strong_colour.GetAsString());
    pConf->Write(wxT("ColourIntermediate"), m_settings.intermediate_colour.GetAsString());
    pConf->Write(wxT("ColourWeak"), m_settings.weak_colour.GetAsString());
    pConf->Write(wxT("ColourDopplerApproaching"), m_settings.doppler_approaching_colour.GetAsString());
    pConf->Write(wxT("ColourDopplerReceding"), m_settings.doppler_receding_colour.GetAsString());
    pConf->Write(wxT("ColourArpaEdge"), m_settings.arpa_colour.GetAsString());
    pConf->Write(wxT("ColourAISText"), m_settings.ais_text_colour.GetAsString());
    pConf->Write(wxT("ColourPPIBackground"), m_settings.ppi_background_colour.GetAsString());
    pConf->Write(wxT("RadarCount"), m_settings.radar_count);
    pConf->Write(wxT("DockSize"), m_settings.dock_size);
    pConf->Write(wxT("FixedHeadingValue"), m_settings.fixed_heading_value);
    pConf->Write(wxT("FixedHeading"), m_settings.fixed_heading);
    pConf->Write(wxT("FixedPosition"), m_settings.pos_is_fixed);
    pConf->Write(wxT("FixedLatValue"), m_settings.fixed_pos.lat);
    pConf->Write(wxT("FixedLonValue"), m_settings.fixed_pos.lon);
    pConf->Write(wxT("RadarDescription"), m_settings.radar_description_text);
    pConf->Write(wxT("TargetMixerAddress"), m_settings.target_mixer_address.to_string());

    for (int r = 0; r < (int)m_settings.radar_count; r++) {
      pConf->Write(wxString::Format(wxT("Radar%dType"), r), RadarTypeName[m_radar[r]->m_radar_type]);
      pConf->Write(wxString::Format(wxT("Radar%dLocationInfo"), r), m_radar[r]->GetRadarLocationInfo().to_string());
      pConf->Write(wxString::Format(wxT("Radar%dAddress"), r), m_radar[r]->m_radar_address.FormatNetworkAddress());
      pConf->Write(wxString::Format(wxT("Radar%dInterface"), r), m_radar[r]->GetRadarInterfaceAddress().FormatNetworkAddress());
      pConf->Write(wxString::Format(wxT("Radar%dRange"), r), m_radar[r]->m_range.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dRotation"), r), m_radar[r]->m_orientation.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dTransmit"), r), m_radar[r]->m_state.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dWindowShow"), r), m_settings.show_radar[r]);
      pConf->Write(wxString::Format(wxT("Radar%dWindowDock"), r), m_settings.dock_radar[r]);
      pConf->Write(wxString::Format(wxT("Radar%dControlShow"), r), m_settings.show_radar_control[r]);
      pConf->Write(wxString::Format(wxT("Radar%dTargetShow"), r), m_radar[r]->m_target_on_ppi.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dThreshold"), r), m_radar[r]->m_threshold.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dTrailsState"), r), (int)m_radar[r]->m_target_trails.GetState());
      pConf->Write(wxString::Format(wxT("Radar%dTrails"), r), m_radar[r]->m_target_trails.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dTrueTrailsMotion"), r), m_radar[r]->m_trails_motion.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dMainBangSize"), r), m_radar[r]->m_main_bang_size.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dAntennaForward"), r), m_radar[r]->m_antenna_forward.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dAntennaStarboard"), r), m_radar[r]->m_antenna_starboard.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dRangeAdjustment"), r), m_radar[r]->m_range_adjustment.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dRunTimeOnIdle"), r), m_radar[r]->m_timed_run.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dDopplerAutoTrack"), r), m_radar[r]->m_autotrack_doppler.GetValue());
      pConf->Write(wxString::Format(wxT("Radar%dMinContourLength"), r), m_radar[r]->m_min_contour_length);

      for (int i = 0; i < MAX_CHART_CANVAS; i++) {
        pConf->Write(wxString::Format(wxT("Radar%dOverlayCanvas%d"), r, i), m_radar[r]->m_overlay_canvas[i].GetValue());
      }

      // LOG_DIALOG(wxT("SaveConfig: show_radar[%d]=%d"), r, m_settings.show_radar[r]);
      for (int i = 0; i < GUARD_ZONES; i++) {
        pConf->Write(wxString::Format(wxT("Radar%dZone%dStartBearing"), r, i), m_radar[r]->m_guard_zone[i]->m_start_bearing);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dEndBearing"), r, i), m_radar[r]->m_guard_zone[i]->m_end_bearing);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dOuterRange"), r, i), m_radar[r]->m_guard_zone[i]->m_outer_range);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dInnerRange"), r, i), m_radar[r]->m_guard_zone[i]->m_inner_range);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dType"), r, i), (int)m_radar[r]->m_guard_zone[i]->m_type);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dAlarmOn"), r, i), m_radar[r]->m_guard_zone[i]->m_alarm_on);
        pConf->Write(wxString::Format(wxT("Radar%dZone%dArpaOn"), r, i), m_radar[r]->m_guard_zone[i]->m_arpa_on);
      }
    }

    pConf->Flush();
    
    return true;
  }

  return false;
}
 
void radar_pi::logBinaryData(const wxString &what, const uint8_t *data, int size) {
  wxString explain;
  int i = 0;
  explain.Alloc(size * 3 + 50);
  explain += wxT("");
  explain += what;
  explain += wxString::Format(wxT(" %d bytes: "), size);
  for (i = 0; i < size; i++) {
    if (i % 16 == 0) {
      explain += wxString::Format(wxT(" \n %3d    "), i);
    } else if (i % 8 == 0) {
      explain += wxString::Format(wxT("  "), i);
    }
    explain += wxString::Format(wxT(" %02X"), data[i]);
  }
  LOG_INFO(explain);
}

}  // namespace RadarPlugin
