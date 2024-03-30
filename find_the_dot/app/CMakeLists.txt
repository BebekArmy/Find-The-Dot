# Build the app, using the HAL

include_directories(include)
include_directories(../pru-as4/)
file(GLOB MY_SOURCES "src/*.c")
add_executable(find_the_dot ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(find_the_dot LINK_PRIVATE hal)

# Copy executable to final location (change `hello_world` to project name as needed)
add_custom_command(TARGET find_the_dot POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:find_the_dot>"
     "~/cmpt433/public/Assignment 4/find_the_dot" 
  COMMENT "Copying ARM executable to public NFS directory")
