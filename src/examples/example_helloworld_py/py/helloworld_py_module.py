import aimrt_py
import aimrt_py_logger
import yaml
import datetime


class HelloWorldPyModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.core = aimrt_py.CoreRef()
        self.logger = aimrt_py.LoggerRef()
        self.work_executor = aimrt_py.ExecutorRef()

    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "HelloWorldPyModule"
        return info

    def Initialize(self, core):
        self.core = core
        self.logger = self.core.GetLogger()

        # log
        aimrt_py_logger.info(self.logger, "Module initialize")

        # configure
        configurator = self.core.GetConfigurator()
        if(configurator):
            module_cfg_file_path = configurator.GetConfigFilePath()
            with open(module_cfg_file_path, 'r') as file:
                data = yaml.safe_load(file)
                aimrt_py_logger.info(self.logger, str(data))

        # executor
        self.work_executor = self.core.GetExecutorManager().GetExecutor("work_thread_pool")
        if(not self.work_executor):
            aimrt_py_logger.error(self.logger, "Get executor 'work_thread_pool' failed.")
            return False

        return True

    def Start(self):
        aimrt_py_logger.info(self.logger, "Module start")

        def test_task():
            aimrt_py_logger.info(self.logger, "run test task.")

        self.work_executor.Execute(test_task)
        self.work_executor.ExecuteAfter(datetime.timedelta(seconds=1), test_task)
        self.work_executor.ExecuteAt(datetime.datetime.now() + datetime.timedelta(seconds=2), test_task)

        return True

    def Shutdown(self):
        aimrt_py_logger.info(self.logger, "Module shutdown")
