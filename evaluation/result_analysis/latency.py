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
                latency.append(float(time))
            content = f.readline()
       # latency = latency[1:]
        #print("count",len(latency),"queries")
        #print("avg latency: ",round(sum(latency)/(len(latency)*1.0),1))
        #print("min:",min(latency))
        #print("max:",max(latency))
        latency.sort(reverse=False)
        #print(latency[0],latency[-1])
        #print(latency)
        median = np.median(latency)
       # print("median latency: ", round(median,1))
        num = min(round(len(latency)*0.99), len(latency)-1)
       # print("99%th latency: ",round(latency[num],1))
       # print()
        print("Latency average / median / 99th (ms): {}, {}, {}".format(round(sum(latency)/(len(latency)*1.0),1),round(median,1),round(latency[num],1)))
        return latency


if  __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--path-result', type=str, default="./groundtruth/result_gt/query_1_gt.out", help='path to result')
    
    args = parser.parse_args()
    latency = get_latency(args.path_result)
