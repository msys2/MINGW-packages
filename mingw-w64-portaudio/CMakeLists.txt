PROJECT(portaudiocpp)

set(PA_CPP_SOURCES
  source/portaudiocpp/BlockingStream.cxx
  source/portaudiocpp/CallbackInterface.cxx
  source/portaudiocpp/CallbackStream.cxx
  source/portaudiocpp/CFunCallbackStream.cxx
  source/portaudiocpp/CppFunCallbackStream.cxx
  source/portaudiocpp/Device.cxx
  source/portaudiocpp/DirectionSpecificStreamParameters.cxx
  source/portaudiocpp/Exception.cxx
  source/portaudiocpp/HostApi.cxx
  source/portaudiocpp/InterfaceCallbackStream.cxx
  source/portaudiocpp/MemFunCallbackStream.cxx
  source/portaudiocpp/Stream.cxx
  source/portaudiocpp/StreamParameters.cxx
  source/portaudiocpp/System.cxx
  source/portaudiocpp/SystemDeviceIterator.cxx
  source/portaudiocpp/SystemHostApiIterator.cxx
)

set(PA_CPP_INCLUDES
  include/portaudiocpp/AutoSystem.hxx
  include/portaudiocpp/BlockingStream.hxx
  include/portaudiocpp/CallbackInterface.hxx
  include/portaudiocpp/CallbackStream.hxx
  include/portaudiocpp/CFunCallbackStream.hxx
  include/portaudiocpp/CppFunCallbackStream.hxx
  include/portaudiocpp/Device.hxx
  include/portaudiocpp/DirectionSpecificStreamParameters.hxx
  include/portaudiocpp/Exception.hxx
  include/portaudiocpp/HostApi.hxx
  include/portaudiocpp/InterfaceCallbackStream.hxx
  include/portaudiocpp/MemFunCallbackStream.hxx
  include/portaudiocpp/PortAudioCpp.hxx
  include/portaudiocpp/SampleDataFormat.hxx
  include/portaudiocpp/Stream.hxx
  include/portaudiocpp/StreamParameters.hxx
  include/portaudiocpp/SystemDeviceIterator.hxx
  include/portaudiocpp/SystemHostApiIterator.hxx
  include/portaudiocpp/System.hxx
)

IF(WIN32)
  if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
    FIND_PACKAGE(ASIOSDK)
    IF(ASIOSDK_FOUND)
      OPTION(PA_USE_ASIO "Enable support for ASIO" ON)
    ELSE()
      OPTION(PA_USE_ASIO "Enable support for ASIO" OFF)
    ENDIF()
  endif()

  if(PA_USE_ASIO)
    list(APPEND PA_CPP_INCLUDES include/portaudiocpp/AsioDeviceAdapter.hxx)
    list(APPEND PA_CPP_SOURCES source/portaudiocpp/AsioDeviceAdapter.cxx)
  endif()
ENDIF(WIN32)

IF(PA_BUILD_SHARED)
  add_library(portaudiocpp SHARED ${PA_CPP_SOURCES})
  target_link_libraries(portaudiocpp PUBLIC portaudio)
  target_include_directories(portaudiocpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
ENDIF(PA_BUILD_SHARED)
IF(PA_BUILD_STATIC)
  add_library(portaudiocpp_static STATIC ${PA_CPP_SOURCES})
  target_link_libraries(portaudiocpp_static PUBLIC portaudio)
  target_include_directories(portaudiocpp_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
  IF(NOT PA_LIBNAME_ADD_SUFFIX)
    SET_PROPERTY(TARGET portaudiocpp_static PROPERTY OUTPUT_NAME portaudiocpp)
  ENDIF()
ENDIF(PA_BUILD_STATIC)

include(GNUInstallDirs)
install(TARGETS portaudiocpp portaudiocpp_static
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES ${PA_CPP_INCLUDES} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/portaudiocpp")

if(WIN32)
    install(FILES include/portaudiocpp/AsioDeviceAdapter.hxx DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/portaudiocpp")
endif()

set(PORTAUDIOCPP_VERSION 12)
configure_file(cmake/portaudiocpp.pc.in ${CMAKE_CURRENT_BINARY_DIR}/portaudiocpp.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/portaudiocpp.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
