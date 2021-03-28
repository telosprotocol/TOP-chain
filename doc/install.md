# 编译安装指南

## 编译环境准备：
> *必须安装gcc 4.8.5版本*
```shell
sudo yum install -y cmake3 readline-devel.x86_64
sudo yum install gcc gcc-g++
$ gcc -v
gcc version 4.8.5 20150623 (Red Hat 4.8.5-39) (GCC) 
```

## 编译程序

- 解压文件:
```shell
tar -xzf topiolib.tar.gz $TOP_HOME/src # 指定源代码目录
```

- 编译:
```shell
cd $TOP_HOME/src # TOP_src对应解压后目录

# 编译debug版本
sh build.sh

编译后生成文件所在位置：

$TOP_HOME/cbuild/bin/Linux/topio

编译后生成lib库所在位置：

$TOP_HOME/cbuild/lib/Linux/libxtopchain.so.0.9.7.0

# 编译release版本
sh build.sh release

编译后生成文件所在位置：

$TOP_HOME/cbuild_release/bin/Linux/topio

编译后生成lib库所在位置：

$TOP_HOME/cbuild_release/lib/Linux/libxtopchain.so.0.9.7.0




