# Copyright (c) 2023, AgiBot Inc.
# All rights reserved.

set_namespace()

add_subdirectory(helloworld)

if(AIMRT_BUILD_WITH_PROTOBUF)
  add_subdirectory(protobuf_channel)
  add_subdirectory(protobuf_rpc)
endif()
