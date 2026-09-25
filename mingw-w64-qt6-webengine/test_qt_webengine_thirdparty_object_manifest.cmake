if(NOT DEFINED HELPER OR NOT EXISTS "${HELPER}")
    message(FATAL_ERROR "HELPER must name the object-manifest helper")
endif()
if(NOT DEFINED WORKDIR)
    set(WORKDIR "${CMAKE_CURRENT_BINARY_DIR}/qt-object-manifest-test")
endif()

include("${HELPER}")
file(MAKE_DIRECTORY "${WORKDIR}")
set(manifest "${WORKDIR}/packaged-objects.rsp")
set(response "${WORKDIR}/objects.rsp")
file(WRITE "${manifest}" [=[
"D:\package\obj\mojo\core\channel.o"
/ucrt64/lib/qt6-webengine-private/obj/services/device/device.o
D:/package/obj/components/signin/account_info.o
D:/package/obj/media/absent_from_response.o
]=])
file(WRITE "${response}" [=[
"D:/build/obj/mojo/core/channel.o"
D:\build\obj\services\device\device.o
/clang64/build/obj/components/signin/account_info.o
"D:/build/obj/QtWebEngineCore/browser_context.o"
D:/build/obj/third_party/blink/renderer/retained_blink_renderer.o
]=])

qwe_classify_object_response(
    "${response}" "${manifest}" retained_lines retained_paths stats)
if(NOT stats STREQUAL "5;3;2;1")
    message(FATAL_ERROR "Unexpected object classification counters: ${stats}")
endif()

list(FIND retained_paths
    "D:/build/obj/QtWebEngineCore/browser_context.o" first_party_index)
if(first_party_index EQUAL -1)
    message(FATAL_ERROR "The first-party QtWebEngineCore object was removed")
endif()
list(FIND retained_paths
    "D:/build/obj/third_party/blink/renderer/retained_blink_renderer.o"
    blink_renderer_index)
if(blink_renderer_index EQUAL -1)
    message(FATAL_ERROR "A non-manifest Blink renderer object was removed")
endif()

foreach(supplied_path IN ITEMS
        "D:/build/obj/mojo/core/channel.o"
        "D:/build/obj/services/device/device.o"
        "/clang64/build/obj/components/signin/account_info.o")
    list(FIND retained_paths "${supplied_path}" supplied_index)
    if(NOT supplied_index EQUAL -1)
        message(FATAL_ERROR "Manifest object was retained: ${supplied_path}")
    endif()
endforeach()

file(REMOVE_RECURSE "${WORKDIR}")
message(STATUS "QtWebEngine third-party object manifest fixture passed")
