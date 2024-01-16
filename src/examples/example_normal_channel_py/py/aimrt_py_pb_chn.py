import aimrt_py
import google.protobuf


class ProtobufTypeSupport(aimrt_py.TypeSupportBase):
    def __init__(self, protobuf_type):
        super().__init__()
        self.protobuf_type = protobuf_type
        self.type_name = protobuf_type.__name__
        self.serialization_types_supported_vec = ["pb", "json"]

    def Create(self):
        return self.protobuf_type()

    def Destroy(self, msg):
        del msg

    def Copy(self, from_msg, to_msg):
        to_msg.MergeFrom(from_msg)

    def Move(self, from_msg, to_msg):
        to_msg.MergeFrom(from_msg)

    def Serialize(self, serialization_type, msg, buffer):
        try:
            if(serialization_type == "pb"):
                buffer = msg.SerializeToString()
                return True

            if(serialization_type == "json"):
                buffer = google.protobuf.json_format.MessageToJson(msg)
                return True
        except Exception as e:
            return False

        return False

    def Deserialize(self, serialization_type, buffer, msg):
        try:
            if(serialization_type == "pb"):
                msg.ParseFromString(buffer)
                return True

            if(serialization_type == "json"):
                msg = google.protobuf.json_format.ParseFromString(buffer)
                return True
        except Exception as e:
            return False

        return False
