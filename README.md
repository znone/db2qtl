# db2qtl
db2qtl is used to generate QTL code from the database structure, currently supports MySQL and SQLite.

## 构建

db2qtl depends on the following libraries:
- boost >=1.70
- MySQL Connector/C（libmysqlclient）
- SQLite
- QTL
- leech
- nlohmann/json
- YAML-CPP
- libfmt
- inja

The library published on github has been added to the repository as a submodule, and you can pull it locally by running the following command. The remaining libraries need to be downloaded from the official website.
Boost only relies on program_options, filesystem, and preprocessor.
```
git submodule init
git submodule update
```

After obtaining the above libraries, you can start building db2qtl. Compiling db2qtl requires a compiler that supports C ++ 14, such as GCC 5.4 or Visual Studio 2015. If all required libraries are installed under /usr/local under Linux, then you can run make directly to complete the build. Under Windows, use the Visual Studio solution under the vs directory to build. You may need to set the path to the required libraries in your solution first.


## 运行

First write the database configuration to the YAML file. If the configuration of the following is saved in the file ~/test.yaml:
```YAML
connection : 
    type : mysql
    host : localhost
    port : 3306
    user : root
    password : "123456"
    database : test
filename : test_mysql
namespace: test
generate_pool: true
all_tables: true
```
Run the following command to generate the QTL code for operating the database:
```
./db2qtl ~/test.yaml
```
The newly generated QTL code is saved in the file test_mysql.h in the current directory. The option **all_tables** requires for each table in the database to generate a C ++ structure that expresses the structure of the table and a function that performs CRUD operations on the table. The option **generate_pool** indicates that a connection pool to the database is also generated.
