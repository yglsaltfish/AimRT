# Copyright (c) 2023, AgiBot Inc.
# All rights reserved.

import argparse
import threading
import signal
import sys
import aimrt_py
import yaml

from google.protobuf.json_format import MessageToJson
import event_pb2

global_core = None


def signal_handler(sig, frame):
    global global_core

    if(global_core and (sig == signal.SIGINT or sig == signal.SIGTERM)):
        global_core.Shutdown()
        return

    sys.exit(0)


def main():
    parser = argparse.ArgumentParser(description='Example channel subscriber app.')
    parser.add_argument('--cfg_file_path', type=str, default="", help='config file path')
    args = parser.parse_args()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    print("AimRT start.")

    core = aimrt_py.Core()

    global global_core
    global_core = core

    # Initialize
    core_options = aimrt_py.CoreOptions()
    core_options.cfg_file_path = args.cfg_file_path
    core.Initialize(core_options)

    # Create Module
    module_handle = core.CreateModule("NormalSubscriberPyModule")

    # Read cfg
    module_cfg_file_path = module_handle.GetConfigurator().GetConfigFilePath()
    with open(module_cfg_file_path, 'r') as file:
        data = yaml.safe_load(file)
        topic_name = str(data["topic_name"])

    # Subscribe
    subscriber = module_handle.GetChannelHandle().GetSubscriber(topic_name)
    assert subscriber, "Get subscriber for topic '{}' failed.".format(topic_name)

    def EventHandle(msg):
        aimrt_py.info(module_handle.GetLogger(), "Get new pb event, data: {}".format(MessageToJson(msg)))

    aimrt_py.Subscribe(subscriber, event_pb2.ExampleEventMsg, EventHandle)

    # Start
    thread = threading.Thread(target=core.Start)
    thread.start()

    while thread.is_alive():
        thread.join(1.0)

    print("AimRT exit.")


if __name__ == '__main__':
    main()
