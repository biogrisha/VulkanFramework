# Create a library target
add_library(MyRTTI STATIC 
    src/MyRTTI.cpp
    src/LocId.cpp
    src/ComponentInterface/ComponentInterface.cpp
    include/MyRTTI.h
    include/LocId.h
    include/ComponentInterface/ComponentInterface.h
)

# Set include directories
target_include_directories(MyRTTI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
