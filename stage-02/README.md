第二期 - 制作精简镜像(上) (Stage 2 - Make Minimal Image I)
===
![docker-mario](http://nos.126.net/docker/docker-images.png)


## 介绍
前段时间网易蜂巢曾经推出蜂巢 `Logo` T恤，用的正是 Docker 镜像制作，最神奇的是，它最终的镜像大小只有 `585` 字节。
```bash
$ docker images | grep hub.c.163.com/public/logo
REPOSITORY                      TAG           IMAGE ID         CREATED       SIZE
hub.c.163.com/public/logo       latest        6fbdd13cd204     11 days ago   585 B
```
[点击此处](https://blog.c.163.com/2016/07/67/)，可以了解该镜像的制作过程，这其中就用到了不少精简镜像的技术，尤其是针对 C 程序的优化和精简。但我们平常开发肯定不止用 C 语言，甚至有些镜像都不是我们自己来打包的（比如下载公共镜像），那是否有一些通用的精简 Docker 镜像的手段呢？答案是肯定的，甚至有的镜像可以精简 98%。精简镜像大小的好处不言而喻，既节省了存储空间，又能节省带宽，加快传输等。那好，接下来就请跟随我来学习怎么制作精简 Docker 镜像。

## 镜像层(Layers)
在开始制作镜像之前，首先了解下镜像的原理，而这其中最重要的概念就是`镜像层(Layers)`。镜像层依赖于一系列的底层技术，比如文件系统(filesystems)、写时复制(copy-on-write)、联合挂载(union mounts)等，幸运的是你可以在很多地方学习到[这些技术](https://docs.docker.com/engine/userguide/storagedriver/imagesandcontainers/)，这里就不再赘述技术细节。


![docker-image-layers](http://nos.126.net/docker/docker-image-layers.png)

总的来说，你最需要记住这点：
```
在 Dockerfile 中， 每一条指令都会创建一个镜像层，继而会增加整体镜像的大小。
```

举例来说：

```Dockerfile
FROM busybox
RUN mkdir /tmp/foo
RUN dd if=/dev/zero of=/tmp/foo/bar bs=1048576 count=100
RUN rm /tmp/foo/bar
```

以上 Dockerfile 干了几件事：
1. 基于一个官方的基础镜像 busybox(只有1M多)
2. 创建一个文件夹(/tmp/foo)和一个文件(bar)，该文件分配了100M大小
3. 再把这个大文件删除

实际上它最终什么也没做，我们把它构建成镜像（构建可以参考[一期](https://github.com/bingohuang/play-docker-images/tree/master/stage-01)）：
```
docker build -t busybox:test .
```
再让我们来对比下原生的 busybox 镜像大小和我们生成的镜像大小：
```bash
$ docker images | grep busybox
busybox    test     896c63dbdb96    2 seconds ago    106 MB
busybox    latest   2b8fd9751c4c    9 weeks ago      1.093 MB
```
出乎意料的是，却生成了 106 MB 的镜像。

多出了 100 M，这是为何？这点和 Git 类似（都用到了Copy-On-Write技术），我用 git 做了如下两次提交（添加了又删除），请问 `A_VERY_LARGE_FILE` 还在 git 仓库中吗？
```git
$ git add  A_VERY_LARGE_FILE
$ git commit
$ git rm  A_VERY_LARGE_FILE
$ git commit
```
答案是:在的，并且会占用仓库的大小。Git 会保存每一次提交的文件版本，而 Dockerfile 中每一条指令都可能增加整体镜像的大小，即使它最终什么事情都没做。

## 制作步骤
了解了镜像层知识，有助于我们接下来制作精简镜像。这里开始，以最常用的开源缓存软件 `Redis` 为例，从一步步试验，来介绍如何制作更精简的 Docker 镜像。

### lab-1：初始化构建 Redis 镜像
**直接上 `Dockerfile` ：**

```dockerfile
FROM ubuntu:trusty
ENV VER     3.0.0
ENV TARBALL http://download.redis.io/releases/redis-$VER.tar.gz
# ==> Install curl and helper tools...
RUN apt-get update
RUN apt-get install -y  curl make gcc
# ==> Download, compile, and install...
RUN curl -L $TARBALL | tar zxv
WORKDIR  redis-$VER
RUN make
RUN make install
#...
# ==> Clean up...
WORKDIR /
RUN apt-get remove -y --auto-remove curl make gcc
RUN apt-get clean
RUN rm -rf /var/lib/apt/lists/*  /redis-$VER
#...
CMD ["redis-server"]
```

结合注释，读起来并不困难，用到的都是常规的几个命令，简要介绍如下：
+ FROM：顶头写，指定一个基础镜像，此处基于 `ubuntu:trusty`
+ ENV：设置环境变量，这里设置了 `VER` 和 `TARBALL` 两个环境变量
+ RUN：最常用的 Dockerfile 指令，用于运行各种命令，这里调用了 8 次 RUN 指令
+ WORKDIR：指定工作目录，相当于指令 `cd`
+ CMD：指定镜像默认执行的命令，此处默认执行 redis-server 命令来启动 redis

**执行构建：**

```bash
$ docker build  -t redis:lab-1  .
```
_注：国内网络，更新下载可能会较慢_

**查看大小：**

| Lab |  iamge  | Base       | Lang    | .red[*] |  Size (MB) | &nbsp;&nbsp; Memo               |
|:---:|:--------|:-----------|:-----:|:---:|---------------:|:--------------------------------|
|  1 |  redis  |  `ubuntu`  |   C   | dyn |   347.3        | &nbsp;&nbsp; base ubuntu        |

动辄就有 300多 M 的大小，不能忍，下面我们开始一步步优化。


### lab-2: 优化基础镜像

**精简1：选用更小的基础镜像。**

常用的 Linux 系统镜像一般有 `ubuntu`、`centos`、`debian`，其中`debian` 更轻量，而且够用，对比如下：
```
REPOSITORY          TAG        IMAGE ID         VIRTUAL SIZE
---------------     ------     ------------     ------------
centos              7          214a4932132a     215.7 MB
centos              6          f6808a3e4d9e     202.6 MB
ubuntu              trusty     d0955f21bf24     188.3 MB
ubuntu              precise    9c5e4be642b7     131.9 MB
debian              jessie     65688f7c61c4     122.8 MB
debian              wheezy     1265e16d0c28     84.96 MB
```
替换 `debian:jessie` 作为我们的基础镜像。

**优化 Dockerfile：**
```dockerfile
FROM debian:jessie

#...
```
**执行构建：**
```bash
$ docker build  -t redis:lab-2  .
```

**查看大小：**

| Lab |  image  | Base       | Lang    | .red[*] |  Size (MB) | &nbsp;&nbsp; Memo               |
|:---:|:--------|:-----------|:-----:|:---:|---------------:|:--------------------------------|
|  01 |  redis  |  `ubuntu`  |   C   | dyn |   347.3        | &nbsp;&nbsp; base ubuntu        |
|  02 |  redis  |  `debian`  |   C   | dyn |   305.7        | &nbsp;&nbsp; base debian        |

减少了42M，稍有成效，但并不明显。细心的同学应该发现，只有 122 MB 的 `debian` 基础镜像，构建后增加到了 305 MB，看来这里面肯定有优化的空间，如何优化就要用到我们开头说到的 `Image Layer` 知识了。

### lab-3：串联 Dockerfile 指令

**精简2： 串联你的 Dockerfile 指令（一般是 `RUN` 指令）。**

Dockerfile 中的 RUN 指令通过 `&&` 和 `/` 支持将命令串联在一起，有时能达到意想不到的精简效果。

**优化 Dockerfile：**
```dockerfile
FROM debian:jessie

ENV VER     3.0.0
ENV TARBALL http://download.redis.io/releases/redis-$VER.tar.gz


RUN echo "==> Install curl and helper tools..."  && \
    apt-get update                      && \
    apt-get install -y  curl make gcc   && \
    \
    echo "==> Download, compile, and install..."  && \
    curl -L $TARBALL | tar zxv  && \
    cd redis-$VER               && \
    make                        && \
    make install                && \
    ...
    echo "==> Clean up..."  && \
    apt-get remove -y --auto-remove curl make gcc  && \
    apt-get clean                                  && \
    rm -rf /var/lib/apt/lists/*  /redis-$VER

#...
CMD ["redis-server"]
```

构建：
```bash
$ docker build  -t redis:lab-3  .
```

查看大小：

| Lab |  Image  | Base       | Lang    | .red[*] |  Size (MB) | &nbsp;&nbsp; Memo               |
|:---:|:--------|:-----------|:-----:|:---:|---------------:|:--------------------------------|
|  01 |  redis  |  `ubuntu`  |   C   | dyn |   347.3        | &nbsp;&nbsp; base ubuntu        |
|  02 |  redis  |  `debian`  |   C   | dyn |   305.7        | &nbsp;&nbsp; base debian        |
|  03 |  redis  |  `debian`  |   C   | dyn |   151.4        | &nbsp;&nbsp; cmd chaining       |

哇！一下子减少了 50%，效果明显啊！这是最常用的一个精简手段了。

### lab-4：压缩你的镜像

**优化3：试着用命令或工具压缩你的镜像。**

docker 自带的一些命令还能协助压缩镜像，比如 `export` 和 `import`
```bash
$ docker run -d redis:lab-3
$ docker export 71b1c0ad0a2b | docker import - redis:lab-4
```

但麻烦的是需要先将容器运行起来，而且这个过程中你会丢失镜像原有的一些信息，比如：导出端口，环境变量，默认指令。

所以一般通过命令行来精简镜像都是实验性的，那么这里再推荐一个小工具： [docker-squash](https://github.com/jwilder/docker-squash)。用起来更简单方便，并且不会丢失原有镜像的自带信息。

**下载安装：**

https://github.com/jwilder/docker-squash#installation

**压缩操作：**
```
$ docker save redis:lab-3 \
  | sudo docker-squash -verbose -t redis:lab-4  \
  | docker load
```
_注： 该工具在 Mac 下并不好使，请在 Linux 下使用_

**对比大小：**

| Lab |  Image  | Base       | PL    | .red[*] |  Size (MB) | &nbsp;&nbsp; Memo               |
|:---:|:--------|:-----------|:-----:|:---:|---------------:|:--------------------------------|
|  01 |  redis  |  `ubuntu`  |   C   | dyn |   347.3        | &nbsp;&nbsp; base ubuntu        |
|  02 |  redis  |  `debian`  |   C   | dyn |   305.7        | &nbsp;&nbsp; base debian        |
|  03 |  redis  |  `debian`  |   C   | dyn |   151.4        | &nbsp;&nbsp; cmd chaining       |
|  04 |  redis  |  `debian`  |   C   | dyn |   151.4        | &nbsp;&nbsp; docker-squash      |

好吧，从这里看起来并没有太大作用，所以我只能说`试着`，而不要报太大期望。

## 总结
本期我们介绍了镜像层的知识，并且通过实验，介绍三种如何精简镜像的技巧（下期还有更强大的技巧）。这里主要介绍了三种精简方法：选用更精小的镜像，串联 Dockerfile 运行指令，以及试着压缩你的镜像。通过这几个技巧，已经可以将 300M 大小的镜像压缩到 150M，压缩率50%，效果还是不错。但这还远远不够，下期将介绍一些终极手段，压缩效果可以达到 `98%`，敬请关注吧！

## 参考
+ ☛ [Dockerfile Best Practices - take 2](http://crosbymichael.com/dockerfile-best-practices-take-2.html) - by Michael Crosby, 2014-03-09.
+ ☛ [Optimizing Docker Images](http://www.centurylinklabs.com/optimizing-docker-images/) - by Brian DeHamer, 2014-07-28.
+ ☛ [Squashing Docker Images](http://jasonwilder.com/blog/2014/08/19/squashing-docker-images/) - by Jason Wilder, 2014-08-19.

## 视频
@[网易云课堂](http://study.163.com) -《[玩转 Docker 镜像](http://study.163.com/course/courseMain.htm?courseId=1003188013)》- [第二期：制作精简镜像(上)]()
