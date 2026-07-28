# Create a library target
add_library(MathLibrary STATIC 
    src/MathLibrary.cpp
    include/MathLibrary.h
)

# Set include directories
target_include_directories(MathLibrary
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(MathLibrary PRIVATE /fsanitize=address)
target_link_options(MathLibrary PRIVATE /fsanitize=address)
