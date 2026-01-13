class SignalBus:
    def __init__(self):
        self._providers = {}

    def register_provider(self, service_name, callback):
        self._providers[service_name] = callback

    def request(self, service_name, *args, **kwargs):
        if service_name in self._providers:
            return self._providers[service_name](*args, **kwargs)
        print(f"Error: Nadie ofrece el servicio '{service_name}'")
        return None
