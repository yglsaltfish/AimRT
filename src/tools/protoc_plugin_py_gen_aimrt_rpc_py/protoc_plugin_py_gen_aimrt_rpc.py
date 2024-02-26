#! /usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

from google.protobuf.compiler import plugin_pb2 as plugin
from google.protobuf.compiler.plugin_pb2 import CodeGeneratorRequest as CodeGeneratorRequest
from google.protobuf.compiler.plugin_pb2 import CodeGeneratorResponse as CodeGeneratorResponse
from google.protobuf.descriptor_pb2 import FileDescriptorProto


class AimRTCodeGenerator(object):
    t_pyfile_one_service_func: str = r"""
    def {{rpc_func_name}}(self, ctx_ref, req):
        return (aimrt_py.RpcStatus(aimrt_py.RpcStatusRetCode.SVR_NOT_IMPLEMENTED), {{full_rpc_rsp_py_name}}())
"""

    t_pyfile_one_service_register_func: str = r"""
        # {{rpc_func_name}}
        {{simple_rpc_req_name}}_aimrt_ts = aimrt_py.TypeSupport()
        {{simple_rpc_req_name}}_aimrt_ts.SetTypeName("pb:" + {{full_rpc_req_py_name}}.DESCRIPTOR.full_name)
        {{simple_rpc_req_name}}_aimrt_ts.SetSerializationTypesSupportedList(["pb", "json"])

        {{simple_rpc_rsp_name}}_aimrt_ts = aimrt_py.TypeSupport()
        {{simple_rpc_rsp_name}}_aimrt_ts.SetTypeName("pb:" + {{full_rpc_rsp_py_name}}.DESCRIPTOR.full_name)
        {{simple_rpc_rsp_name}}_aimrt_ts.SetSerializationTypesSupportedList(["pb", "json"])

        def {{rpc_func_name}}AdapterFunc(ctx_ref, req_str):
            serialization_type = ctx_ref.GetSerializationType()

            try:
                req = {{full_rpc_req_py_name}}()
                if(serialization_type == "pb"):
                    req.ParseFromString(req_str)
                elif(serialization_type == "json"):
                    google.protobuf.json_format.Parse(req_str, req)
                else:
                    return (aimrt_py.RpcStatus(aimrt_py.RpcStatusRetCode.SVR_INVALID_SERIALIZATION_TYPE), "")
            except Exception as e:
                return (aimrt_py.RpcStatus(aimrt_py.RpcStatusRetCode.SVR_DESERIALIZATION_FAILDE), "")

            try:
                st, rsp = self.{{rpc_func_name}}(ctx_ref, req)
            except Exception as e:
                return (aimrt_py.RpcStatus(aimrt_py.RpcStatusRetCode.SVR_HANDLE_FAILDE), "")

            try:
                rsp_str = ""
                if(serialization_type == "pb"):
                    rsp_str = rsp.SerializeToString()
                elif(serialization_type == "json"):
                    rsp_str = google.protobuf.json_format.MessageToJson(rsp)
                else:
                    return (aimrt_py.RpcStatus(aimrt_py.RpcStatusRetCode.SVR_INVALID_SERIALIZATION_TYPE), "")
            except Exception as e:
                return (aimrt_py.RpcStatus(aimrt_py.RpcStatusRetCode.SVR_SERIALIZATION_FAILDE), "")

            return (st, rsp_str)

        self.RegisterServiceFunc("pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
                                 {{simple_rpc_req_name}}_aimrt_ts, {{simple_rpc_rsp_name}}_aimrt_ts, {{rpc_func_name}}AdapterFunc)
"""

    t_pyfile_one_service_class: str = r"""
class {{service_name}}(aimrt_py.ServiceBase):
    def __init__(self):
        super().__init__()
{{pyfile_service_register_func}}
{{pyfile_service_func}}
"""

    t_pyfile_one_service_proxy_func: str = r"""
    def {{rpc_func_name}}(self, ctx_ref, req):
        ctx = aimrt_py.RpcContext()

        if(ctx_ref and ctx_ref.GetSerializationType() == ""):
            ctx_ref.SetSerializationType("pb")
        else:
            ctx = self.rpc_handle_ref.NewContextSharedPtr()
            ctx_ref = aimrt_py.RpcContextRef(ctx)
            ctx_ref.SetSerializationType("pb")

        serialization_type = ctx_ref.GetSerializationType()

        rsp = {{full_rpc_rsp_py_name}}()

        try:
            req_str = ""
            if(serialization_type == "pb"):
                req_str = req.SerializeToString()
            elif(serialization_type == "json"):
                req_str = google.protobuf.json_format.MessageToJson(req)
            else:
                return (aimrt_py.RpcStatus(aimrt_py.RpcStatusRetCode.CLI_INVALID_SERIALIZATION_TYPE), rsp)
        except Exception as e:
            return (aimrt_py.RpcStatus(aimrt_py.RpcStatusRetCode.CLI_SERIALIZATION_FAILDE), rsp)

        status, rsp_str = self.rpc_handle_ref.Invoke("pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
                                                     ctx_ref, req_str)

        try:
            if(serialization_type == "pb"):
                rsp.ParseFromString(rsp_str)
            elif(serialization_type == "json"):
                google.protobuf.json_format.Parse(rsp_str, rsp)
            else:
                return (aimrt_py.RpcStatus(aimrt_py.RpcStatusRetCode.CLI_INVALID_SERIALIZATION_TYPE), rsp)
        except Exception as e:
            return (aimrt_py.RpcStatus(aimrt_py.RpcStatusRetCode.CLI_DESERIALIZATION_FAILDE), rsp)

        return (status, rsp)
"""

    t_pyfile_one_service_proxy_register_func: str = r"""
        # {{rpc_func_name}}
        {{simple_rpc_req_name}}_aimrt_ts = aimrt_py.TypeSupport()
        {{simple_rpc_req_name}}_aimrt_ts.SetTypeName("pb:" + {{full_rpc_req_py_name}}.DESCRIPTOR.full_name)
        {{simple_rpc_req_name}}_aimrt_ts.SetSerializationTypesSupportedList(["pb", "json"])

        {{simple_rpc_rsp_name}}_aimrt_ts = aimrt_py.TypeSupport()
        {{simple_rpc_rsp_name}}_aimrt_ts.SetTypeName("pb:" + {{full_rpc_rsp_py_name}}.DESCRIPTOR.full_name)
        {{simple_rpc_rsp_name}}_aimrt_ts.SetSerializationTypesSupportedList(["pb", "json"])

        ret = rpc_handle.RegisterClientFunc("pb:/{{package_name}}.{{service_name}}/{{rpc_func_name}}",
                                            {{simple_rpc_req_name}}_aimrt_ts, {{simple_rpc_rsp_name}}_aimrt_ts)
        if(not ret):
            return False
"""

    t_pyfile_one_service_proxy_class: str = r"""
class {{service_name}}Proxy:
    def __init__(self, rpc_handle_ref=aimrt_py.RpcHandleRef()):
        self.rpc_handle_ref = rpc_handle_ref
{{pyfile_service_proxy_func}}
    @staticmethod
    def RegisterClientFunc(rpc_handle):
{{pyfile_service_proxy_register_func}}
        return True
"""

    t_pyfile: str = r"""# This file was generated by protoc-gen-aimrt_rpc which is a self-defined pb compiler plugin, do not edit it!!!

import aimrt_py
import google.protobuf
import {{py_package_name}}
{{pyfile_import_dependency_py_package}}

{{pyfile_service_class}}
{{pyfile_service_proxy_class}}
"""

    @staticmethod
    def gen_simple_name_str(ns: str) -> str:
        return ns.split(".")[-1]

    def generate(self, request: CodeGeneratorRequest) -> CodeGeneratorResponse:
        """Generate code for the given request"""

        # create message type py name dict
        message_type_py_name_dict = {}
        for proto_file in request.proto_file:
            file_name: str = proto_file.name
            package_name: str = proto_file.package
            py_package_name: str = file_name.replace('.proto', '_pb2').replace("/", ".")

            for message_type in proto_file.message_type:
                message_type_full_name = "." + package_name + "." + message_type.name
                message_type_py_name = py_package_name + "." + message_type.name
                message_type_py_name_dict[message_type_full_name] = message_type_py_name

        # Generate code for each file
        response: CodeGeneratorResponse = CodeGeneratorResponse()
        for proto_file in request.proto_file:
            if len(proto_file.service) == 0:
                continue

            file_name: str = proto_file.name
            package_name: str = proto_file.package
            py_package_name: str = file_name.replace('.proto', '_pb2').replace("/", ".")
            aimrt_rpc_py_file_name: str = file_name.replace('.proto', '_aimrt_rpc_pb2.py')

            pyfile_import_dependency_py_package: str = ""
            for dependency_proto_file in proto_file.dependency:
                import_dependency_py_package: str = "import " + dependency_proto_file.replace('.proto', '_pb2')
                pyfile_import_dependency_py_package = pyfile_import_dependency_py_package + import_dependency_py_package + "\n"

            pyfile_service_class: str = ""
            pyfile_service_proxy_class: str = ""

            for ii in range(0, len(proto_file.service)):
                service: ServiceDescriptorProto = proto_file.service[ii]
                if ii != 0:
                    pyfile_service_class += "\n"
                    pyfile_service_proxy_class += "\n"

                service_name: str = service.name

                pyfile_service_register_func: str = ""
                pyfile_service_func: str = ""
                pyfile_service_proxy_func: str = ""
                pyfile_service_proxy_register_func: str = ""

                for jj in range(0, len(service.method)):
                    method: MethodDescriptorProto = service.method[jj]

                    if jj != 0:
                        pyfile_service_register_func += "\n"
                        pyfile_service_func += "\n"
                        pyfile_service_proxy_func += "\n"
                        pyfile_service_proxy_register_func += "\n"

                    rpc_func_name: str = method.name
                    simple_rpc_req_name: str = self.gen_simple_name_str(method.input_type)
                    simple_rpc_rsp_name: str = self.gen_simple_name_str(method.output_type)
                    full_rpc_req_py_name: str = message_type_py_name_dict[method.input_type]
                    full_rpc_rsp_py_name: str = message_type_py_name_dict[method.output_type]

                    pyfile_one_service_register_func: str = self.t_pyfile_one_service_register_func \
                        .replace("{{simple_rpc_req_name}}", simple_rpc_req_name) \
                        .replace("{{simple_rpc_rsp_name}}", simple_rpc_rsp_name) \
                        .replace("{{full_rpc_req_py_name}}", full_rpc_req_py_name) \
                        .replace("{{full_rpc_rsp_py_name}}", full_rpc_rsp_py_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)
                    pyfile_service_register_func += pyfile_one_service_register_func

                    pyfile_one_service_func: str = self.t_pyfile_one_service_func \
                        .replace("{{simple_rpc_req_name}}", simple_rpc_req_name) \
                        .replace("{{simple_rpc_rsp_name}}", simple_rpc_rsp_name) \
                        .replace("{{full_rpc_req_py_name}}", full_rpc_req_py_name) \
                        .replace("{{full_rpc_rsp_py_name}}", full_rpc_rsp_py_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)
                    pyfile_service_func += pyfile_one_service_func

                    pyfile_one_service_proxy_func: str = self.t_pyfile_one_service_proxy_func \
                        .replace("{{simple_rpc_req_name}}", simple_rpc_req_name) \
                        .replace("{{simple_rpc_rsp_name}}", simple_rpc_rsp_name) \
                        .replace("{{full_rpc_req_py_name}}", full_rpc_req_py_name) \
                        .replace("{{full_rpc_rsp_py_name}}", full_rpc_rsp_py_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)
                    pyfile_service_proxy_func += pyfile_one_service_proxy_func

                    pyfile_one_service_proxy_register_func: str = self.t_pyfile_one_service_proxy_register_func \
                        .replace("{{simple_rpc_req_name}}", simple_rpc_req_name) \
                        .replace("{{simple_rpc_rsp_name}}", simple_rpc_rsp_name) \
                        .replace("{{full_rpc_req_py_name}}", full_rpc_req_py_name) \
                        .replace("{{full_rpc_rsp_py_name}}", full_rpc_rsp_py_name) \
                        .replace("{{rpc_func_name}}", rpc_func_name)
                    pyfile_service_proxy_register_func += pyfile_one_service_proxy_register_func

                pyfile_one_service_class: str = self.t_pyfile_one_service_class \
                    .replace("{{pyfile_service_register_func}}", pyfile_service_register_func) \
                    .replace("{{pyfile_service_func}}", pyfile_service_func) \
                    .replace("{{service_name}}", service_name)
                pyfile_service_class += pyfile_one_service_class

                pyfile_one_service_proxy_class: str = self.t_pyfile_one_service_proxy_class \
                    .replace("{{pyfile_service_proxy_func}}", pyfile_service_proxy_func) \
                    .replace("{{pyfile_service_proxy_register_func}}", pyfile_service_proxy_register_func) \
                    .replace("{{service_name}}", service_name)
                pyfile_service_proxy_class += pyfile_one_service_proxy_class

            # pyfile
            pyfile: CodeGeneratorResponse.File = CodeGeneratorResponse.File()
            pyfile.name = aimrt_rpc_py_file_name
            pyfile.content = self.t_pyfile \
                .replace("{{pyfile_import_dependency_py_package}}", pyfile_import_dependency_py_package) \
                .replace("{{pyfile_service_class}}", pyfile_service_class) \
                .replace("{{pyfile_service_proxy_class}}", pyfile_service_proxy_class) \
                .replace("{{py_package_name}}", py_package_name) \
                .replace("{{package_name}}", package_name)
            response.file.append(pyfile)

        return response


if __name__ == '__main__':
    # Load the request from stdin
    request: CodeGeneratorRequest = CodeGeneratorRequest.FromString(sys.stdin.buffer.read())

    aimrt_code_generator: AimRTCodeGenerator = AimRTCodeGenerator()

    response: CodeGeneratorResponse = aimrt_code_generator.generate(request)

    # Serialize response and write to stdout
    sys.stdout.buffer.write(response.SerializeToString())
