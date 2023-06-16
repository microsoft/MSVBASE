# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import csv
import argparse

def query_generate(path_img, path_or):
    outputfile="/artifacts/vbase/query_run/table5/sql/query_selectivity_0.9_vbase.sql"
    
    with open(path_img, 'r', encoding="utf8") as f_img, \
         open(path_or, 'r', encoding="utf8") as f_or, \
         open(outputfile,'w',encoding="utf8") as out:
        tsv_img = csv.reader(f_img, delimiter="\t")
        tsv_or = csv.reader(f_or, delimiter="\t")
        idx = 0
        out.write("\\c test_db;\n\n")
        out.write("drop index if exists bindex;\n")
        out.write("set enable_seqscan=off;\n")
        out.write("set enable_indexscan=on;\n")
        for (id, image_embedding), (_, popularity_count) in zip(tsv_img, tsv_or):
            image_embedding=image_embedding[1:-1]
            
            out.write("\\timing on\n")
            out.write(f"select id from recipe_table where (popularity<={9000}) order by image_embedding<*>ARRAY[{image_embedding}] limit 50;\n")
            out.write("\\timing off\n")
            idx += 1
            if idx%1000 == 0:
                print(f"{idx} document embeddings saved...")
            if idx==10000:
                break

if  __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--path-img', type=str, default="/artifacts/raw_data/queries/img_embeds.tsv", help='path to image embedding query')
    parser.add_argument('--path-or', type=str, default="/artifacts/raw_data/queries/price.tsv", help='path to query or filter')

    args = parser.parse_args()

    query_generate(args.path_img, args.path_or)
