from mimetypes import init
from lockstar_general.communication.BackendDP import BackendDP
from enum import Enum
from struct import pack, unpack, calcsize
from lockstar_general.communication.BackendModuleTypes import BackendModuleTypes


class SinglePIDMethods(Enum):
    INIT = 0
    SET_PID = 1
    

class SinglePIDBackendDP(BackendDP):

    @classmethod
    def for_init(cls, output_min, output_max, channel_error, channel_setpoint, channel_out, triggered):
        module_identifier_b = BackendModuleTypes.SINGLEPID.value.to_bytes(2, 'big')
        method_identifier_b = SinglePIDMethods.INIT.value.to_bytes(2, 'big')
        payload_b = pack('>ddbbb?', output_min, output_max, channel_error, channel_setpoint, channel_out, triggered)
        payload_length = calcsize('>ddbbb?')
        payload_length_b = payload_length.to_bytes(4, 'big')

        return cls(module_identifier_b, method_identifier_b, payload_length_b, payload_b)

    def _initialize_for_init(self):
        (output_min, output_max, channel_error, channel_setpoint, channel_out, triggered) = unpack('>ddbbb?', self.payload_b)

        self.method_identifier = SinglePIDMethods.INIT

        self.output_min = output_min
        self.output_max = output_max
        self.channel_error = channel_error
        self.channel_setpoint = channel_setpoint
        self.channel_out = channel_out
        self.triggered = triggered


    def __init__(self, module_identifier_b, method_identifier_b, payload_length_b, payload_b) -> None:
        super().__init__(module_identifier_b, method_identifier_b, payload_length_b, payload_b)
        self.output_min = None
        self.output_max = None
        self.channel_error = None
        self.channel_setpoint = None
        self.channel_out = None
        self.triggered = None

        self.module_identifier = BackendModuleTypes.SINGLEPID

        # === parse payload
        self.method_identifier = int.from_bytes(method_identifier_b, 'big')

        if self.method_identifier == SinglePIDMethods.INIT.value:
            self._initialize_for_init()
        elif self.method_identifier == SinglePIDMethods.SET_PID.value:
            pass
