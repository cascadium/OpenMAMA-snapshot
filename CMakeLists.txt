cmake_minimum_required(VERSION 3.0)
project(OpenMAMA_snapshot)

set(CMAKE_CXX_STANDARD 14)

set(OPENMAMA_ROOT /home/fquinn/SourceExternal/github/fquinner/OpenMAMA/build/install)

find_package(Poco REQUIRED Foundation Util Net)
include_directories("${OPENMAMA_ROOT}/include")
include_directories(.)
link_directories("${OPENMAMA_ROOT}/lib")

add_executable(cascadium-snapshot
        main.cpp
        MainApplication.cpp
        MainApplication.h
        subsystems/SubsystemServletRouter.cpp
        subsystems/SubsystemServletRouter.h
        subsystems/SubsystemOpenMama.cpp
        subsystems/SubsystemOpenMama.h
        subsystems/openmama/OpenMamaDictionaryRequestor.h
        subsystems/openmama/OpenMamaStoreMessageListener.cpp
        subsystems/openmama/OpenMamaStoreMessageListener.h
        servlets/ServletOpenMamaSnapshot.cpp
        servlets/ServletOpenMamaSnapshot.h
        servlets/ServletRequestHandlerFactory.cpp
        servlets/ServletRequestHandlerFactory.h OptionNames.h subsystems/SubsystemCommon.h)

#target_link_libraries(cascadium-snapshot
#        libmamacmd
#        libmamacppmd
#        libmamdamd
#        libmamdabookmd
#        Poco::Foundation
#        Poco::Util
#        Poco::Net)

target_link_libraries(cascadium-snapshot
        mama
        mamacpp
        mamda
        mamdabook
        Poco::Foundation
        Poco::Util
        Poco::Net)
