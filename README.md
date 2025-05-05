# NodeJS marine radar library

**Warning: This is so far just a prototype**

This is a nodejs wrapper for the radar receiver classes of [the OpenCPN radar plugin](https://github.com/opencpn-radar-pi/radar_pi),
making it possible to use them outside of OpenCPN and get direct programmatic access to radar imagery.

This opens the possibility to do experimentation on advanced image analysis on radar imagery, e.g. using machine learning or existing traditional computer vision libraries.

# Installation and usage

```
bash build.sh
nodejs test.js
```

# API
Radars start out in RADAR_OFF mode and change to RADAR_STANDBY when the radar is found. You need to wait for this event before telling the radar to transmit.

* `radar_receiver.getRadarTypes()` - List supported radar types. The index in this list is the radar_id.
* `radar_info = new radar_receiver.RadarInfo(radar_id, spoke_fn, config_fn)` - Connect to a radar. The two function arguments are
  * `spoke_fn(angle, bearing, data, range_meters, time) => undefined` - called for each received spoke of data
  * `config_fn({type: {...}, value: {...}})` - called whenever properties (configuration, status etc) changes.
* `radar_info.setProperty("PROPERTY_NAME", value)` - Set a property to some value.
* `radar_info.getProperty("PROPERTY_NAME")` - Get the current value for a property.
* `radar_info.getPropertyType("PROPERTY_NAME")` - get a type description for a property.
* `radar_info.getProperties()` - Returns all properties with their types and current values.
