# 修改CMake文件并构建运行echo程序

## 项目结构分析
- 主目录：/home/louis/VScodeProjects/MyWebServer
- 源代码目录：src/
  - src/reactor/：事件循环相关代码
  - src/server/：服务器相关代码
  - src/socket/：网络套接字相关代码
  - src/utils/：工具类代码
- 示例目录：example/
  - example/echo.cc：示例echo服务器程序

## 问题分析
1. **主CMakeLists.txt**：缺少对源代码的编译配置，没有创建库文件
2. **子CMakeLists.txt**：路径配置错误，无法正确找到源文件

## 解决方案

### 1. 修改主CMakeLists.txt
- 添加对src目录下所有源代码的编译配置
- 创建一个静态库，包含所有核心代码
- 设置正确的包含路径

### 2. 修改example/CMakeLists.txt
- 修正源文件路径
- 移除错误的GLOB_RECURSE命令
- 添加对主库的链接

### 3. 构建和运行
- 创建build目录
- 使用cmake配置项目（使用ninja生成器）
- 使用ninja编译项目
- 运行echo程序

## 具体修改内容

### 主CMakeLists.txt修改
```cmake
cmake_minimum_required(VERSION 3.14)
project(MyWebServer LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

# 收集所有源文件
file(GLOB_RECURSE SOURCES src/*/*.cc)

# 创建静态库
add_library(MyWebServer STATIC ${SOURCES})

# 设置包含路径
target_include_directories(MyWebServer PUBLIC ${PROJECT_SOURCE_DIR}/src)

add_subdirectory(example)
```

### example/CMakeLists.txt修改
```cmake
# 添加可执行文件
add_executable(echo echo.cc)

# 链接主库
target_link_libraries(echo PRIVATE MyWebServer)

# 设置包含路径
target_include_directories(echo PRIVATE ${PROJECT_SOURCE_DIR}/src)
```

## 构建步骤
1. 创建build目录：`mkdir -p build && cd build`
2. 配置项目：`cmake -G Ninja ..`
3. 编译项目：`ninja`
4. 运行程序：`./example/echo`