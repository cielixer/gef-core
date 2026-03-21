target_sources(gef
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gef/core/module/AtomicModule.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gef/core/module/ModuleRegistry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gef/core/system/System.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gef/core/scheduler/Scheduler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/gef/core/system/Flow.cpp
)
