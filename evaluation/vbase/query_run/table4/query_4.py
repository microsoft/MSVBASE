import csv
import argparse

def query_generate(path_img, path_rec):
    outputfile="/artifacts/vbase/query_run/table4/sql/query_4_vbase.sql"
    
    with open(path_img, 'r', encoding="utf8") as f_img, \
         open(path_rec, 'r', encoding="utf8") as f_rec, \
         open(outputfile,'w',encoding="utf8") as out:
        tsv_img= csv.reader(f_img, delimiter="\t")
        tsv_rec= csv.reader(f_rec, delimiter="\t")
        idx = 0
        out.write("\\c test_db;\n\n")
        out.write("set enable_seqscan=off;\n")
        out.write("set enable_indexscan=on;\n")
        
        for (id, image_embedding),(_, text_embedding) in zip(tsv_img,tsv_rec):
            image_embedding=image_embedding[1:-1]
            text_embedding=text_embedding[1:-1]
            
            out.write("\\timing on\n")
            out.write(f"select id from topk('recipe_table',50, 64, 100,'id','','','text_embedding <*> ARRAY[{text_embedding}]','image_embedding <*> ARRAY[{image_embedding}]') as (id int, distance float);\n")
            out.write("\\timing off\n\n")
            
            idx += 1
            if idx%1000 == 0:
                print(f"{idx} document embeddings saved...")
            if idx==10000:
                break

if  __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--path-img', type=str, default="/artifacts/raw_data/queries/img_embeds.tsv", help='path to image embedding query')
    parser.add_argument('--path-rec', type=str, default="/artifacts/raw_data/queries/rec_embeds.tsv", help='path to text embedding query')

    args = parser.parse_args()

    query_generate(args.path_img,args.path_rec)
