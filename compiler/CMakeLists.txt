# NovelMind Script Compiler (nmc)
# Command-line tool for compiling NM Script files to bytecode

# Create the compiler executable
add_executable(nmc
    src/main.cpp
)

target_link_libraries(nmc
    PRIVATE
        engine_core
        novelmind_compiler_options
)

# Set output name
set_target_properties(nmc PROPERTIES
    OUTPUT_NAME "nmc"
)

# Installation
install(TARGETS nmc
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
