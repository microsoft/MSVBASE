# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import csv
import argparse

def query_generate():
    outputfile="/artifacts/vbase/query_run/table4/sql/query_8_vbase.sql"

    with open(outputfile,'w',encoding="utf8") as out:
        #idx = 0
        out.write("\\c test_db;\n\n")
        out.write("set enable_seqscan=off;\n")
        out.write("set enable_indexscan=on;\n")
        out.write("\\timing on\n")
        out.write(f"select recipe_table.id as did, tag_table.id as tid, recipe_table.image_embedding<*>tag_table.image_embedding as distance   from recipe_table join tag_table   on recipe_table.image_embedding <<*>> array_cat(ARRAY[cast(0.01 as float8)], tag_table.image_embedding);\n")
        out.write("\\timing off\n\n")

if  __name__ == "__main__":
    parser = argparse.ArgumentParser()

    args = parser.parse_args()

    query_generate()
