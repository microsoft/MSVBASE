import csv
import argparse
import os

def save_recipe_data(path_image, path_text, path_text_data, path_number_data, path_result):
    with open(path_image, 'r', encoding="utf8") as f_image, \
         open(path_text, 'r', encoding="utf8") as f_text, \
         open(path_text_data, 'r', encoding="utf8") as f_textData, \
         open(path_number_data, 'r', encoding="utf8") as f_numberData, \
         open(path_result,'w',encoding="utf8") as out:
        csv.field_size_limit(500*1024*1024)
        tsv_image = csv.reader(f_image, delimiter="\t")
        tsv_text = csv.reader(f_text, delimiter="\t")
        tsv_textData= csv.reader(f_textData, delimiter="\t")
        tsv_numberData= csv.reader(f_numberData, delimiter="\t")
        idx = 0
        for (id, image_embedding), (_, text_embedding), (_, ingredient, instruction), (_, ingre_count) in zip(tsv_image, tsv_text, tsv_textData, tsv_numberData):
            image_embedding = [float(ele) for ele in image_embedding[1:-1].split(', ')]
            image_embedding='{'+str(image_embedding)[1:-1]+'}'
            text_embedding = [float(ele) for ele in text_embedding[1:-1].split(', ')]
            text_embedding='{'+str(text_embedding)[1:-1]+'}'
            text = ingredient.replace('_', ' ') + " " + instruction.replace('_', ' ')
            text = text.replace("\'","")
            text = text.replace("%","")
            out.write(f"{id}\t{image_embedding}\t{text_embedding}\t{text}\t{str(ingre_count)}\n")
            idx += 1
            if idx%10000 == 0:
                print(f"{idx} rows data saved...")

if  __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--path-image-embedding', type=str, default="../raw_data/collections/img_embeds_collection.tsv",
                        help='path to image embedding result')
    parser.add_argument('--path-text-embedding', type=str, default="../raw_data/collections/rec_embeds_collection.tsv",
                        help='path to text embedding result')
    parser.add_argument('--path-text-data', type=str, default="../raw_data/collections/text.tsv",
                        help='path to text data')
    parser.add_argument('--path-number-data', type=str, default="../raw_data/collections/price.tsv",
                        help='path to number data')
    parser.add_argument('--path-result', type=str, default="../raw_data/collections/img_embeds_collection.tsv",
                        help='path to result')
    args = parser.parse_args()
    if os.path.exists(args.path_result):
        print("file exists")
        exit()
    save_recipe_data(args.path_image_embedding, args.path_text_embedding, args.path_text_data, args.path_number_data, args.path_result)
