# MSVBASE

MSVBASE is a system that efficiently supports complex queries of both approximate similarity search and relational operators. It integrates high-dimensional vector indices into PostgreSQL, a relational database to facilitate complex approximate similarity queries.
It is the implementation of the paper [VBASE: Unifying Online Vector Similarity Search and Relational Queries via Relaxed Monotonicity](https://www.usenix.org/system/files/osdi23-zhang-qianxi_1.pdf)
## **Build Docker**
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

## **SQL**
It is compatible with PostgreSQL syntax and protocols, supporting vector distance calculations for L2 and Inner product. It also supports hnsw and sptag indexes. Soon, we will be introducing spann and more indexes. Stay tuned!
### **Command Line**
```
docker exec -it --privileged --user=root vbase_open_source bash
psql -U vectordb
```

### **SQL syntax**
* Currently, 'float array' is used to store vectors.
```
create table t_table(id int, price int, m_vector_1 float8[10], m_vector_2 float8[10]);
```
* When creating a vector index, it is necessary to specify the algorithm and the distance calculation method to be used.
```
create index vector_index_1 on t_table using hnsw(m_vector_1) with(dimension=10,distmethod=l2_distance);
create index vector_index_2 on t_table using sptag(m_vector_2) with(dimension=10,distmethod=inner_product_distance);
```
* When calculating distances, the '<->' operator represents the L2 distance, while '<*>' represents the inner product distance.
```
select id from t_table where price > 15 order by m_vector_1 <-> '{5,9,8,6,2,1,1,0,4,3}' limit 10;
select id from t_table where price > 15 order by m_vector_2 <*> '{5,9,8,6,2,1,1,0,4,3}' limit 5;
```
* It also supports distance threshold-based filtering queries. The query will retrieve vector data that is within the distance threshold.
In the query, the '<<->>' operator represents the L2 distance, while '<<*>>' represents the inner product distance.
The first element of the array represents the distance threshold.
```
select id from t_table where price > 15 and m_vector_1 <<->> '{30,5,9,8,6,2,1,1,0,4,3}';
```
* Multi-vector column query.
```
select id from t_table
order by approximate_sum('0.5 * m_vector_1<->{5,9,8,6,2,1,1,0,4,3} + m_vector_2<*>{5,9,8,6,2,1,1,0,4,3}' ) limit 5;
```

### **Example**
```
create database test;
\c test;
create extension vectordb;
create table t_table(id int, price int, m_vector_1 float8[10]);
insert into t_table values(1, 10, '{1,2,3,4,5,6,7,8,9,0}');
insert into t_table values(2, 20, '{5,6,7,1,2,3,4,8,9,1}');
insert into t_table values(3, 30, '{9,8,7,6,5,4,3,2,1,0}');
create index t4_index on t_table using hnsw(m_vector_1) with(dimension=10,distmethod=l2_distance);
set enable_seqscan=false;
select id from t_table where price > 15 order by m_vector_1 <-> '{5,9,8,6,2,1,1,0,4,3}' limit 1;
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
