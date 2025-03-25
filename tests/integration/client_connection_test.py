#!/usr/bin/python3
import requests
import concurrent.futures
import multiprocessing

SERVER_HOSTNAME = "0.0.0.0"
SERVER_PORT = "8005"
NUM_REQ_TO_SEND = 100

def send_request() -> bool:
    try:
        resp = requests.get(f"http://{SERVER_HOSTNAME}:{SERVER_PORT}")
        print(f"Got response: {resp}, {resp.headers}")
        if resp.status_code != 200:
            # raise RuntimeError(f"Request failed with response {resp.status_code}")
            print("ERROR: Got a bad response!")
            return False

    except Exception as e:
        print(f"Encountered {e} when issuing request")
        return False
    return True

def print_done(future) -> None:
    return
    # print(f"{future}: I'm done!")


with concurrent.futures.ThreadPoolExecutor(max_workers=multiprocessing.cpu_count()) as executor:
    for _ in range(NUM_REQ_TO_SEND):
        future = executor.submit(send_request)
        future.add_done_callback(print_done)

