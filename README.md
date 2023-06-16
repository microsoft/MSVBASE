# MSVBASE
## What's NEW!
New Research Paper [VBASE: Unifying Online Vector Similarity Search and Relational Queries via Relaxed Monotonicity](https://www.usenix.org/system/files/osdi23-zhang-qianxi_1.pdf) - _published in OSDI 2023_
## What's MSVBASE!
MSVBASE is a new system capable of efficiently serving complex online queries that involve both approximate similarity search and relational operators on scalar and vector data-sets. MSVBASE builds a unified query execution engine to support a wide range of queries both on scalar and vector data, and shows superior query performance and accuracy.
It integrates high-dimensional vector indices into PostgreSQL with minimal code modifications.

## **Quickstart**
### **Clone and Patch**
```
git clone https://github.com/microsoft/MSVBASE.git
cd MSVBASE
git submodule update --init --recursive
./scripts/patch.sh
```
### **Build**
```
./scripts/dockerbuild.sh
```

### **Run**
```
./scripts/dockerrun.sh
```

## **Features and SQL syntax**
It is compatible with PostgreSQL syntax and protocol, supporting vector distance calculations for L2 and Inner product. It also supports [hnsw](https://github.com/nmslib/hnswlib) and [sptag](https://github.com/microsoft/SPTAG/) indices. Soon, we are going to integrate spann and more indices.
### **Command Line**
```
docker exec -it --privileged --user=root vbase_open_source bash
psql -U vectordb
```

### **SQL syntax**
It preserves all the features of PostgreSQL while extending query support on vector data.
* Currently, 'float array' is used to store vectors.
```
create table t_table(id int, price int, vector_1 float8[10], vector_2 float8[10]);
```
* Insert or Import data.
```
insert into t_table values(1, 10, '{1,2,3,4,5,6,7,8,9,0}', '{5,6,7,1,2,3,4,8,9,1}');
copy t_table from 'your_data_path.tsv' DELIMITER E'\t' csv quote e'\x01';
```
* When creating a vector index, it is necessary to specify the algorithm and the distance calculation method to be used.
```
create index vector_index_1 on t_table using hnsw(vector_1) with(dimension=10,distmethod=l2_distance);
create index vector_index_2 on t_table using sptag(vector_2) with(dimension=10,distmethod=inner_product_distance);
```
* **TopK**. When calculating distances, the '<->' operator represents the L2 distance, while '<*>' represents the inner product distance.
```
select id from t_table order by vector_1 <-> '{5,9,8,6,2,1,1,0,4,3}' limit 10;
select id from t_table order by vector_2 <*> '{5,9,8,6,2,1,1,0,4,3}' limit 5;
```
* **TopK + Filter**.
```
select id from t_table where price > 15 order by vector_1 <-> '{5,9,8,6,2,1,1,0,4,3}' limit 10;
select id from t_table where price > 15 order by vector_2 <*> '{5,9,8,6,2,1,1,0,4,3}' limit 5;
```
* **Distance Range Filter**. It also supports distance threshold-based filtering queries. The query will retrieve vector data that is within the distance threshold.
In the query, the '<<->>' operator represents the L2 distance, while '<<*>>' represents the inner product distance.
The first element of the array represents the distance threshold.
```
select id from t_table where price > 15 and vector_1 <<->> '{30,5,9,8,6,2,1,1,0,4,3}';
```
* **Multi-vector Column Query**.
```
select id from t_table
order by approximate_sum('0.5 * vector_1<->{5,9,8,6,2,1,1,0,4,3} + vector_2<*>{5,9,8,6,2,1,1,0,4,3}' ) limit 5;
```
* **Join**. Join on vector similarity with threshold
```
select t_table.id as tid, d_table.id as did
from t_table join d_table
on t_table.vector_2 <<*>> array_cat(ARRAY[cast(10 as float8)], d_table.vector_2);
```

* Example
```
create database test;
\c test;
create extension vectordb;
create table t_table(id int, price int, vector_1 float8[10]);
insert into t_table values(1, 10, '{1,2,3,4,5,6,7,8,9,0}');
insert into t_table values(2, 20, '{5,6,7,1,2,3,4,8,9,1}');
insert into t_table values(3, 30, '{9,8,7,6,5,4,3,2,1,0}');
create index t4_index on t_table using hnsw(vector_1) with(dimension=10,distmethod=l2_distance);
set enable_seqscan=false;
select id from t_table where price > 15 order by vector_1 <-> '{5,9,8,6,2,1,1,0,4,3}' limit 1;
insert into t_table values(4, 40, '{19,18,17,16,15,14,13,12,11,10}');
delete from t_table where id = 2;
```

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft 
trademarks or logos is subject to and must follow 
[Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.
