#!/bin/bash

data='{
	"module_parameter_map": {
		"ParameterModule": {
			"value": {
				"test_bool": {
					"bool_value": true
				},
				"test_int": {
					"int_value": "-1024"
				},
				"test_uint": {
					"uint_value": "1024"
				},
				"test_double": {
					"double_value": 999.999
				},
				"test_string": {
					"string_value": "hello world"
				},
				"test_byte_array": {
					"bytes_value": "aGVsbG8gd29ybGQ="
				},
				"test_bool_array": {
					"bool_array": {
						"items": [false,true,false,true]
					}
				},
				"test_int8_array": {
					"bytes_value": "aGVsbG8gd29ybGQ="
				},
				"test_uint8_array": {
					"bytes_value": "aGVsbG8gd29ybGQ="
				},
				"test_int16_array": {
					"int_array": {
						"items": ["0","-1","-2","-3","-4","-5"]
					}
				},
				"test_uint16_array": {
					"uint_array": {
						"items": ["0","1","2","3","4","5"]
					}
				},
				"test_int32_array": {
					"int_array": {
						"items": ["0","-1","-2","-3","-4","-5"]
					}
				},
				"test_uint32_array": {
					"uint_array": {
						"items": ["0","1","2","3","4","5"]
					}
				},
				"test_int64_array": {
					"int_array": {
						"items": ["0","-1","-2","-3","-4","-5"]
					}
				},
				"test_uint64_array": {
					"uint_array": {
						"items": ["0","1","2","3","4","5"]
					}
				},
				"test_float_array": {
					"double_array": {
						"items": [0.1,1.2,2.3,3.4,4.5,5.6]
					}
				},
				"test_double_array": {
					"double_array": {
						"items": [0.1,1.2,2.3,3.4,4.5,5.6]
					}
				},
                "test_string_array": {
					"string_array": {
						"items": ["aaaaa","bbbbb","ccccc","ddddd","eeeee","fffff"]
					}
				}
			}
		}
	}
}'


curl -i \
    -H 'content-type:application/json' \
    -X POST 'http://127.0.0.1:50080/rpc/aimrt.protocols.parameter_plugin.ParameterService/Load' \
    -d "$data"
