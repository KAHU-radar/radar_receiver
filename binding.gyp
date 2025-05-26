{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "src/addon.cpp",
        "src/radar_info_wrapper.cpp",
        "src/settings.cpp",

        "src/radar_pi.cpp",
        "src/RadarInfo.cpp",
        "src/ControlsDialog.cpp",
        "src/Arpa.cpp",

        "deps/radar_pi/src/GuardZone.cpp",
        "deps/radar_pi/src/Kalman.cpp",
        "deps/radar_pi/src/RadarFactory.cpp",
        "deps/radar_pi/src/socketutil.cpp",

        "deps/radar_pi/src/emulator/EmulatorControl.cpp",
        "deps/radar_pi/src/emulator/EmulatorControlsDialog.cpp",
        "deps/radar_pi/src/emulator/EmulatorReceive.cpp",
        "deps/radar_pi/src/garminhd/GarminHDControl.cpp",
        "deps/radar_pi/src/garminhd/GarminHDControlsDialog.cpp",
        "deps/radar_pi/src/garminhd/GarminHDReceive.cpp",
        "deps/radar_pi/src/garminxhd/GarminxHDControl.cpp",
        "deps/radar_pi/src/garminxhd/GarminxHDControlsDialog.cpp",
        "deps/radar_pi/src/garminxhd/GarminxHDReceive.cpp",
        "deps/radar_pi/src/navico/NavicoControl.cpp",
        "deps/radar_pi/src/navico/NavicoControlsDialog.cpp",
        "deps/radar_pi/src/navico/NavicoLocate.cpp",
        "deps/radar_pi/src/navico/NavicoReceive.cpp",
        "deps/radar_pi/src/raymarine/RaymarineLocate.cpp",
        "deps/radar_pi/src/raymarine/RaymarineReceive.cpp",
        "deps/radar_pi/src/raymarine/RME120Control.cpp",
        "deps/radar_pi/src/raymarine/RME120ControlsDialog.cpp",
        "deps/radar_pi/src/raymarine/RMQuantumControl.cpp",
        "deps/radar_pi/src/raymarine/RMQuantumControlsDialog.cpp",
      ],
      "include_dirs": [
        "include",

        "deps/radar_pi/include/emulator",
        "deps/radar_pi/include/garminhd",
        "deps/radar_pi/include/garminxhd",
        "deps/radar_pi/include/navico",
        "deps/radar_pi/include/raymarine",
        
        "<!(node -p \"require('node-addon-api').include\")",
        "<!(node -p \"require('node-addon-api').include_dir\")",
        "deps/wxWidgets/dist/include/wx-3.3",
        "deps/wxWidgets/dist/lib/wx/include/gtk3-unicode-static-3.3",
        "deps/wxWidgets/dist/lib"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags_cc": [
        "-g", "-O0",
        "<!(/usr/bin/env wx-config --cxxflags)",
        "-std=c++17",
        "-frtti",
        "-fexceptions"],
      "defines": ["NAPI_CPP_EXCEPTIONS", "WXWIN_COMPATIBILITY_2_8=1", "wxUSE_UNSAFE_WXSTRING_CONV=1"],
      "libraries": [
        "-Wl,--whole-archive",
        "<!(/usr/bin/env wx-config --libs)",
        "-Wl,--no-whole-archive",
        "-lX11", "-lXxf86vm", "-lGL", "-lGLU", "-lpthread", "-ldl", "-lm"
      ]
    }
  ]
}
