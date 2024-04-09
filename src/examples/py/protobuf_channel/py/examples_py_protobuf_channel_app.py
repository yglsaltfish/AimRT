import argparse
import threading
import signal
import sys
import aimrt_py
import normal_publisher_module
import normal_subscriber_module

global_core = None


def signal_handler(sig, frame):
    global global_core

    if (global_core and (sig == signal.SIGINT or sig == signal.SIGTERM)):
        global_core.Shutdown()
        return

    sys.exit(0)


def run_aimrt_core(args):
    try:
        core = aimrt_py.Core()

        global global_core
        global_core = core

        publisher_module = normal_publisher_module.NormalPublisherModule()
        core.RegisterModule(publisher_module)

        subscriber_module = normal_subscriber_module.NormalSubscriberModule()
        core.RegisterModule(subscriber_module)

        core_options = aimrt_py.CoreOptions()
        core_options.cfg_file_path = args.cfg_file_path
        core_options.dump_cfg_file = args.dump_cfg_file
        core_options.dump_cfg_file_path = args.dump_cfg_file_path
        core.Initialize(core_options)

        core.Start()

        core.Shutdown()

        global_core = None
    except Exception as e:
        print("AimRT run with exception and exit. {}".format(e))
        sys.exit(-1)


def main():
    parser = argparse.ArgumentParser(description='Example normal channel app.')

    parser.add_argument('--cfg_file_path', type=str, default="", help='config file path')
    parser.add_argument('--dump_cfg_file', type=bool, default=False, help='dump config file')
    parser.add_argument('--dump_cfg_file_path', type=str, default="", help='dump config file path')

    args = parser.parse_args()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    print("AimRT start.")

    thread = threading.Thread(target=run_aimrt_core, kwargs={'args': args})
    thread.start()
    thread.join()

    print("AimRT exit.")


if __name__ == '__main__':
    main()
