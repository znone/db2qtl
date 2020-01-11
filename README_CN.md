# db2qtl
db2qtl用于从数据库结构生成QTL代码，目前支持MySQL和SQLite。

## 构建

db2qtl依赖以下库：
- boost >=1.70
- MySQL Connector/C（libmysqlclient）
- SQLite
- QTL
- leech
- nlohmann/json
- YAML-CPP
- libfmt
- inja

发布在github上的库已经做为子模块添加到仓库中，运行以下命令就可以拉取到本地。其余的库需要到官方网站下载。
boost仅依赖 program_options、filesystem、preprocessor这几个子库。
```
git submodule init
git submodule update
```

获取以上库后，就可以开始构建db2qtl。编译db2qtl需要支持C++14的编译器，例如 GCC 5.4或 Visual Studio 2015。Linux下如果所有需要的库都安装在/usr/local下，那么直接运行make就可以完成构建。Windows下通过vs目录下的Visual Studio解决方案来构建。可能需要先在解决方案中设置好所需要的库的路径。


## 运行

首先把数据库配置写入到yaml文件，假如以下内容的配置保存在文件~/test.yaml中：
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
运行以下命令就可以生成操作数据库的QTL代码：
```
./db2qtl ~/test.yaml
```
新生成的QTL代码被保存在当前目录的文件 test_mysql.h 中。选项all_tables要求为数据库中的每个表生成表达表结构的C++结构，并生成对表进行CRUD操作的函数。选项generate_pool指示同时还生成一个访问数据库的连接池。
