#!/usr/bin/python3
import requests

SERVER_HOSTNAME = "0.0.0.0"
SERVER_PORT = "8000"

for _ in range(1):
    resp = requests.get(f"http://{SERVER_HOSTNAME}:{SERVER_PORT}")
    print(f"Got response: {resp.status_code}")
    if resp.status_code != 200:
        raise RuntimeError(f"Request failed with response {resp.status_code}")

print(f"Got a good response from {SERVER_HOSTNAME}:{SERVER_PORT}")

