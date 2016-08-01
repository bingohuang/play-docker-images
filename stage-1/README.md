第一期 - 童年的马里奥 (Stage 1 - Childhood Mario)
===
![docker-mario](http://nos.126.net/comb/docker-mario-game.jpg)


## 介绍
`黑白机小霸王`，可以说是我们小时候最大的娱乐方式了，其中又以`马里奥（Mario）`最让人印象深刻(小时候看不懂英文，我们都叫它`顶蘑菇`)。本期《玩转Docker镜像》推出 `Docker` 版`在线马里奥`小游戏，一起来找回童年的记忆吧。（通过蜂巢平台运行还可以让你的童年小伙伴一起来玩耍哦）

![docker-mario](http://nos.126.net/comb/docker-mario.png)

游戏界面如上图所示，界面非常简单，开箱即玩，这里有一个蜂巢平台上搭建好的试玩地址： [`http://59.111.100.173`](http://59.111.100.173)

操作方式：
+ 移动（Move）：`上下左右箭头`或者`WSDL`四键
+ 发射（Fire）：`Shift`键
+ 冲刺（Sprint）：`CTRL`键
+ 暂停（Pause）：`P`键
+ 静音（Mute）：`M`键

Let's Play!

注： PC上运营更佳，需要键盘支持哦，以上地址为试玩，不会保证一直可用。

## Dockerfile
```
FROM hub.c.163.com/bingohuang/debian:163

MAINTAINER bingohuang <me@bingohuang.com>

RUN apt-get update && apt-get install -y nginx

COPY docker-mario /usr/share/nginx/www

EXPOSE 80

ENTRYPOINT ["nginx", "-g", "daemon off;"]
```
#### Dockerfile 解析：
以下对Dockerfile做逐句解析

1. 关键字 `FROM`，选用包含163更新源的Debian镜像，版本7.9
2. 关键字 `MAINTAINER`，添加作者信息和联系方式（有任何问题或反馈欢迎发邮件沟通）
3. 关键字 `RUN`，运行命令，这里是更新程序列表并安装 `Nginx` 程序
4. 关键字 `COPY`，将 `docker-mario` 源码拷贝到容器的 `/usr/share/nginx/www` 目录下（这是第三步安装好 `Nginx` 程序后自动生成的目录
5. 关键字 `EXPOSE`，暴露端口，这里是 `Nginx` 的默认端口：80
6. 关键字 `ENTRYPOINT`，指定容器运行的默认指令（不会被用户指令覆盖）


## 镜像

#### 构建镜像
```
docker build -t hub.c.163.com/bingohuang/docker-mario:1.0 .
```
#### 查看镜像
```
docker images

REPOSITORY                              TAG                 DIGEST              IMAGE ID            CREATED             SIZE
hub.c.163/bingohuang/docker-mario       1.0                 <none>              48c6e8c697c2        44 seconds ago      205.6 MB
```
#### 上传镜像
注：如果想先本地运行，直接跳到下一步：`容器-本地运行`
```
# 登录蜂巢镜像中心
docker login hub.c.163.com
Username: <蜂巢邮箱>
Password: <蜂巢密码>
Email: <蜂巢邮箱> #最新版的Docker无需输入Email

# 推送到镜像中心
docker push hub.c.163.com/bingohuang/docker-mario:1.0
```

## 容器
#### 本地运行
```
# 运行
docker run --name docker-mario -d -p 89:80 hub.c.163.com/bingohuang/docker-mario:1.0

# 查看
docker ps

# 访问
打开浏览器，访问 http://127.0.0.1:89/
```
#### 蜂巢运行
Docker镜像能做到一次构建，到处运行，所以镜像是核心。

首先打开 [蜂巢镜像中心](https://c.163.com/hub#/m/home/)，如果你在上一步做了 `镜像-上传镜像` 到蜂巢平台，那将在你的[私有仓库](https://c.163.com/dashboard#/m/mirrorRepo/)中看到你刚刚上传的镜像。

如果你想用我们打包上传好的镜像，只需要在镜像中心的搜索框页面，输入 `docker-mario`，结果中的 `bingohuang/docker-mario` 就是我们上传好的镜像了。 点击`收藏`，之后就可以在`容器管理`页面创建该镜像的容器。

## 演示
视频地址：
