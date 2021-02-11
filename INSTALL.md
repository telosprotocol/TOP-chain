# <center>INSTALL TOPIO</center>

INSTALL - Installation of TOPIO on different machines.

This file contains instructions for compiling TOPIO. If you already have an
executable version of TOPIO, you don't need this.

# DevDependencies

Befor installation, maybe you should get gcc and cmake ready.

### gcc/g++ 4.8.5
 (exactly 4.8.5, not support higher or lower gcc version)

### cmake3
(version 3 or higher)

```
sudo yum install  cmake3.x86_64
```

### libreadline.so

```
sudo yum install readline-devel.x86_64
```

# Compile and Install
### use shell-script
Here are some brief instructions on how to install TOPIO.  First you need to
compile.  Since you're impatient, try this

```
sh build.sh
```
more options show in **build_options.sh**, you can choose the compile options by yourself.

```
cat build_options.sh
```

when compile done, then use this to install:

```
sudo sh build.sh install
```

attention: the install location of topio is **`/usr/bin/topio`**.

if you want to build release version, use this:

```
sh build.sh release
```

then install release version topio, use this:

```
sudo sh build.sh release install
```

that's all for compile and install for topio using shell-script!


futhermore, maybe you need do this befor run topio:

```
sudo ldconfig
```

then run topio:

```
topio -v
```


### use cmake tool
Or, if you preffer to use **cmake** manually:

```
mkdir cbuild
cd cbuild
cmake3 .. -DXENABLE_TESTS=OFF -DXENABLE_CODE_COVERAGE=OFF 
make -j4
sudo make install
```

attention: the install location of topio is **`/usr/bin/topio`**.

if you want to build release version:

```
-DCMAKE_BUILD_TYPE=Release
```

if you want to use metrics which collect some important run-time infomation, such as throughput capacity：

```
-DBUILD_METRICS=ON
```

if you want to set noratelimit on topio:

```
-DXDISABLE_RATELIMIT=ON
```

TODO, other cmake options.

TODO, other cmake options.

TODO, other cmake options.


futhermore, maybe you need do this befor run topio:

```
sudo ldconfig
```

then run topio:

```
topio -v
```


# Package
if you want to package topio and copy to another machine, use this:

```
# for topio-0.0.0.0-debug.tar.gz
sh pack.sh

# for topio-0.0.0.0-release.tar.gz
sh pack.sh release
```

And then copy this file to another machine to install topio:

```
tar topio-0.0.0.0-release.tar.gz
cd topio-0.0.0.0-release
sudo sh install.sh
```

futhermore, maybe you need do this befor run topio:

```
sudo ldconfig
```

then run topio:

```
topio -v
```

Good Luck!



