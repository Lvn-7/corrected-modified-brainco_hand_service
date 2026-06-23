* 每只手（左或右）通过一个 USB-转-串口设备进行控制，并各自生成一对主题：`rt/brainco/(left or right)/(cmd or state)`。

* 手指的位置和速度都被归一化到 [0, 1] 的范围。

* 推荐将所有手指速度都设置为 1.0。

* 手指索引映射如下：[拇指、拇指副指、食指、中指、无名指、小指]。

> 你还可以参考一个类似项目 [unitree-g1-brainco-hand](https://github.com/BrainCoTech/unitree-g1-brainco-hand)，该项目由 BrainCoTech 适配。

# 1. 📦 安装

```bash
# 在用户开发计算单元 PC2（NVIDIA Jetson Orin NX 板）上
sudo apt install libspdlog-dev libfmt-dev
cd ~
git clone https://github.com/unitreerobotics/brainco_hand_service
cd brainco_hand_service
mkdir build && cd build
cmake ..
make -j6
```

# 2. 🚀 启动

```bash
cd ~/brainco_hand_service/bin
# 运行 `sudo ./brainco_hand_server -h` 获取更多信息。输出如下：
# Unitree Brainco Hand Service:
#  -h [ --help ]                  显示帮助信息
#  -v [ --version ]               显示版本
#  -n [ --network_interface ] arg 指定 DDS 网络接口

# 启动服务
sudo ./brainco_hand_server --network eth0
# 简化（使用默认配置）
sudo ./brainco_hand_server

# 另一个终端，运行测试示例
# 用法: ./test_brainco_hand_server [left|right]
# 若未指定，默认为 left。
# 正常情况下，你会看到灵巧手反复做握拳和张开动作。

# 测试左手
cd ~/brainco_hand_service/bin
sudo ./test_brainco_hand_server
# 或测试右手
sudo ./test_brainco_hand_server right
```

# 2.1 🎛️ 单次控制测试

`brainco_hand_cli` 提供统一的单次控制接口，适合调试指定手、指定手指、指定位置和速度。

```bash
cd ~/brainco_hand_service/bin

# 格式：
# sudo ./brainco_hand_cli <left|right|both> <finger_index|all|both> <position|open|close> [speed]
# sudo ./brainco_hand_cli <left|right|both> <open|close> [speed]
#
# finger_index: 0~5 = [拇指、拇指副指、食指、中指、无名指、小指]
# position: 0.0 表示张开，1.0 表示闭合，可以使用 0.5 这类中间值
# speed: 0.0~1.0，未指定时默认为 1.0

# 控制左手第 3 个手指到半闭合，速度 1.0
sudo ./brainco_hand_cli left 3 0.5 1

# 右手所有手指张开
sudo ./brainco_hand_cli right both open

# 左右手全部闭合
sudo ./brainco_hand_cli both close

# 左右手所有手指到 30% 闭合，速度 0.8
sudo ./brainco_hand_cli both all 0.3 0.8
```

# 3. 🚀🚀🚀 开机自启服务

完成上述安装和配置，并成功运行 test_brainco_hand_server 后，你可以通过以下脚本将 test_brainco_hand_server 配置为系统开机自动启动：

```bash
cd ~/brainco_hand_service
bash setup_autostart.sh
```

根据脚本提示完成配置即可。



# ❓ 常见问题

1. `make -j6` 出错：

   ```bash
   unitree@ubuntu:~/brainco_hand_service/build$ make -j6
   Scanning dependencies of target brainco_hand_server
   Scanning dependencies of target test_brainco_hand_server
   [ 50%] Building CXX object CMakeFiles/test_brainco_hand_server.dir/test/test_brainco_hand_server.cpp.o
   [ 50%] Building CXX object CMakeFiles/brainco_hand_server.dir/main.cpp.o
   /home/unitree/brainco_hand_service/test/test_brainco_hand_server.cpp:1:10: fatal error: unitree/idl/go2/MotorCmds_.hpp: No such file or directory
       1 | #include <unitree/idl/go2/MotorCmds_.hpp>
         |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   /home/unitree/brainco_hand_service/main.cpp:1:10: fatal error: unitree/idl/go2/MotorCmds_.hpp: No such file or directory
       1 | #include <unitree/idl/go2/MotorCmds_.hpp>
         |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   compilation terminated.
   compilation terminated.
   ```

   该错误说明 unitree_sdk2 头文件未找到。先编译并安装 unitree_sdk2：

   ```bash
   cd ~
   git clone https://github.com/unitreerobotics/unitree_sdk2
   cd unitree_sdk2
   mkdir build & cd build
   cmake ..
   sudo make install
   ```
