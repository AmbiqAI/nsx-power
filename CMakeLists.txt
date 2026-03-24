if(NOT DEFINED NSX_BOARD_FLAGS_TARGET)
    message(FATAL_ERROR "Board must define NSX_BOARD_FLAGS_TARGET before ns-power is added.")
endif()
if(NOT DEFINED NSX_SOC_FAMILY)
    message(FATAL_ERROR "NSX_SOC_FAMILY must be defined before ns-power is added.")
endif()

set(NSX_POWER_ARCH_DIR "")
if(NSX_SOC_FAMILY STREQUAL "apollo3" OR NSX_SOC_FAMILY STREQUAL "apollo3p")
    set(NSX_POWER_ARCH_DIR "apollo3")
elseif(NSX_SOC_FAMILY STREQUAL "apollo4l" OR NSX_SOC_FAMILY STREQUAL "apollo4p")
    set(NSX_POWER_ARCH_DIR "apollo4")
elseif(NSX_SOC_FAMILY STREQUAL "apollo5b" OR NSX_SOC_FAMILY STREQUAL "apollo510" OR NSX_SOC_FAMILY STREQUAL "apollo510b")
    set(NSX_POWER_ARCH_DIR "apollo5")
elseif(NSX_SOC_FAMILY STREQUAL "apollo330P")
    set(NSX_POWER_ARCH_DIR "apollo330")
else()
    message(FATAL_ERROR "Unsupported NSX_SOC_FAMILY for ns-power: ${NSX_SOC_FAMILY}")
endif()

set(NSX_POWER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/ns_power.c
    ${CMAKE_CURRENT_LIST_DIR}/src/${NSX_POWER_ARCH_DIR}/ns_power.c
)

if(NSX_POWER_ARCH_DIR STREQUAL "apollo4")
    list(APPEND NSX_POWER_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/apollo4/ns_tempco.c
    )
endif()

add_library(nsx_power STATIC ${NSX_POWER_SOURCES})
set_target_properties(nsx_power PROPERTIES EXPORT_NAME power)
add_library(nsx::power ALIAS nsx_power)

target_link_libraries(nsx_power PUBLIC
    ${NSX_BOARD_FLAGS_TARGET}
    nsx_soc_hal
    nsx_core
    nsx_harness
    nsx_utils
)

target_include_directories(nsx_power PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/includes-api>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/${NSX_POWER_ARCH_DIR}>
    $<BUILD_INTERFACE:${NSX_ROOT}/modules/nsx-core/includes-api>
    $<BUILD_INTERFACE:${NSX_ROOT}/modules/nsx-harness/includes-api>
    $<BUILD_INTERFACE:${NSX_ROOT}/modules/nsx-utils/includes-api>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-power/includes-api>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-power/src/${NSX_POWER_ARCH_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-core/includes-api>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-harness/includes-api>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-utils/includes-api>
)

install(DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/includes-api
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-power
)
install(DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/src/${NSX_POWER_ARCH_DIR}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-power/src
)

install(TARGETS
    nsx_power
    EXPORT nsxTargets
)
