create database spann_db;
\c spann_db;

CREATE EXTENSION vectordb;

create table recipe_table(id int PRIMARY KEY, image_embedding float8[1025], text_embedding float8[1025], description text, popularity int);
alter table recipe_table alter image_embedding  set storage plain;
alter table recipe_table alter text_embedding  set storage plain;
alter table recipe_table alter description set storage plain;

copy recipe_table from '/artifacts/data_prepare/data_source_l2.tsv' DELIMITER E'\t' csv quote e'\x01';

create index image_spann_index on recipe_table using spann(image_embedding spann_vector_l2_ops);
create index text_spann_index on recipe_table using spann(text_embedding spann_vector_l2_ops);
create index bindex on recipe_table(popularity);

create table tag_table(id int PRIMARY KEY,image_embedding float8[1025]);
copy tag_table from '/artifacts/data_prepare/tag_datasource_l2.tsv' DELIMITER E'\t' csv quote e'\x01';
