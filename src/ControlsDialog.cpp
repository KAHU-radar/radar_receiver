/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  Radar Plugin
 * Author:   David Register
 *           Dave Cowell
 *           Kees Verruijt
 *           Douwe Fokkema
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

#include "ControlsDialog.h"
#include "RadarInfo.h"

PLUGIN_BEGIN_NAMESPACE

// The following are only for logging, so don't care about translations.
const string ControlTypeNames[CT_MAX] = {
    "Unused",
#define CONTROL_TYPE(x, y) y,
#include "ControlType.inc"
#undef CONTROL_TYPE
};

wxString RadarControlButton::GetLabel() const {
  return firstLine;
}

void RadarControlButton::SetFirstLine(wxString first_line) { firstLine = first_line; }

void RadarControlButton::UpdateLabel(bool force) {
}


ControlsDialog::~ControlsDialog() {
  for (size_t i = 0; i < ARRAY_SIZE(m_ctrl); i++) {
    if (m_ctrl[i].names) {
      delete[] m_ctrl[i].names;
    }
    if (m_ctrl[i].autoNames) {
      delete[] m_ctrl[i].autoNames;
    }
  }
  for (size_t i = 0; i < ARRAY_SIZE(m_button); i++) {
    if (m_button[i]) {
      delete[] m_button[i];
    }
  }
}

void ControlsDialog::Create(radar_pi *m_pi, RadarInfo *m_ri) {
  #define CONTROL_BTN_GENERAL(CTRL, name) m_button[CTRL] = new RadarControlButton(this, m_ctrl[CTRL], &name)
  #define CONTROL_BTN(CTRL, member) CONTROL_BTN_GENERAL(CTRL, m_ri->member)
  #define CONTROL_BTN_COND(CTRL, member) if (m_ctrl[CTRL].type) { CONTROL_BTN(CTRL, member); }
  CONTROL_BTN_COND(CT_STATE, m_state);
  CONTROL_BTN_COND(CT_NOISE_REJECTION, m_noise_rejection);
  CONTROL_BTN_COND(CT_THRESHOLD, m_threshold);
  CONTROL_BTN_COND(CT_TARGET_EXPANSION, m_target_expansion);
  CONTROL_BTN_COND(CT_STC, m_stc);
  CONTROL_BTN_COND(CT_TUNE_FINE, m_tune_fine);
  CONTROL_BTN_COND(CT_TUNE_COARSE, m_coarse_tune);
  CONTROL_BTN_COND(CT_STC_CURVE, m_stc_curve);
  CONTROL_BTN_COND(CT_INTERFERENCE_REJECTION, m_interference_rejection);
  CONTROL_BTN_COND(CT_TARGET_SEPARATION, m_target_separation);
  CONTROL_BTN_COND(CT_SCAN_SPEED, m_scan_speed);
  CONTROL_BTN_COND(CT_TARGET_BOOST, m_target_boost);
  CONTROL_BTN_COND(CT_BEARING_ALIGNMENT, m_bearing_alignment);
  CONTROL_BTN_COND(CT_RANGE_ADJUSTMENT, m_range_adjustment);
  CONTROL_BTN_COND(CT_DISPLAY_TIMING, m_display_timing);
  CONTROL_BTN_COND(CT_MAIN_BANG_SUPPRESSION, m_main_bang_suppression);
  for (size_t z = 0; z < NO_TRANSMIT_ZONES; z++) {
    // The NO TRANSMIT START button
    if (m_ctrl[CT_NO_TRANSMIT_START_1 + z].type) {
      CONTROL_BTN(CT_NO_TRANSMIT_START_1 + z, m_no_transmit_start[z]);
      m_ri->m_no_transmit_zones = wxMax(m_ri->m_no_transmit_zones, z + 1);
    }

    if (m_ctrl[CT_NO_TRANSMIT_END_1 + z].type) {
      CONTROL_BTN(CT_NO_TRANSMIT_END_1 + z, m_no_transmit_end[z]);
    }
  }
  CONTROL_BTN_COND(CT_ANTENNA_HEIGHT, m_antenna_height);
  CONTROL_BTN_COND(CT_ANTENNA_FORWARD, m_antenna_forward);
  CONTROL_BTN_COND(CT_ANTENNA_STARBOARD, m_antenna_starboard);
  CONTROL_BTN_COND(CT_LOCAL_INTERFERENCE_REJECTION, m_local_interference_rejection);
  CONTROL_BTN_COND(CT_SIDE_LOBE_SUPPRESSION, m_side_lobe_suppression);
  CONTROL_BTN_COND(CT_MAIN_BANG_SIZE, m_main_bang_size);
  CONTROL_BTN_COND(CT_ACCENT_LIGHT, m_accent_light);
  CONTROL_BTN_COND(CT_RANGE, m_range);
  CONTROL_BTN_COND(CT_GAIN, m_gain);
  CONTROL_BTN_COND(CT_COLOR_GAIN, m_color_gain);
  CONTROL_BTN_COND(CT_SEA, m_sea);
  CONTROL_BTN_COND(CT_SEA_STATE, m_sea_state);
  CONTROL_BTN_COND(CT_RAIN, m_rain);
  CONTROL_BTN_COND(CT_FTC, m_ftc);
  CONTROL_BTN_COND(CT_MODE, m_mode);
  CONTROL_BTN_COND(CT_ALL_TO_AUTO, m_all_to_auto);
  for (int i = 0; i < MAX_CHART_CANVAS; i++) {
    CONTROL_BTN(CT_OVERLAY_CANVAS, m_overlay_canvas[i]);
  }
  CONTROL_BTN_GENERAL(CT_TRANSPARENCY, m_pi->m_settings.overlay_transparency);
  CONTROL_BTN_COND(CT_DOPPLER, m_doppler);
  CONTROL_BTN_COND(CT_DOPPLER_THRESHOLD, m_doppler_threshold);
  CONTROL_BTN_COND(CT_AUTOTTRACKDOPPLER, m_autotrack_doppler);
  CONTROL_BTN_COND(CT_TARGET_TRAILS, m_target_trails);
  CONTROL_BTN_COND(CT_TRAILS_MOTION, m_trails_motion);
  CONTROL_BTN(CT_ORIENTATION, m_orientation);
  CONTROL_BTN(CT_CENTER_VIEW, m_view_center);
  if (m_ctrl[CT_REFRESHRATE].type) {
   CONTROL_BTN_GENERAL(CT_REFRESHRATE, m_pi->m_settings.refreshrate);
  }
  if (m_ctrl[CT_TIMED_IDLE].type) {
    CONTROL_BTN(CT_TIMED_IDLE, m_timed_idle);
    CONTROL_BTN_COND(CT_TIMED_RUN, m_timed_run);
  }
}


PLUGIN_END_NAMESPACE
