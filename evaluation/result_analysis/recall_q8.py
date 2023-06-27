import pandas as pd
import numpy as np
import argparse

def get_recall(result_file):
    with open(result_file, 'r', encoding="utf8") as f:
        latency = []
        content = f.readline()
        while content:
            if content.find("row") != -1:
                tail = content.find("row")
                rows = content[1:tail]
                print("Recall: ", round(float(rows)/362093,4))
                break
            content = f.readline()
       # latency = latency[1:]
        #print("count",len(latency),"queries")
        return latency


if  __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--path-query', type=str, default="./groundtruth/result_gt/query_1_gt.out", help='path to result')
    
    args = parser.parse_args()
    recall = get_recall(args.path_query)
