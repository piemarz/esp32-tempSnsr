# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    # list the source files of this component
    SRCS
    main.c
    deepSleep/deepSleep.c
    driverDHT11Interface/driverDHT11Interface.c
    
    # optional, add here public include directories
    INCLUDE_DIRS
    .
    ./deepSleep
    ./driverDHT11Interface
    ../dht11/src
    
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
