第三期 - 制作精简镜像(下) (Stage 3 - Make Minimal Image II)
===
![docker-mario](http://nos.126.net/docker/docker-commands.png)

## 介绍
查看 docker hub，我们往往能看到很多极小的镜像，如下：
TODO：table
这么小的镜像是怎么做出来的呢？
##

## 制作步骤

### lab-5: 使用最精简的 base image

使用 `scratch` 或者 `busybox` 作为基础镜像。

关于 scratch：

+ 一个空镜像，只能用于构建镜像，通过 `FROM scratch`
+ 在构建一些基础镜像，比如 `debian` 、 `busybox`，非常有用
+ 用于构建超少镜像，比如构建一个包含所有库的二进制文件

关于 `busybox`
+ 只有 1~5M 的大小
+ 包含了常用的 UNIX 工具
+ 非常方便构建小镜像

这些超小的基础镜像，结合能生成静态原生 ELF 文件的编译语言，比如C/C++，比如 Go，特别方便构建超小的镜像。

cloudcomb-logo（C语言开发） 就是用到了该原理，才能构建出 585 字节的镜像。

`redis` 同样使用 C语言 开发，看来也有很大的优化空间，下面这个实验，让我们介绍具体的操作方法。

### lab-6: 提取动态链接的 .so 文件

实验上下文：
```bash
$ cat /etc/os-release

NAME="Ubuntu"
VERSION="14.04.2 LTS, Trusty Tahr"
```

```bash
$ uname -a
Linux localhost 3.13.0-46-generic #77-Ubuntu SMP
Mon Mar 2 18:23:39 UTC 2015
x86_64 x86_64 x86_64 GNU/Linux
```

隆重推出 ldd：打印共享的依赖库
```bash
$ ldd  redis-3.0.0/src/redis-server
    linux-vdso.so.1 =>  (0x00007fffde365000)
    libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f307d5aa000)
    libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007f307d38c000)
    libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f307cfc6000)
    /lib64/ld-linux-x86-64.so.2 (0x00007f307d8b9000)
```

将所有需要的 .so 文件打包：
```bash
$ tar ztvf rootfs.tar.gz
4485167  2015-04-21 22:54  usr/local/bin/redis-server
1071552  2015-02-25 16:56  lib/x86_64-linux-gnu/libm.so.6
 141574  2015-02-25 16:56  lib/x86_64-linux-gnu/libpthread.so.0
1840928  2015-02-25 16:56  lib/x86_64-linux-gnu/libc.so.6
 149120  2015-02-25 16:56  lib64/ld-linux-x86-64.so.2
```

再制作成 Dockerfile：
```dockerfile
FROM scratch
ADD  rootfs.tar.gz  /
COPY redis.conf     /etc/redis/redis.conf
EXPOSE 6379
CMD ["redis-server"]
```

执行构建：
```bash
$ docker build  -t redis-05  .
```

查看大小：

| Lab |         | Base       | PL    | .red[*] |  Size (MB) | &nbsp;&nbsp; Memo               |
|:---:|:--------|:-----------|:-----:|:---:|---------------:|:--------------------------------|
|  01 |  redis  |  `ubuntu`  |   C   | dyn |   347.3        | &nbsp;&nbsp; base ubuntu        |
|  02 |  redis  |  `debian`  |   C   | dyn |   305.7        | &nbsp;&nbsp; base debian        |
|  03 |  redis  |  `debian`  |   C   | dyn |   151.4        | &nbsp;&nbsp; cmd chaining       |
|  04 |  redis  |  `debian`  |   C   | dyn |   151.4        | &nbsp;&nbsp; docker-squash      |
|  05 |  redis  |  `scratch` |   C   | dyn |    7.73        | &nbsp;&nbsp; rootfs: .so        |

哇！显著提高啦！

测试一下：

```bash
$ docker run -d --name redis-05 redis-05

$ redis-cli  -h  \
  $(docker inspect -f '{{.NetworkSettings.IPAddress}}' redis-05)

$ redis-benchmark  -h  \
  $(docker inspect -f '{{.NetworkSettings.IPAddress}}' redis-05)
```

总结一下：

1. 用 `ldd` 查出所需的 .so 文件
2. 将所有依赖压缩成 `rootfs.tar` 或 `rootfs.tar.gz`，之后打进 `scratch` 基础镜像

### lab-7: 为 Go 应用构建精简镜像

Go 语言天生就方便用来构建精简镜像，得益于它能方便的打包成包含静态链接的二进制文件。

打个比方，你有一个 4 MB 大小的包含静态链接的 Go 二进制，并且将其打进 scratch 这样的基础镜像，你得到的镜像大小也只有区区的 4 MB。这可是包含同样功能的 Ruby 程序的百分之一啊。

这里再给大家介绍一个非常好用开源的 Go 编译工具：golang-builder，并给大家实际演示一个例子

程序代码：

```go
package main // import "github.com/CenturyLinkLabs/hello"

import "fmt"

func main() {
    fmt.Println("Hello World")
}
```

Dockerfile：

```dockerfile
FROM scratch
COPY hello /
ENTRYPOINT ["/hello"]
```

通过 golang-builder 打包成镜像：

```bash
docker run --rm \
    -v $(pwd):/src \
    -v /var/run/docker.sock:/var/run/docker.sock \
    centurylink/golang-builder
```

查看镜像大小(Mac下测试)：

```bash
$ docker images
REPOSITORY   TAG      IMAGE ID       CREATED          VIRTUAL SIZE
hello        latest   1a42948d3224   24 seconds ago   1.59 MB
```

哇！这么省力，就能创建几 M 大小的镜像，Go 简介就是为 Docker 镜像量身定做的！

## 总结
1. 优化基础镜像
2. 串接 Dockerfile 命令：
3. 压缩 Docker images
4. 优化程序依赖
5. 选用更合适的开发语言

## 参考
- ☛ [scratch in Docker Hub](https://registry.hub.docker.com/_/scratch/)
- ☛ [Make FROM scratch a special cased 'no-base' spec](https://github.com/docker/docker/pull/8827)
- ☛ [vDSO (virtual dynamic shared object)](http://en.wikipedia.org/wiki/VDSO)
- ☛ [Small Docker Images For Go Apps](http://www.centurylinklabs.com/small-docker-images-for-go-apps/) (with [golang-builder](https://github.com/CenturyLinkLabs/golang-builder))
- ☛ [Building Docker Images for Static Go Binaries](https://medium.com/@kelseyhightower/optimizing-docker-images-for-static-binaries-b5696e26eb07)

## 视频
@[网易云课堂](http://study.163.com) -《[玩转 Docker 镜像](http://study.163.com/course/courseMain.htm?courseId=1003188013)》- [第三期：制作精简镜像(下)]()
