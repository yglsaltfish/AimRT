import aimrt_py


class HelloWorldPyModule(aimrt_py.ModuleBase):
    def Info(self):
        info = aimrt_py.ModuleInfo()
        info.name = "HelloWorldPyModule"
        return info

    def Initialize(self, core):
        print("Module initialize")
        return True

    def Start(self):
        print("Module start")
        return True

    def Shutdown(self):
        print("Module shutdown")
