import csv
import argparse

def query_generate():
    outputfile="/artifacts/spann/query_run/table8/sql/query_8_spann.sql"

    with open(outputfile,'w',encoding="utf8") as out:
        #idx = 0
        out.write("\\c spann_db;\n\n")
        out.write("set enable_seqscan=off;\n")
        out.write("set enable_indexscan=on;\n")
        out.write("\\timing on\n")
        out.write(f"select recipe_table.id as did, tag_table.id as tid from recipe_table join tag_table on recipe_table.image_embedding <<->> array_cat(ARRAY[cast(0.01 as float8)], tag_table.image_embedding);\n")
        out.write("\\timing off\n\n")

if  __name__ == "__main__":
    parser = argparse.ArgumentParser()

    args = parser.parse_args()

    query_generate()
