# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import csv
import argparse
import os
import numpy as np
np.set_printoptions(suppress=True)

def transform(x):
    '''
    x: np.array, shape=(6980, 768)
    output: np.array, shape=(6980, 769)
    '''
    extracol = np.zeros(x.shape[0]).astype(np.float32)
    return np.hstack((extracol.reshape(-1, 1), x)).astype(np.float32)

def save_tag_data(path_image, path_result):
    with open(path_image, 'r', encoding="utf8") as f_image, \
         open(path_result,'w',encoding="utf8") as out:
        csv.field_size_limit(500*1024*1024)
        tsv_image = csv.reader(f_image, delimiter="\t")
        idx = 0
        result=[]
        for (id, image_embedding) in tsv_image:
            image_embedding = [float(ele) for ele in image_embedding[1:-1].split(', ')]
            result.append(image_embedding)
            idx += 1
            if idx%10000 == 0:
                print(f"read {idx} rows data...")
        print(len(result))
        np.set_printoptions(suppress=True)
        np.set_printoptions(precision=8)
        input_array = np.array(result)
        output_array = transform(input_array)
        np.savetxt(path_result, output_array, delimiter=',',fmt='%.08f')


if  __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--path-image-embedding', type=str, default="../raw_data/queries/img_embeds.tsv",
                        help='path to image embedding result')
    parser.add_argument('--path-result', type=str, default="../raw_data/collections/img_embeds_collection.tsv",
                        help='path t result')
    args = parser.parse_args()

    if os.path.exists(args.path_result):
        print("file exists")
        exit()

    save_tag_data(args.path_image_embedding, args.path_result)
