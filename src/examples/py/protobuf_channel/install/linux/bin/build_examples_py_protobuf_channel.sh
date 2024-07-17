#!/bin/bash

protoc_cmd=./protoc
protocols_dir=../src/protocols/example/

${protoc_cmd} -I${protocols_dir} --python_out=./ ${protocols_dir}/event.proto


