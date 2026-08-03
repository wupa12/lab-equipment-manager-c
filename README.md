# Lab Equipment Manager

一个使用标准 C11 编写的实验室仪器信息管理工具，提供交互式终端界面和可脚本化命令行接口。项目支持仪器信息的新增、查询、搜索、修改、删除、库存统计和本地持久化，并通过临时文件、磁盘同步与备份恢复降低保存中断造成的数据损坏风险。

项目只依赖 C 标准库和少量操作系统 API，不需要数据库服务或第三方运行库。

## 功能

- 仪器信息的完整 CRUD。
- 按唯一编号精确查询。
- 按名称、类别、规格、型号或日期进行关键字搜索。
- 自动生成递增编号，删除记录后不会改变其他仪器的编号。
- 统计仪器种类、类别、总数量、库存总金额和低库存条目。
- 支持名称、类别和规格中包含空格或中文。
- 使用 UTF-8 TSV 文件持久化，特殊字符会自动转义。
- 原子化保存：先写临时文件，再同步磁盘、备份旧文件并替换。
- 启动时可从中断保存遗留的 `.bak` 文件恢复。
- 所有输入均使用有边界的 `fgets`，不使用不安全的 `gets`。
- 同时提供交互模式和适合脚本调用的单命令模式。
- GCC 与 Clang 自动化构建测试。

## 数据模型

每条仪器记录包含：

| 字段 | 类型 | 约束 | 示例 |
|---|---|---|---|
| `id` | 正整数 | 自动生成且唯一 | `8` |
| `name` | UTF-8 文本 | 非空，少于 128 字节 | `数字示波器` |
| `category` | UTF-8 文本 | 非空，少于 128 字节 | `电子测量` |
| `specification` | UTF-8 文本 | 非空，少于 128 字节 | `100MHz 双通道` |
| `model` | UTF-8 文本 | 非空，少于 128 字节 | `DSO-100` |
| `purchase_date` | 日期文本 | `YYYY-MM-DD` | `2026-08-03` |
| `price` | 非负浮点数 | 大于等于 0 | `2599.50` |
| `quantity` | 非负整数 | 大于等于 0 | `6` |

## 快速开始

### 1. 准备编译器

推荐环境：

- GCC 或 Clang，支持 C11。
- 可选：CMake 3.16+。
- Windows PowerShell、Linux shell 或 macOS Terminal。

检查 GCC：

```bash
gcc --version
```

如果 Windows 已安装 MinGW，但 `gcc` 不在 PATH，可以临时添加：

```powershell
$env:Path = 'C:\path\to\mingw64\bin;' + $env:Path
gcc --version
```

### 2. Windows 构建与测试

在包含 `README.md`、`src` 和 `include` 的项目根目录打开 PowerShell：

```powershell
.\scripts\test.ps1
.\scripts\build.ps1
```

成功输出示例：

```text
All tests passed (38 assertions).
Built build/lab-manager.exe
```

生成的程序位于：

```text
build\lab-manager.exe
```

### 3. Linux/macOS 构建与测试

```bash
sh scripts/test.sh
sh scripts/build.sh
```

生成文件：

```text
build/lab-manager
```

也可以使用 Make：

```bash
make test
make
```

### 4. 使用 CMake

这是跨平台且适合 IDE 的构建方式：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Visual Studio、CLion 和 VS Code CMake Tools 均可直接打开项目根目录的 `CMakeLists.txt`。

## 使用示例

下文使用 Linux/macOS 的 `./build/lab-manager`。Windows 请替换为：

```text
.\build\lab-manager.exe
```

### 查看帮助

```bash
./build/lab-manager --help
```

### 使用内置示例数据

示例文件是只读演示数据，适合快速查看列表和统计：

```bash
./build/lab-manager --data examples/sample-instruments.tsv list
./build/lab-manager --data examples/sample-instruments.tsv stats
./build/lab-manager --data examples/sample-instruments.tsv search 化学
./build/lab-manager --data examples/sample-instruments.tsv show 3
```

列表输出示例：

```text
ID    | NAME                 | CATEGORY         | SPEC         | MODEL        | DATE       | PRICE   | QTY
------------------------------------------------------------------------------------------------------
1     | 酒精喷灯             | 化学仪器         | 大号         | AAZ99        | 2021-05-06 | 19.75   | 10
2     | 蒸馏烧瓶             | 化学仪器         | 大号         | YYU77        | 2022-02-26 | 15.91   | 83
...
共 7 条记录。
```

### 启动交互模式

```bash
./build/lab-manager
```

首次启动时数据库为空。新增数据后，程序会自动创建 `data/instruments.tsv`。

```text
========== 实验室仪器信息管理 ==========
1. 查看全部    2. 按编号查看    3. 搜索
4. 新增        5. 修改          6. 删除
7. 库存统计    0. 退出
请选择: 4
名称: 数字示波器
类别: 电子测量
规格: 100MHz 双通道
型号: DSO-100
购入日期 (YYYY-MM-DD): 2026-08-03
单价: 2599.50
数量: 6
新增成功，编号为 1。
```

再次启动程序会读取相同的数据文件，之前保存的记录仍然存在。

### 使用命令行完成完整 CRUD

命令行中的文本如果包含空格，需要使用引号。

新增：

```bash
./build/lab-manager add "数字示波器" "电子测量" "100MHz 双通道" "DSO-100" "2026-08-03" 2599.50 6
```

预期输出：

```text
新增成功，编号为 1。
```

查询：

```bash
./build/lab-manager show 1
```

```text
编号: 1
名称: 数字示波器
类别: 电子测量
规格: 100MHz 双通道
型号: DSO-100
购入日期: 2026-08-03
单价: 2599.50
数量: 6
库存金额: 15597.00
```

修改数量：

```bash
./build/lab-manager update 1 quantity 12
```

修改其他字段：

```bash
./build/lab-manager update 1 name "数字存储示波器"
./build/lab-manager update 1 category "电子测试设备"
./build/lab-manager update 1 specification "200MHz 四通道"
./build/lab-manager update 1 model "DSO-200X"
./build/lab-manager update 1 date "2026-08-10"
./build/lab-manager update 1 price 3199.00
```

支持的字段名称：`name`、`category`、`specification`、`model`、`date`、`price`、`quantity`。`spec` 和 `qty` 可作为简写。

搜索：

```bash
./build/lab-manager search "示波"
```

统计：

```bash
./build/lab-manager stats
```

```text
仪器种类数: 1
类别数: 1
库存总件数: 12
库存总金额: 38388.00
低库存条目（数量 < 10）: 0
库存金额最高: 数字存储示波器 (38388.00)
```

删除：

```bash
./build/lab-manager remove 1
```

### 使用独立的数据文件

`--data` 必须放在具体命令前面。父目录不存在时会自动创建：

```bash
./build/lab-manager --data data/lab-a.tsv add "电子天平" "计量设备" "0.001g" "B-1000" "2026-07-10" 1899 2
./build/lab-manager --data data/lab-a.tsv list

./build/lab-manager --data data/lab-b.tsv add "显微镜" "光学仪器" "1600x" "M-1600" "2026-07-11" 3200 5
./build/lab-manager --data data/lab-b.tsv list
```

两个路径对应两个互不影响的数据集合。

## 命令参考

```text
lab-manager [--data <file>]
lab-manager [--data <file>] list
lab-manager [--data <file>] show <id>
lab-manager [--data <file>] search <text>
lab-manager [--data <file>] stats
lab-manager [--data <file>] add <name> <category> <spec> <model> <date> <price> <qty>
lab-manager [--data <file>] update <id> <field> <value>
lab-manager [--data <file>] remove <id>
```

成功返回退出码 `0`；数据错误或未找到记录返回 `1`；命令格式错误返回 `2`，便于 shell 脚本判断执行结果。

## 文件格式与持久化安全

数据使用带标题行的 UTF-8 TSV：

```text
id<TAB>name<TAB>category<TAB>specification<TAB>model<TAB>purchase_date<TAB>price<TAB>quantity
1<TAB>数字示波器<TAB>电子测量<TAB>100MHz 双通道<TAB>DSO-100<TAB>2026-08-03<TAB>2599.50<TAB>6
```

字段中的反斜杠、Tab、换行和回车会分别保存为 `\\`、`\t`、`\n`、`\r`，加载时自动还原。程序拒绝字段缺失、编号重复、日期格式错误、负价格和负数量，不会静默跳过坏数据。

每次修改的保存流程：

1. 把完整数据写入 `<file>.tmp`。
2. 刷新 C 缓冲区并调用 `_commit` 或 `fsync`。
3. 将旧文件改名为 `<file>.bak`。
4. 将临时文件替换为正式文件。
5. 成功后删除备份。

如果进程在替换过程中意外停止，而正式文件不存在，下一次加载会尝试从 `.bak` 恢复。不要在程序运行时手工修改数据文件；重要数据仍应定期离线备份。

## 架构

```text
CLI / Interactive UI
        │
        ├── input.c          安全输入与数字解析
        ├── instrument.c     数据模型验证和搜索
        └── repository.c     CRUD、动态数组、TSV 与原子保存
```

业务逻辑不依赖终端界面，因此可以单独测试或复用。`InstrumentRepository` 使用动态数组，容量按需倍增；当前查找复杂度为 O(n)，适合中小型本地数据集合。

## 自动化测试

```bash
sh scripts/test.sh
```

Windows：

```powershell
.\scripts\test.ps1
```

当前测试包含 38 个断言，覆盖：

- 合法与非法数据验证。
- 整数、浮点数的严格解析。
- 新增、重复编号、查询、修改和删除。
- 自动编号计算。
- 中英文和带空格文本。
- Tab 与反斜杠转义后的保存/加载往返。
- 嵌套数据目录的自动创建。
- `.bak` 中断恢复。
- 异常 TSV 的拒绝与行号提示。

GitHub Actions 会分别使用 GCC 和 Clang 执行严格警告构建与测试。

## 项目结构

```text
.
├── .github/workflows/ci.yml
├── examples/sample-instruments.tsv
├── include/
│   ├── cli.h
│   ├── input.h
│   ├── instrument.h
│   └── repository.h
├── scripts/
│   ├── build.ps1
│   ├── build.sh
│   ├── test.ps1
│   └── test.sh
├── src/
│   ├── cli.c
│   ├── input.c
│   ├── instrument.c
│   ├── main.c
│   └── repository.c
├── tests/test_repository.c
├── CMakeLists.txt
├── Makefile
└── README.md
```

## 常见问题

### Windows 提示找不到 gcc

确认 MinGW 的 `bin` 目录已加入 PATH：

```powershell
$env:Path = 'C:\path\to\mingw64\bin;' + $env:Path
gcc --version
```

### 提示“加载失败”

检查 `--data` 指定的文件是否有读取权限，并确认它是本项目的八字段 TSV 格式。错误信息会包含无法解析的行号。

### 中文命令行参数乱码

Windows 版本会把系统代码页参数转换为 UTF-8，并将控制台切换为 UTF-8。建议使用 Windows Terminal 或新版 PowerShell，并使用本项目脚本重新编译。

### 修改后数据没有出现在预期文件

确认每次运行使用了同一个 `--data` 参数。不指定时默认路径是 `data/instruments.tsv`。

### 终端显示表格没有完全对齐

不同终端对中文字符宽度的处理不同，这只影响显示，不影响 UTF-8 数据内容。详细查看单条记录时可使用 `show <id>`。

## 已知限制与改进方向

- 当前数据启动时全部载入内存，适合中小规模数据。
- 查询使用线性扫描；大数据量可以增加哈希索引或 B 树。
- 当前是单进程工具，没有跨进程文件锁；不要同时用两个进程修改同一文件。
- 日期只验证格式和基本范围，可进一步加入闰年和每月天数校验。
- 可继续增加 CSV/JSON 导入导出、分页、排序、库存预警阈值和图形界面。

## License

本项目采用 [MIT License](LICENSE)。
