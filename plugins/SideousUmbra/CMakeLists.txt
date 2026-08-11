dpf_add_plugin(sideous-umbra
  TARGETS lv2 vst3 clap jack
  UI_TYPE cairo
  FILES_DSP
      SideousUmbraPlugin.cpp
  FILES_UI
      SideousUmbraUI.cpp)

target_include_directories(sideous-umbra PUBLIC ".")
