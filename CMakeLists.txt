cmake_minimum_required(VERSION 3.0)
project(OpenMAMA_snapshot)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

set(CMAKE_CXX_STANDARD 14)
set(OPENMAMA_ROOT /opt/openmama)
message("OPENMAMA_ROOT=${OPENMAMA_ROOT}")
find_package(Poco REQUIRED Foundation Util Net)
include_directories("${OPENMAMA_ROOT}/include")
include_directories(.)
link_directories("${OPENMAMA_ROOT}/lib")

add_executable(cascadium-snapshot
        main.cpp
        MainApplication.cpp
        MainApplication.h
        OptionNames.h
        subsystems/SubsystemServletRouter.cpp
        subsystems/SubsystemServletRouter.h
        subsystems/SubsystemOpenMama.cpp
        subsystems/SubsystemOpenMama.h
        subsystems/openmama/OpenMamaDictionaryRequestor.h
        subsystems/openmama/OpenMamaStoreMessageListener.cpp
        subsystems/openmama/OpenMamaStoreMessageListener.h
        servlets/ServletOpenMamaServerSentEvents.cpp
        servlets/ServletOpenMamaServerSentEvents.h
        servlets/ServletOpenMamaSnapshot.cpp
        servlets/ServletOpenMamaSnapshot.h
        servlets/ServletRequestHandlerFactory.cpp
        servlets/ServletRequestHandlerFactory.h
        subsystems/SubsystemCommon.h
)

target_link_libraries(cascadium-snapshot
        mama
        mamacpp
        mamda
        mamdabook
        ${Poco_Foundation_LIBRARY}
        ${Poco_Util_LIBRARY}
        ${Poco_Net_LIBRARY})
