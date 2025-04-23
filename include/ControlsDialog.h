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

#ifndef _CONTROLSDIALOG_H_
#define _CONTROLSDIALOG_H_

#include "RadarControlItem.h"
#include "radar_pi.h"
#define HAVE_CONTROL(a, b, c, d, e, f, g)
#include "SoftwareControlSet.h"
#undef HAVE_CONTROL

PLUGIN_BEGIN_NAMESPACE

enum { // process ID's
    ID_TEXTCTRL1 = 10000,
    ID_BACK,
    ID_PLUS_TEN,
    ID_PLUS,
    ID_VALUE,
    ID_MINUS,
    ID_MINUS_TEN,
    ID_AUTO,
    ID_OFF,
    ID_CONTROL_BUTTON,

    ID_INSTALLATION,
    ID_NO_TRANSMIT,
    ID_PREFERENCES,

    ID_GAIN,

    ID_CLEAR_CURSOR,
    ID_ACQUIRE_TARGET,
    ID_DELETE_TARGET,
    ID_DELETE_ALL_TARGETS,

    ID_TARGETS_ON_PPI,
    ID_CLEAR_TRAILS,
    ID_ORIENTATION,
    ID_VIEW_CENTER,

    ID_TRANSMIT_STANDBY,

    ID_SHOW_RADAR_PPI,
    ID_DOCK_RADAR_PPI,

    ID_RADAR_OVERLAY0,
    ID_ADJUST = ID_RADAR_OVERLAY0 + MAX_CHART_CANVAS,
    ID_ADVANCED,
    ID_GUARDZONE,
    ID_WINDOW,
    ID_VIEW,
    ID_BEARING,
    ID_ZONE1,
    ID_ZONE2,
    ID_POWER,

    ID_CONFIRM_BOGEY,

    ID_MESSAGE,
    ID_BPOS,

    ID_BEARING_SET, // next one should be BEARING_LINES higher
    ID_NEXT = ID_BEARING_SET + BEARING_LINES,

};

class RadarControlButton;
class RadarRangeControlButton;
class RadarButton;

extern wxString guard_zone_names[2];

extern const string ControlTypeNames[CT_MAX];

extern wxSize g_buttonSize;

class ControlInfo {
public:
    ControlType type;
    int autoValues;
    wxString* autoNames;
    bool hasOff;
    bool hasAutoAdjustable;
    int defaultValue;
    int minValue;
    int maxValue;
    int minAdjustValue;
    int maxAdjustValue;
    int stepValue;
    int nameCount;
    wxString unit;
    wxString* names;
};

//----------------------------------------------------------------------------------------------------------
//    Radar Control Dialog Specification
//----------------------------------------------------------------------------------------------------------
class ControlsDialog  {
public:
    ControlsDialog()
    {
     /*
        for (size_t i = 0; i < ARRAY_SIZE(m_ctrl); i++) {
            m_ctrl[i].type = CT_NONE;
            m_ctrl[i].names = 0;
            m_ctrl[i].autoNames = 0;
            m_ctrl[i].hasOff = false;
            m_ctrl[i].hasAutoAdjustable = false;
        }
     */
    };

    ~ControlsDialog();

    ControlInfo m_ctrl[CT_MAX];
protected:
    void DefineControl(ControlType ct, int autoValues, wxString auto_names[],
        int defaultValue, int minValue, int maxValue, int stepValue,
        int nameCount, wxString names[])
    {
        m_ctrl[ct].type = ct;
        if (defaultValue == CTD_DEF_OFF) {
            m_ctrl[ct].hasOff = true;
            defaultValue = CTD_DEF_ZERO;
        }
        m_ctrl[ct].defaultValue = defaultValue;
        m_ctrl[ct].minValue = minValue;
        m_ctrl[ct].maxValue = maxValue;
        m_ctrl[ct].stepValue = stepValue;
        m_ctrl[ct].nameCount = nameCount;

        // To simplify the macros a control without autovalues passes in
        // CTD_AUTO_NO, which is an array of 1 with length zero.
        if (autoValues == 1 && auto_names[0].length() == 0) {
            autoValues = 0;
            m_ctrl[ct].autoNames = 0;
        }
        m_ctrl[ct].autoValues = autoValues;

        if (autoValues > 0) {
            m_ctrl[ct].autoNames = new wxString[autoValues];
            for (int i = 0; i < autoValues; i++) {
                m_ctrl[ct].autoNames[i] = auto_names[i];
            }
        }

        if (nameCount == 1 && names[0].length() > 0) {
            m_ctrl[ct].unit = names[0];
        } else if (nameCount > 0 && names[0].length() > 0) {
            m_ctrl[ct].names = new wxString[nameCount];
            for (int i = 0; i < nameCount; i++) {
                m_ctrl[ct].names[i] = names[i];
            }
        }
    }
};

class RadarButton {
public:
    RadarButton() {

    };
};

class RadarControlButton : public wxButton {
    friend class RadarRangeControlButton;

public:
    RadarControlButton() {

    };
};

class RadarRangeControlButton : public RadarControlButton {
public:
};

// This sets up the initializer macro in the constructor of the
// derived control dialogs.
#define HAVE_CONTROL(a, b, c, d, e, f, g)                                      \
    wxString a##_auto_names[] = b;                                             \
    wxString a##_names[] = g;                                                  \
    DefineControl(a, ARRAY_SIZE(a##_auto_names), a##_auto_names, c, d, e, f,   \
        ARRAY_SIZE(a##_names), a##_names);
#define SKIP_CONTROL(a)

PLUGIN_END_NAMESPACE

#endif
