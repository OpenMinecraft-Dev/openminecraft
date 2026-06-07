#!/usr/bin/env python
import re

def parse_parameters(param_str: str):
    """
    解析参数列表字符串，返回 [(类型, 名称), ...]
    例如: "OMElysiaJNIEnv *env, void *address, jlong capacity"
    """
    if not param_str.strip():
        return []

    # 按逗号分割
    params = [p.strip() for p in param_str.split(',') if p.strip()]
    result = []
    for param in params:
        # 正则：非贪婪捕获类型部分，然后空格和指针符号，最后是参数名
        # 类型部分可能包含 const、unsigned、struct 等，名称前可能有 * 或多个 *
        match = re.match(r'^(.+?)\s*([*\s]*)\s*([_a-zA-Z][_a-zA-Z0-9]*)\s*$', param)
        if not match:
            if param == "...":
                result.append((param, ""))
                continue
            raise ValueError(f"无法解析参数: {param}")
        base_type = match.group(1).strip()
        ptr_space = match.group(2).strip()  # 包含 * 和可能空格
        param_name = match.group(3)
        # 将指针符号合并回类型
        param_type = f"{base_type}{' ' if ptr_space.startswith('*') else ''}{ptr_space}".strip()
        result.append((param_type, param_name))
    return result

def parse_function_pointer(definition: str):
    pattern = r'^([\w\s*]+?)\(\*(\w+)\)\((.*)\);$'
    match = re.match(pattern, definition.strip())
    if not match:
        raise ValueError("无法解析该函数指针定义")
    return_type = match.group(1).strip()
    name = match.group(2)
    params = match.group(3)
    return return_type, name, params

def process(s):
    type, n, args = parse_function_pointer(s)
    argst = parse_parameters(args)
    argst.pop(0)
    if argst and argst[-1][0] == "...":
        print("template <typename... Ts>")
    print(f"{type} {n}({", ".join([f"{"Ts... args" if type == "..." else type}{name}" for (type, name) in argst])})")
    print("{")
    if type == "void":
        print("    ", end="")
    else:
        print("    return ", end="")
    aa = ["this"]
    for a in argst:
        if a[0] == "...":
            aa.append("std::forward<Ts>(args)...")
        else:
            aa.append(a[1])
    print(f"internal->{n}({", ".join(aa)});")
    print("}")
    # print(type, n, argst)

s = ""
while True:
    line = input()
    s += line;
    s = s.replace("\n", "").replace("    ", "")
    if line.endswith(");"):
        process(s)
        s = ""
