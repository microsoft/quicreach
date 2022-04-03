# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

add_executable(quicreach quicreach.cpp)
target_link_libraries(quicreach PRIVATE inc warnings msquic)
install(TARGETS quicreach EXPORT quicreach DESTINATION lib)
