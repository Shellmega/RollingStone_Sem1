# RollingStone3D 最终运行包

本目录只提供当前最终版：[RollingStone3D_最终版_20261113.zip](RollingStone3D_最终版_20261113.zip)。

1. 在 GitHub 文件页点击下载按钮，保存 ZIP；或下载整个仓库。
2. 将 ZIP 完整解压到普通文件夹。
3. 双击 `RollingStone3D.exe` 启动默认场景；`run_elastic_3d.cmd` 启动弹性挡板场景，`run_obstacles_3d.cmd` 启动组合场景。
4. 详细操作、参数应用、结果统计和回放说明见压缩包中的 `README.md`。

运行环境：Windows 10/11 x64，显卡支持 OpenGL 3.3。字体、纹理、场景、自检工具和许可均在包内，不需要安装 Visual Studio。`selftests` 含22项自检；解压后可执行 `run_3d.cmd --self-test`。

本包来自工作区已验收的 `RollingStone3D_最终版_20261113` 发布目录。包含255个经过哈希核对的发布文件，以及文件清单；不包含用户本机的运行结果、旧版本、构建缓存或过程备份。校验值见 [SHA256.txt](SHA256.txt)。

需要修改程序时使用仓库中的 `src3d` 源码；源码构建与自检见 [项目 README](../README.md)。
