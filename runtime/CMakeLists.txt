# NovelMind Runtime
# Console-based runtime for playing visual novels

# Create the runtime executable
add_executable(novelmind_runtime
    src/main.cpp
)

target_link_libraries(novelmind_runtime
    PRIVATE
        engine_core
        novelmind_compiler_options
)

# Set output name
set_target_properties(novelmind_runtime PROPERTIES
    OUTPUT_NAME "novelmind_runtime"
)

# Installation
install(TARGETS novelmind_runtime
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
