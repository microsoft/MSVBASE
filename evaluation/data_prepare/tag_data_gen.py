import csv
import argparse
import os

def save_tag_data(path_image, path_result):
    with open(path_image, 'r', encoding="utf8") as f_image, \
         open(path_result,'w',encoding="utf8") as out:
        csv.field_size_limit(500*1024*1024)
        tsv_image = csv.reader(f_image, delimiter="\t")
        idx = 0
        for (id, image_embedding) in tsv_image:
            image_embedding = [float(ele) for ele in image_embedding[1:-1].split(', ')]
            image_embedding='{'+str(image_embedding)[1:-1]+'}'
            out.write(f"{id}\t{image_embedding}\n")
            idx += 1
            if idx%1000 == 0:
                print(f"{idx} rows data saved...")

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
