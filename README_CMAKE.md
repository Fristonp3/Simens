# GD32F450 Template (CMake)

## 目录约定

此工程按原模板工程的路径约定组织，`GD32_SDK_ROOT` 下应包含：

- `Firmware/`
- `Utilities/`
- `Template/`（当前目录）

默认 `GD32_SDK_ROOT` 为 `Template` 的上一级目录，即 `/home/fristonp/Simens`。

## 构建命令

```bash
cd /home/fristonp/Simens/Template
./scripts/setup_wsl_env.sh
./scripts/configure_build.sh /home/fristonp/Simens build
```

## 输出文件

构建后在 `build/` 下生成：

- `gd32f450_template.elf`
- `gd32f450_template.hex`
- `gd32f450_template.bin`
- `gd32f450_template.map`

## ST-Link 烧录
默认配置文件：`openocd_stlink.cfg`

```bash
cmake --build build --target flash
```

## 常见问题

- 若出现 `-march=nocona` 之类 x86 参数错误，通常是 Conda 环境变量污染；`configure_build.sh` 已自动清理 `CC/CXX/CFLAGS/...`。
- 若提示缺少 `Firmware/` 或 `Utilities/`，先执行：

```bash
./scripts/check_gd32_sdk.sh /home/fristonp/Simens
```
