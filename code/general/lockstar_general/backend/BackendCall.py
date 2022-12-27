import json
import logging

class BackendCall:
    """Call sent to backend by client. (Should be implemented in C++)"""
    
    DELIMITTER =  b'\xBC'

    @classmethod
    def from_bytes(cls, bytes_dict):
        try:
            call_dict = json.loads(bytes_dict.decode('utf-8'))
            
            return cls(call_dict['client_id'], call_dict['module_name'], call_dict['method_name'], call_dict['args'])
        except Exception as ex:
            logging.error(f'BackendCall.from_bytes: Could not parse bytes dict: {ex}')
            raise ex

    def __init__(self, client_id, module_name, method_name, args) -> None:
        """Contains the data corresponding to a Backend method call

        Args:
            client_id (int): client_id (used to identify different clients)
            module_name (str): Class Name of the Module which should be called
            method_name (str): name of the method of the module which should be called
            args (dict): dictionary containing the the named arguments for the method to call
        """
        self.client_id = client_id
        self.module_name = module_name
        self.method_name = method_name
        self.args = args

    def to_bytes(self):
        call_dict = {
            'client_id': self.client_id, 
            'module_name': self.module_name,
            'method_name': self.method_name,
            'args': self.args
            }
        try:
            return json.dumps(call_dict).encode('utf-8') + BackendCall.DELIMITTER
        except Exception as ex:
            logging.error(f'BackendCall.to_bytes: Could not parse dict to json bytes: {ex}')
            raise ex