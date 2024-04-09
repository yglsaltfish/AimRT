import aimrt_py
import aimrt_py_log
import aimrt_py_pb_chn
import yaml
import datetime
import time

from google.protobuf.json_format import MessageToJson
import event_pb2


class NormalPublisherModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.core = aimrt_py.CoreRef()
        self.logger = aimrt_py.LoggerRef()
        self.work_executor = aimrt_py.ExecutorRef()

        self.topic_name = "test_topic"
        self.channel_frq = 0.5
        self.publisher = aimrt_py.PublisherRef()

        self.loop_count = 0
        self.stop_flag = False
        self.stoped_flag = False

    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "NormalPublisherModule"
        return info

    def Initialize(self, core):
        self.core = core
        self.logger = self.core.GetLogger()

        # log
        aimrt_py_log.info(self.logger, "Module initialize")

        try:
            # configure
            configurator = self.core.GetConfigurator()
            if (configurator):
                module_cfg_file_path = configurator.GetConfigFilePath()
                if (module_cfg_file_path):
                    with open(module_cfg_file_path, 'r') as file:
                        data = yaml.safe_load(file)
                        self.topic_name = str(data["topic_name"])
                        self.channel_frq = float(data["channel_frq"])

            # executor
            self.work_executor = self.core.GetExecutorManager().GetExecutor("work_thread_pool")
            if (not self.work_executor):
                aimrt_py_log.error(self.logger, "Get executor 'work_thread_pool' failed.")
                return False

            # channel-publisher
            self.publisher = self.core.GetChannelHandle().GetPublisher(self.topic_name)
            if (not self.publisher):
                aimrt_py_log.error(self.logger, "Get publisher for '{}' failed.".format(self.topic_name))
                return False

            aimrt_py_pb_chn.RegisterPublishType(self.publisher, event_pb2.ExampleEventMsg)

        except Exception as e:
            aimrt_py_log.error(self.logger, "Initialize failed. {}".format(e))
            return False

        return True

    def Start(self):
        aimrt_py_log.info(self.logger, "Module start")

        try:
            self.work_executor.Execute(self.PublishLoop)

        except Exception as e:
            aimrt_py_log.error(self.logger, "Initialize failed. {}".format(e))
            return False

        return True

    def Shutdown(self):
        self.stop_flag = True

        while (not self.stoped_flag):
            time.sleep(1)

        aimrt_py_log.info(self.logger, "Module shutdown")

    def PublishLoop(self):
        if (self.stop_flag):
            self.stoped_flag = True
            return

        try:
            self.loop_count = self.loop_count + 1
            aimrt_py_log.info(self.logger,
                              "Loop count : {} -------------------------".format(self.loop_count))

            # publish event
            event_msg = event_pb2.ExampleEventMsg()
            event_msg.msg = "count {}".format(self.loop_count)
            event_msg.num = self.loop_count
            aimrt_py_log.info(self.logger,
                              "Publish new pb event, data: {}".format(MessageToJson(event_msg)))

            aimrt_py_pb_chn.Publish(self.publisher, event_msg)

            # next loop
            self.work_executor.ExecuteAfter(
                datetime.timedelta(seconds=1 / self.channel_frq),
                self.PublishLoop)

        except Exception as e:
            aimrt_py_log.error(self.logger, "PublishLoop failed. {}".format(e))
            self.stoped_flag = True
