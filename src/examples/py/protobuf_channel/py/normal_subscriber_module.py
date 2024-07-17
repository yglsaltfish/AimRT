import aimrt_py
import yaml

from google.protobuf.json_format import MessageToJson
import event_pb2


class NormalSubscriberModule(aimrt_py.ModuleBase):
    def __init__(self):
        super().__init__()
        self.core = aimrt_py.CoreRef()
        self.logger = aimrt_py.LoggerRef()

        self.topic_name = "test_topic"
        self.subscriber = aimrt_py.SubscriberRef()

    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "NormalSubscriberModule"
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
                        self.topic_name = str(data["topic_name"])

            # channel-subscriber
            self.subscriber = self.core.GetChannelHandle().GetSubscriber(self.topic_name)
            if (not self.subscriber):
                aimrt_py.error(self.logger, "Get subscriber for '{}' failed.".format(self.topic_name))
                return False

            def EventHandle(msg):
                aimrt_py.info(self.logger, "Receive new pb event, data: {}".format(MessageToJson(msg)))

            aimrt_py.Subscribe(self.subscriber, event_pb2.ExampleEventMsg, EventHandle)

        except Exception as e:
            aimrt_py.error(self.logger, "Initialize failed. {}".format(e))
            return False

        return True

    def Start(self):
        aimrt_py.info(self.logger, "Module start")

        return True

    def Shutdown(self):
        aimrt_py.info(self.logger, "Module shutdown")
