import json
import logging


class BackendResponse:
    """Response from the backend to client (Should be implemented in C++)"""
    
    DELIMITTER =  b'\xBC'

    @staticmethod
    def ACK():
        return BackendResponse('ACK')

    @staticmethod
    def NACK():
        return BackendResponse('NACK')

    @staticmethod
    def wrong_client_id():
        return BackendResponse({'error': 'wrong_client_id'})

    def is_wrong_client_id(self):
        if self.has_error():
            return self.response['error'] == 'wrong_client_id'


    @classmethod
    def from_bytes(cls, bytes_dict):
        try:
            call_dict = json.loads(bytes_dict.decode('utf-8'))
            return cls(call_dict)
        except Exception as ex:
            logging.error(f'BackendResponse.from_bytes: Could not parse bytes dict: {ex}')
            raise ex

    def __init__(self, response) -> None:
        """Contains the data corresponding to a response from the backend to the client

        Args:
            args (dict): dictionary containing the the named arguments for reply
        """
        self.response = response

    def is_ACK(self) -> bool:
        return self.response == 'ACK'

    def has_error(self):
        if isinstance(self.response, dict) and 'error' in self.response and len(self.response['error']) > 0:
            return True
        else:
            return False

    def to_bytes(self):
        try:
            return json.dumps(self.response).encode('utf-8') + BackendResponse.DELIMITTER
        except Exception as ex:
            logging.error(f'BackendResponse.to_bytes: Could not parse dict to json bytes: {ex}')
            raise ex