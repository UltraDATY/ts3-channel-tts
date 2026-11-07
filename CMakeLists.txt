cmake_minimum_required(VERSION 3.20)
project(ChannelTTS VERSION 1.3.0 LANGUAGES CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "This plugin requires Windows x64")
endif()
add_library(channel_tts SHARED src/plugin.cpp)
target_compile_features(channel_tts PRIVATE cxx_std_17)
target_include_directories(channel_tts PRIVATE sdk/include)
target_compile_definitions(channel_tts PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(channel_tts PRIVATE sapi ole32 uuid)
set_target_properties(channel_tts PROPERTIES PREFIX "" OUTPUT_NAME "channel_tts_win64")
if(MSVC)
  target_compile_options(channel_tts PRIVATE /W4 /EHsc /utf-8)
  set_property(TARGET channel_tts PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
install(TARGETS channel_tts RUNTIME DESTINATION plugins)



