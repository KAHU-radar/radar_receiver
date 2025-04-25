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
}

// wxSize g_buttonSize;


PLUGIN_END_NAMESPACE
