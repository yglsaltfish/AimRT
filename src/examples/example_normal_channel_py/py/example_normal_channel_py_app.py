import argparse
import aimrt_py
import normal_publisher_py_module
import normal_subscriber_py_module


def main():
    parser = argparse.ArgumentParser(description='Example normal channel app.')

    parser.add_argument('--cfg_file_path', type=str, default="", help='config file path')
    parser.add_argument('--dump_cfg_file', type=bool, default=False, help='dump config file')
    parser.add_argument('--dump_cfg_file_path', type=str, default="", help='dump config file path')

    args = parser.parse_args()

    print("AimRT start with cfg file: ", args.cfg_file_path)

    core_options = aimrt_py.CoreOptions()
    core_options.cfg_file_path = args.cfg_file_path
    core_options.dump_cfg_file = args.dump_cfg_file
    core_options.dump_cfg_file_path = args.dump_cfg_file_path
    core_options.register_signal = True
    core_options.auto_set_to_global = True

    try:
        core = aimrt_py.Core()

        publisher_module = normal_publisher_py_module.NormalPublisherPyModule()
        core.RegisterModule(publisher_module)

        subscriber_module = normal_subscriber_py_module.NormalSubscriberPyModule()
        core.RegisterModule(subscriber_module)

        core.Initialize(core_options)
        core.Start()
        core.Shutdown()
    except RuntimeError as e:
        print("AimRT run with exception and exit. {e}")

    print("AimRT exit.")


if __name__ == '__main__':
    main()
