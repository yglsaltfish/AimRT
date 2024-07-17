#!/bin/bash



protoc_cmd=./protoc
protocols_dir=../src/protocols/example/

${protoc_cmd} -I${protocols_dir} --python_out=./ ${protocols_dir}/common.proto ${protocols_dir}/rpc.proto
${protoc_cmd} -I${protocols_dir} --aimrt_rpc_out=./ --plugin=protoc-gen-aimrt_rpc=$(which protoc_plugin_py_gen_aimrt_py_rpc) ${protocols_dir}/rpc.proto
