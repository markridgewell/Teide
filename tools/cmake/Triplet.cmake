
if(NOT DEFINED TEIDE_SANITIZER)
    message(SEND_ERROR "Triplet determination requires cache variable TEIDE_SANITIZER, which was not defined.")
endif()

if(LINUX)
    if(TEIDE_SANITIZER STREQUAL "UBSAN")
        set(VCPKG_TARGET_TRIPLET "x64-linux-ubsan")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-linux")
    endif()
elseif(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md")
endif()
