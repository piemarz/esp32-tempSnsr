# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    # list the source files of this component
    SRCS
    main.c
    deepSleep/deepSleep.c
    dht/dhtDriver.c
    
    # optional, add here public include directories
    INCLUDE_DIRS
    .
    ./deepSleep
    ./dht
    
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES            # optional, list the public requirements (component names)
    PRIV_REQUIRES       # optional, list the private requirements
)
