# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import csv
import argparse
import numpy as np

def get_id(path_id):
    with open(path_id, 'r', encoding="utf8") as f_data:
        ids=[]
        tsv_data= csv.reader(f_data, delimiter="\t")
        for (id, _) in tsv_data:
            ids.append(id)
        return ids

def parse_result(path_result,ids):
    with open(path_result, 'r', encoding="utf8") as f:
        n=0
        content=f.readline()
        res_map={}
        while content:
            if content.find("Timing is on.")!=-1:
                n+=1
                next_line=f.readline()
                next_line=f.readline()
                
                res_score=[]
                while next_line.find("row")==-1:
                    next_line=f.readline()
                    if next_line.find("row")!=-1:
                        res_map[ids[n-1]]=res_score
                        break
                    else:
                        if next_line.find('|')!=-1:
                            s_id=int(next_line.split('|')[0])
                        else:
                            s_id=int(next_line)
                        res_score.append(str(s_id))
            content=f.readline()
        return res_map

def parse_gt(path_gt):
    # for exact file
    with open(path_gt, 'r', encoding="utf8") as f_data:
        tsv_data= csv.reader(f_data, delimiter="\t")
        res_map={}
        for (qid, id, rank, score) in tsv_data:
            if rank=='1':
                res_map[qid]=[]
            res_map[qid].append(id)
        return res_map

def recall(gt_map,query_map,id):
    if len(gt_map[id])==0:
        return 1
    queied=0
    for x in gt_map[id]:
        if x in query_map[id]:
            queied+=1
    return queied/len(gt_map[id])


if  __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--path-id', type=str, default="/artifacts/raw_data/queries/img_embeds.tsv",
                        help='path to get query id')
    parser.add_argument('--path-gt', type=str, default="./result/query_1_gt.out",
                        help='path to gt')
    parser.add_argument('--path-query', type=str, default="./result/query_1_pase.out",
                        help='path to query result')

    args = parser.parse_args()

    ids=get_id(args.path_id)
    query_map=parse_result(args.path_query,ids)
    #print("query len",len(query_map.keys()))
    #print(query_map['12'])
    gt_map=parse_result(args.path_gt,ids)
    #print(gt_map['12'])

    effective=0
    sum_recall=0.0
    for id in ids:
        #if effective>=5000:
        #    break
        effective+=1
        sum_recall+=recall(gt_map,query_map,id)
        c = recall(gt_map,query_map,id)
        #print(effective)
        #if c < 0.9:
        #    print(id)
        #    print(c)
    avg_recall=sum_recall/(effective*1.0)
    #print(f"{effective} counted.")
    print("Recall: ", round(avg_recall,4))
    
