import aimrt_py
import yaml
import datetime
import time

import rpc_pb2
import rpc_aimrt_rpc_pb2

from google.protobuf.json_format import MessageToJson


class NormalRpcClientModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.core = aimrt_py.CoreRef()
        self.logger = aimrt_py.LoggerRef()
        self.work_executor = aimrt_py.ExecutorRef()

        self.rpc_frq = 0.5
        self.rpc_handle = aimrt_py.RpcHandleRef()
        self.proxy = rpc_aimrt_rpc_pb2.ExampleServiceProxy()

        self.loop_count = 0
        self.stop_flag = False
        self.stoped_flag = False

    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "NormalRpcClientModule"
        return info

    def Initialize(self, core):
        self.core = core
        self.logger = self.core.GetLogger()

        # log
        aimrt_py.info(self.logger, "Module initialize")

        try:
            # configure
            configurator = self.core.GetConfigurator()
            if (configurator):
                module_cfg_file_path = configurator.GetConfigFilePath()
                if (module_cfg_file_path):
                    with open(module_cfg_file_path, 'r') as file:
                        data = yaml.safe_load(file)
                        self.rpc_frq = float(data["rpc_frq"])

            # executor
            self.work_executor = self.core.GetExecutorManager().GetExecutor("work_thread_pool")
            if (not self.work_executor):
                aimrt_py.error(self.logger, "Get executor 'work_thread_pool' failed.")
                return False

            # rpc-client
            self.rpc_handle = self.core.GetRpcHandle()
            ret = rpc_aimrt_rpc_pb2.ExampleServiceProxy.RegisterClientFunc(self.rpc_handle)
            if (not ret):
                aimrt_py.error(self.logger, "Register client failed.")
                return False

            self.proxy = rpc_aimrt_rpc_pb2.ExampleServiceProxy(self.rpc_handle)

        except Exception as e:
            aimrt_py.error(self.logger, "Initialize failed. {}".format(e))
            return False

        return True

    def Start(self):
        aimrt_py.info(self.logger, "Module start")

        try:
            self.work_executor.Execute(self.ClientLoop)

        except Exception as e:
            aimrt_py.error(self.logger, "Initialize failed. {}".format(e))
            return False

        return True

    def Shutdown(self):
        self.stop_flag = True

        while (not self.stoped_flag):
            time.sleep(1)

        aimrt_py.info(self.logger, "Module shutdown")

    def ClientLoop(self):
        if (self.stop_flag):
            self.stoped_flag = True
            return

        try:
            self.loop_count = self.loop_count + 1
            aimrt_py.info(self.logger,
                          "Loop count : {} -------------------------".format(self.loop_count))

            # call rpc
            req = rpc_pb2.GetFooDataReq()
            req.msg = "count {}".format(self.loop_count)

            aimrt_py.info(self.logger,
                          "Client start new rpc call. req: {}".format(MessageToJson(req)))

            ctx = aimrt_py.RpcContext()
            ctx.SetTimeout(datetime.timedelta(seconds=30))
            status, rsp = self.proxy.GetFooData(ctx, req)

            if (status):
                aimrt_py.info(
                    self.logger, "Client get rpc ret, status: {}, rsp: {}".format(
                        status.ToString(), MessageToJson(rsp)))
            else:
                aimrt_py.warn(self.logger,
                              "Client get rpc error ret, status: {}".format(status.ToString()))

            # next loop
            self.work_executor.ExecuteAfter(
                datetime.timedelta(seconds=1 / self.rpc_frq),
                self.ClientLoop)

        except Exception as e:
            aimrt_py.error(self.logger, "ClientLoop failed. {}".format(e))
            self.stoped_flag = True
