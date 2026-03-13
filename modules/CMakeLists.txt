file(GLOB MODULE_SOURCES CONFIGURE_DEPENDS "*.cpp")

foreach(SOURCE ${MODULE_SOURCES})
    get_filename_component(MODULE_NAME ${SOURCE} NAME_WE)
    add_gef_module(${MODULE_NAME} ${SOURCE})
endforeach()
