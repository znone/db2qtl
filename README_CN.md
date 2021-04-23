# db2qtl
db2qtl用于从数据库结构生成QTL代码，目前支持MySQL和SQLite。

## 构建

db2qtl依赖以下库，需要单独下载和编译：
- boost >=1.70
boost仅依赖 program_options、filesystem、preprocessor这几个子库。

以下数据库的客户端，需要单独下载、编译或安装：
- MySQL Connector/C（libmysqlclient）
- libpg
- SQLite

以下库已经添加为本仓库的子模块，可以在克隆时自动拉取：
- QTL
- leech
- nlohmann/json
- libfmt
- YAML-CPP
- inja

libfmt和YAML-CPP需要编译和安装。

```
git submodule init
git submodule update
```

准备好以上的库后，就可以开始构建db2qtl。编译db2qtl需要支持C++14的编译器，例如 GCC 5.4或 Visual Studio 2015。

### Visual Studio

把下载的sqlite源码解压缩到third_party\sqlite目录。
用Visual Studio 打开vs\db2qtl.sln，在属性管理器中修改属性db2qtl中定义的宏：
- MYSQL_HOME是MySQL的安装目录
- PG_HOME是PostgreSQL安装目录
- BOOST_HOME是boost库所在的目录

### Linux
设置环境变量BOOST_HOME为boost库所在的目录。
直接运行make就可以完成构建。

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
