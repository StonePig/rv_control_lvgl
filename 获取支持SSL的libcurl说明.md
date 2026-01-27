# 获取支持 SSL 的 libcurl.so 文件

## ⚠️ 重要说明

由于预构建的支持 SSL 的 libcurl.so 文件通常不提供直接下载链接，**最可靠的方法是自行编译**。

## 推荐方案：使用 robertying/openssl-curl-android 编译（最推荐）

### 步骤：

1. **安装 Android NDK**
   - 下载并安装 Android NDK (推荐 r21e 或更高版本)
   - 设置环境变量 `ANDROID_NDK_HOME`

2. **克隆仓库**
   ```bash
   git clone https://github.com/robertying/openssl-curl-android.git
   cd openssl-curl-android
   ```

3. **编译**
   ```bash
   # 编译所有架构
   ./build.sh
   
   # 或单独编译某个架构
   ./build.sh arm64-v8a
   ```

4. **提取文件**
   - 编译完成后，在 `build/` 目录下找到各架构的 `libcurl.so` 文件
   - 复制到项目的对应目录

### 优点：
- ✅ 支持 SSL/TLS (OpenSSL)
- ✅ 支持所有架构 (arm64-v8a, armeabi-v7a, x86, x86_64)
- ✅ 社区维护良好（234+ stars）
- ✅ 可以自定义编译选项

## 替代方案

### 方案 1: Poko-Apps/curl-openssl-android
- **地址**: https://github.com/Poko-Apps/curl-openssl-android
- **说明**: 通过 GitHub Actions 自动构建，可能有 releases
- **支持**: arm64-v8a, armeabi-v7a, x86, x86_64

### 方案 2: vvb2060/curl-android (静态库)
- **地址**: https://github.com/vvb2060/curl-android
- **说明**: 提供静态库（.a），如果项目使用静态链接可以考虑
- **Maven**: `implementation("io.github.vvb2060.ndk:curl:8.18.0")`
- **注意**: 这是静态库，不是动态库 (.so)

### 方案 3: ObsidianX/android-libcurl-builder
- **地址**: https://github.com/ObsidianX/android-libcurl-builder
- **说明**: 简单的构建脚本
- **支持**: armeabi-v7a, arm64-v8a

## 文件替换位置

下载或编译完成后，将 `libcurl.so` 文件放置到以下目录：

```
app/src/main/cpp/jniLibs/
├── arm64-v8a/
│   └── libcurl.so
├── armeabi-v7a/
│   └── libcurl.so
├── x86/
│   └── libcurl.so
└── x86_64/
    └── libcurl.so
```

## 依赖库

如果使用 OpenSSL 版本，可能还需要以下依赖库（放在相同目录）：

- `libssl.so` - OpenSSL SSL 库
- `libcrypto.so` - OpenSSL 加密库

## 验证 SSL 支持

替换后，可以通过以下方式验证：

1. **编译项目**
   ```bash
   ./gradlew assembleDebug
   ```

2. **运行时检查**
   - 使用 `curl_version()` 函数检查特性
   - 尝试访问 HTTPS URL 测试

3. **检查符号**
   ```bash
   nm -D libcurl.so | grep ssl
   ```

## 备份

**重要**: 替换前请备份原有文件！

```bash
# 备份脚本示例
for arch in arm64-v8a armeabi-v7a x86 x86_64; do
    cp app/src/main/cpp/jniLibs/$arch/libcurl.so app/src/main/cpp/jniLibs/$arch/libcurl.so.backup
done
```

## 故障排除

如果遇到问题：

1. **库加载失败**: 检查架构是否匹配
2. **SSL 错误**: 确认依赖库（libssl.so, libcrypto.so）已正确放置
3. **版本不兼容**: 确保 API 级别 >= 21（项目要求）

## 快速开始脚本

已创建 PowerShell 脚本 `download_libcurl_final.ps1`，运行它查看当前文件状态和详细说明。
