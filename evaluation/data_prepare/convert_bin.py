import numpy as np

import argparse

import struct

import pandas as pd




def process_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--src", help="The input file (.tsv)")
    parser.add_argument("--dst", help="The output file prefix (.bin)")
    return parser.parse_args()

if __name__ == "__main__":
    args = process_args()
    print("reading csv")
    train=pd.read_csv(args.src, sep='\t', header=None)
    print("convert csv to bin")
    dim = 1025
    with open(args.dst, "wb") as f:
        print(len(train))
        f.write(struct.pack('i', len(train)))
        f.write(struct.pack('i', dim))
        for i in range (0, len(train)):
            test = train.iloc[i][0]
            test_dim = test.split(',')
            for j in range(0, dim):
                f.write((struct.pack('f', float(test_dim[j]))))
