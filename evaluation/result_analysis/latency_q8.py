import pandas as pd
import numpy as np
import argparse

def get_latency(result_file):
    with open(result_file, 'r', encoding="utf8") as f:
        latency = []
        content = f.readline()
        while content:
            if content.find("Time") != -1:
                tail = content.find("ms")
                time = content[6:tail]
                print("Latency average / median / 99th (ms): {}, {}, {}".format(round(float(time), 1), round(float(time), 1), round(float(time), 1)))
                break
            content = f.readline()
        return latency


if  __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--path-result', type=str, default="./groundtruth/result_gt/query_1_gt.out", help='path to result')
    
    args = parser.parse_args()
    latency = get_latency(args.path_result)
