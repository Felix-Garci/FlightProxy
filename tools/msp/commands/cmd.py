from commands.serialize import FlightProxyParser


class cmd:
    def __init__(self, id, transactor):
        self.id_: int = id
        self.cmd_on = False
        self.serialize_: FlightProxyParser
        self.transactor_ = transactor

    def setup(self):
        print(f"setingUP desde comando{self.id_}")
        self.serialize_ = FlightProxyParser("str11", "response")

        response_payload = self.transactor_(100 + self.id_, [])
        response_payload = bytes(response_payload)

        signature = self.serialize_.deserialize(response_payload)

        response_payload = self.transactor_(200 + self.id_, [])
        response_payload = bytes(response_payload)

        names = self.serialize_.deserialize(response_payload)

        self.serialize_ = FlightProxyParser(signature["response"], names["response"])

        self.cmd_on = True

    def get_signature(self):
        return self.serialize_.raw_sig

    def get_names(self):
        return self.serialize_.names

    def can_get(self):
        return self.serialize_.can_consume

    def get(self):
        if self.cmd_on and self.can_get():
            response_payload = self.transactor_(300 + self.id_, [])
            if response_payload is not None:
                response_payload = bytes(response_payload)
                data = self.serialize_.deserialize(response_payload)
                return data

        return None

    def can_set(self):
        return self.serialize_.can_produce

    def set(self, data):
        if self.cmd_on and self.can_set():
            data = self._process_data(data)
            serialized = self.serialize_.serialize(data)
            return self.transactor_(400 + self.id_, serialized)
        else:
            return None

    def _process_data(self, data):
        if isinstance(data, dict):
            return data
        if not isinstance(data, list):
            data = [data]
        return dict(zip(self.get_names(), data))
