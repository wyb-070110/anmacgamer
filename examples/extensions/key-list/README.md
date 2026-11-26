# 键位清单（`key-list`）

简体中文 · [English](README.en.md)

演示 `contributes.converters`：将纯文本 `.keys` 文件导入为方案，也将方案导出为 `.keys`。不需要任何权限。

## 格式

```text
# Arena basics
J 0.82 0.71 Attack
Space 0.94 0.80 Dodge
1 0.40 0.92 Potion
```

首行 `#` 开头为方案名，其余 `#` 行为注释。每行写 `键 x y [名字]`（`x`/`y` 为 `0..1` 比例）。导入生成单击控件；导出写出单键且有位置的控件，其余记在末尾注释。

## 试一试

1. 校验、检查并打包：
   ```powershell
   gs validate examples\extensions\key-list
   gs check examples\extensions\key-list
   gs pack examples\extensions\key-list -o key-list.gsext
   ```
2. 在扩展页点**从文件安装…**安装并启用。
3. 在方案页点**导入**并选择 `samples\arena.keys`（或拖入投屏窗口），生成“Arena basics”方案。
4. 在方案卡片菜单选**导出为 键位清单…**即可导出。

两个转换器共用 `src/main.ts`（`convert(input)` 按 `input.to` 区分，导入返回 `gamescrcpy.profile` v3 的 `{ name, json }`，导出返回 `{ text }`）。
