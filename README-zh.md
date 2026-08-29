[English](README.md)

# OpenMinecraft
新的引擎/客户端项目

## 子模块
- openminecraft-core/binary (字符串哈希，字节序处理)
- openminecraft-core/fontproc (字体整形封装，回退方案)
- openminecraft-core/i18n (国际化工具)
- openminecraft-core/io (JSON 解析器)
- openminecraft-core/log (日志系统)
- openminecraft-core/mem (内存追踪与分配器)
- openminecraft-core/network (网络传输封装，开发中)
- openminecraft-core/renderer (渲染器 API 后端抽象层)
- openminecraft-core/specs (JPEG、PNG、ZIP 等解析器)
- openminecraft-core/util (内部工具)
- openminecraft-core/vfs (虚拟文件系统)
- openminecraft-core/vm (Elysia VM 实现)
- openminecraft-core/world (数据管理)
- openminecraft (Demo)

## 如何构建
### 环境要求
xmake<br>
python<br>
cmake, meson, ninja（可选）

### 构建步骤
#### 下载资源
运行 ```python scripts/fetchsrc.py```

#### 打包 Bundle
##### Windows
确保已安装 ```zip``` 工具<br>
使用 ```choco install zip``` 安装<br>
使用以下命令创建所需的 bundle（PowerShell）
```
cd bootassets
zip -9 -r boot.bundle .
mv boot.bundle ..
cd ..
cd externalassets
zip -9 -r external.bundle .
mv external.bundle ..
cd ..
```
或者手动创建两个 bundle（文件夹 ```boot``` 和 ```externalassets```，不包含顶层文件夹），并保存为 ```boot.bundle``` 和 ```external.bundle```

##### 其他平台
运行 ```sh scripts/updateassets.sh```

#### 构建
运行 ```xmake``` 并等待完成

#### 演示
运行 ```xmake run openminecraft [gl/vk]``` 切换渲染后端