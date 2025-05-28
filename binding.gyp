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
        
        "deps/wxWidgets/build-static/dist/include/wx-3.2",
        "deps/wxWidgets/build-static/dist/lib/wx/include/gtk3-unicode-static-3.2"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags_cc": [
        "-g", "-O0",
        "-D_FILE_OFFSET_BITS=64",
        "-D__WXGTK__",
        "-std=c++17",
        "-frtti",
        "-fexceptions",
        "-pthread"],
      "defines": ["NAPI_CPP_EXCEPTIONS", "WXWIN_COMPATIBILITY_2_8=1", "wxUSE_UNSAFE_WXSTRING_CONV=1"],
      "libraries": [
        "-Wl,--whole-archive",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwx_baseu-3.2",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwx_baseu_net-3.2",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwx_baseu_xml-3.2",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwxexpat-3.2",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwxregexu-3.2",
        "-Ldeps/wxWidgets/build-static/dist/lib/libwxzlib-3.2",
        "-Wl,--no-whole-archive",
        "-lpthread", "-ldl", "-lm"
      ]
    }
  ]
}
