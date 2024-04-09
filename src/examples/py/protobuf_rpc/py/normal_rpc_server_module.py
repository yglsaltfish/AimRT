import aimrt_py
import aimrt_py_log

import rpc_pb2
import rpc_aimrt_rpc_pb2

from google.protobuf.json_format import MessageToJson


class ExampleServiceImpl(rpc_aimrt_rpc_pb2.ExampleService):
    def __init__(self, logger):
        super().__init__()
        self.logger = logger

    def GetFooData(self, ctx_ref, req):
        rsp = rpc_pb2.GetFooDataRsp()
        rsp.msg = "echo " + req.msg

        aimrt_py_log.info(self.logger,
                          "Server handle new rpc call. req: {}, return rsp: {}"
                          .format(MessageToJson(req), MessageToJson(rsp)))

        return aimrt_py.RpcStatus(), rsp

    def GetBarData(self, ctx_ref, req):
        rsp = rpc_pb2.GetBarDataRsp()
        rsp.msg = "echo " + req.msg

        aimrt_py_log.info(self.logger,
                          "Server handle new rpc call. req: {}, return rsp: {}"
                          .format(MessageToJson(req), MessageToJson(rsp)))

        return aimrt_py.RpcStatus(), rsp


class NormalRpcServerModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.core = aimrt_py.CoreRef()
        self.logger = aimrt_py.LoggerRef()

    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "NormalRpcServerModule"
        return info

    def Initialize(self, core):
        self.core = core
        self.logger = self.core.GetLogger()

        # log
        aimrt_py_log.info(self.logger, "Module initialize")

        try:
            # rpc-server
            self.service = ExampleServiceImpl(self.logger)
            ret = self.core.GetRpcHandle().RegisterService(self.service)
            if (not ret):
                aimrt_py_log.error(self.logger, "Register service failed.")
                return False

        except Exception as e:
            aimrt_py_log.error(self.logger, "Initialize failed. {}".format(e))
            return False

        return True

    def Start(self):
        aimrt_py_log.info(self.logger, "Module start")

        return True

    def Shutdown(self):
        aimrt_py_log.info(self.logger, "Module shutdown")
