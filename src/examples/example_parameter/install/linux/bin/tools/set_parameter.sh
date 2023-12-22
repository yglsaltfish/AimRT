#!/bin/bash

echo -e "Set parameter 'not_exist_parameter':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "not_exist_parameter", "parameter_value":{"null_value":{}}}'

echo -e "\n\nSet parameter 'test_bool':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_bool", "parameter_value":{"bool_value":true}}'

echo -e "\n\nSet parameter 'test_int':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_int", "parameter_value":{"int_value":"-1024"}}'

echo -e "\n\nSet parameter 'test_uint':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_uint", "parameter_value":{"uint_value":"1024"}}'

echo -e "\n\nSet parameter 'test_double':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_double", "parameter_value":{"double_value":999.999}}'

echo -e "\n\nSet parameter 'test_string':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_string", "parameter_value":{"string_value":"hello world"}}'

echo -e "\n\nSet parameter 'test_byte_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_byte_array", "parameter_value":{"bytes_value":"aGVsbG8gd29ybGQ="}}'

echo -e "\n\nSet parameter 'test_bool_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_bool_array", "parameter_value":{"bool_array":{"items":[false,true,false,true]}}}'

echo -e "\n\nSet parameter 'test_int8_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_int8_array", "parameter_value":{"bytes_value":"aGVsbG8gd29ybGQ="}}'

echo -e "\n\nSet parameter 'test_int16_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_int16_array", "parameter_value":{"int_array":{"items":["0","-1","-2","-3","-4","-5"]}}}'

echo -e "\n\nSet parameter 'test_int32_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_int32_array", "parameter_value":{"int_array":{"items":["0","-1","-2","-3","-4","-5"]}}}'

echo -e "\n\nSet parameter 'test_int64_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_int64_array", "parameter_value":{"int_array":{"items":["0","-1","-2","-3","-4","-5"]}}}'

echo -e "\n\nSet parameter 'test_uint8_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_uint8_array", "parameter_value":{"bytes_value":"aGVsbG8gd29ybGQ="}}'

echo -e "\n\nSet parameter 'test_uint16_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_uint16_array", "parameter_value":{"uint_array":{"items":["0","1","2","3","4","5"]}}}'

echo -e "\n\nSet parameter 'test_uint32_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_uint32_array", "parameter_value":{"uint_array":{"items":["0","1","2","3","4","5"]}}}'

echo -e "\n\nSet parameter 'test_uint64_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_uint64_array", "parameter_value":{"uint_array":{"items":["0","1","2","3","4","5"]}}}'

echo -e "\n\nSet parameter 'test_float_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_float_array", "parameter_value":{"double_array":{"items":[0.1,1.2,2.3,3.4,4.5,5.6]}}}'

echo -e "\n\nSet parameter 'test_double_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_double_array", "parameter_value":{"double_array":{"items":[0.1,1.2,2.3,3.4,4.5,5.6]}}}'

echo -e "\n\nSet parameter 'test_string_array':"
curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Set' \
    -d '{"module_name": "ParameterModule", "parameter_name": "test_string_array", "parameter_value":{"string_array":{"items":["aaaaa","bbbbb","ccccc","ddddd","eeeee","fffff"]}}}'