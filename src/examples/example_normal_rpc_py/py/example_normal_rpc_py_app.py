import argparse
import aimrt_py
import normal_rpc_client_py_module
import normal_rpc_server_pymodule


def main():
    parser = argparse.ArgumentParser(description='Example normal rpc app.')

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

        rpc_client_module = normal_rpc_client_py_module.NormalRpcClientPyModule()
        core.RegisterModule(rpc_client_module)

        rpc_server_module = normal_rpc_server_pymodule.NormalRpcServerPymodule()
        core.RegisterModule(rpc_server_module)

        core.Initialize(core_options)
        core.Start()
        core.Shutdown()
    except Exception as e:
        print("AimRT run with exception and exit. {}".format(e))

    print("AimRT exit.")


if __name__ == '__main__':
    main()
