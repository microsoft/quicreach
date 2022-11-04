# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if (WIN32)
    add_executable(quicreach quicreach.cpp quicreach.rc)
else()
    add_executable(quicreach quicreach.cpp)
endif()
target_link_libraries(quicreach PRIVATE inc warnings msquic)
if (NOT BUILD_SHARED_LIBS)
    target_link_libraries(quicreach PRIVATE base_link)
endif()
install(TARGETS quicreach EXPORT quicreach DESTINATION lib)
