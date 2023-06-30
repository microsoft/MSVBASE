#!/bin/bash
python3 /artifacts/data_prepare/spann_embedding_data.py --path-image-embedding /artifacts/raw_data/collections/img_embeds_collection.tsv --path-result /artifacts/data_prepare/img_embeds_1025.tsv
python3 /artifacts/data_prepare/convert_bin.py --src /artifacts/data_prepare/img_embeds_1025.tsv --dst /artifacts/data_prepare/img_embeds_1025.bin
python3 /artifacts/data_prepare/data_gen_spann.py --path-image-embedding /artifacts/data_prepare/img_embeds_1025.tsv --path-text-embedding /artifacts/data_prepare/img_embeds_1025.tsv --path-text-data /artifacts/raw_data/collections/text.tsv --path-number-data /artifacts/raw_data/collections/price.tsv  --path-result /artifacts/data_prepare/data_source_l2.tsv
