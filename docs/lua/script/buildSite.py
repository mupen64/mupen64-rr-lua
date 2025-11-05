import json
import re
import os

import markdown
import minify_html
from markdown.extensions.codehilite import CodeHiliteExtension
from markdown.extensions.fenced_code import FencedCodeExtension
from dataclasses import dataclass


# RUN "pip install -r requirements.txt" BEFORE RUNNING THIS SCRIPT. This script
# assumes the working directory is the base git directory (not /script). It is
# recommended that you run this with VS Code instead of the command line

# Command to generate pygments.css (run inside the css folder):
# pygmentize -S pastie -f html -a .codehilite > pygments.css

# TODO:
# - use a proper templating engine instead of string accumulation


class StringAccumulator:
    def __init__(self):
        self.segments: list[str] = []

    def add(self, string: str | list[str]):
        if isinstance(string, list):
            self.segments.append(*string)
        else:
            self.segments.append(string)

    def retrieve(self, separator: str = "") -> str:
        return separator.join(self.segments)


@dataclass(unsafe_hash=True)
class LuaFunc:
    module: str
    name: str

    # (global, print) -> print
    # (wgui, drawtext) -> wgui.drawtext
    def display_name(self) -> str:
        return self.name if self.module == "global" else f"{self.module}.{self.name}"

    # (global, print) -> globalPrint
    # (wgui, drawtext) -> wguiDrawtext
    def internal_name(self) -> str:
        return f"{self.module}{self.name.capitalize()}"


def from_lua_signature(fn: str) -> LuaFunc:
    segments = fn.split(".")
    if len(segments) == 1:
        segments.insert(0, "global")
    return LuaFunc(*segments)


type LineNums = dict[int, LuaFunc]


def transform_links(text: str, line_nums: LineNums) -> str:
    markdown_link_pattern = re.compile(
        r"\[(?P<link_text>.+)\]\(.*mupen.*\.lua\#(?P<line_num>\d+)\)"
    )

    def replace(x: re.Match[str]):
        line_num = x.group("line_num")
        if not isinstance(line_num, str):
            print("line num wasn't a string")
            exit(1)
        link_text = x.group("link_text")
        if not isinstance(link_text, str):
            print("link text wasn't a string")
            exit(1)
        fn = line_nums[int(line_num)]
        return f"[{link_text}](#{fn.internal_name()})"

    return re.sub(markdown_link_pattern, replace, text)


@dataclass
class LuaFuncData:
    description: str
    view: str | None
    dep_message: str | None
    example: str | None

    def transform_all_links(self, line_nums: LineNums):
        self.description = transform_links(self.description, line_nums)
        if self.dep_message is not None:
            self.dep_message = transform_links(self.dep_message, line_nums)


def parse_markdown(str: str) -> str:
    return markdown.markdown(
        str, extensions=[CodeHiliteExtension(), FencedCodeExtension()]
    )


type CppFuncs = dict[str, list[LuaFunc]]


def read_funcs_from_cpp_file(path: str) -> CppFuncs:
    module_pattern = re.compile(
        r"const luaL_Reg (?P<module>[A-Za-z0-9]+)_FUNCS\[\] = \{"
    )
    name_pattern = re.compile(r'\{"(?P<name>\w+)",[^\},]*}')

    func_list_dict: CppFuncs = {}

    try:
        with open(path, "rt", encoding="utf-8") as file:
            data = file.read()

            start = data.find("// begin lua funcs")
            end = data.find("// end lua funcs")

            if start == -1 or end == -1:
                raise Exception("Couldn't find begin or end comment")

            for section in data[start:end].split(";"):
                stripped = section.strip()
                if stripped == "":
                    continue

                matched = module_pattern.search(stripped)
                if matched is None:
                    raise Exception(f"Couldn't find module name in section {section}")

                module = matched.group("module").lower()

                func_list_dict[module] = [
                    LuaFunc(module, name_match.group("name"))
                    for name_match in name_pattern.finditer(section)
                ]

        return func_list_dict
    except FileNotFoundError as e:
        e.add_note(
            f"Have you initialized the mupen submodule and are you running the script from the root directory?"
        )
        raise


type DepMessages = dict[LuaFunc, str]


def read_api_file(path: str) -> DepMessages:
    try:
        with open(path, "rt", encoding="utf-8") as f:
            lines = [line.strip() for line in f]
    except FileNotFoundError as e:
        e.add_note(
            f"Have you initialized the mupen submodule and are you running the script from the root directory?"
        )
        raise

    dep_messages: DepMessages = {}

    dep_message = None

    for l in lines:
        if l.startswith(f"---@deprecated "):
            dep_message = l.removeprefix("---@deprecated ").strip()

        if l.startswith("function "):
            end = l.find("(")
            fn = from_lua_signature(l[9:end].strip())

            if dep_message is not None:
                dep_messages[fn] = dep_message
                dep_message = None

    return dep_messages


def load_example(fn: LuaFunc) -> str | None:
    example_filename = f"examples/{fn.module}/{fn.name}.lua"
    try:
        with open(example_filename, "rt", encoding="utf-8") as f:
            return f.read()
    except FileNotFoundError:
        # print(f"couldn't find example file {example_filename}")
        return None


type JsonFuncs = dict[LuaFunc, list[LuaFuncData]]


def read_funcs_from_json_file(
    path: str, api_filepath: str, included_aliases: list[str]
) -> JsonFuncs:
    deprecation_messages = read_api_file(api_filepath)
    api_filename = api_filepath.split("/")[-1]

    line_nums: LineNums = {}
    functions: JsonFuncs = {}

    try:
        with open(path, "rt", encoding="utf-8") as f:
            data = json.loads(f.read())
    except FileNotFoundError as e:
        e.add_note("Have you exported the documentation to the correct place?")
        raise

    # data is an array with all the variables
    for variable in data:
        # only accept files exported from the api file
        if "DOC" in variable:
            assert variable["DOC"].endswith(api_filename)
            continue

        # store the name of the variable
        variable_name = variable["name"]

        # each variable can have multiple definitions (print has 2, one is for mupen and one is the regular lua one)
        for definition in variable["defines"]:
            line_num = definition["start"][0] + 1

            if definition["type"] == "doc.alias" and variable_name in included_aliases:
                fn = LuaFunc("global", variable_name)
                fn_data = LuaFuncData(definition["desc"], None, None, None)

                if not fn in functions:
                    functions[fn] = []
                functions[fn].append(fn_data)

                line_nums[line_num] = fn
                continue

            if not "file" in definition:
                continue
            file_name = definition["file"]

            # only process the definitions from this file
            if file_name != ".":
                continue

            if not "extends" in definition:
                continue

            extends = definition["extends"]

            if extends["type"] != "function":
                continue

            fn = from_lua_signature(variable_name)

            line_nums[line_num] = fn

            description = extends["desc"]
            view = extends["view"]

            deprecated_message = None
            if definition["deprecated"]:
                deprecated_message = deprecation_messages[fn]

            example = load_example(fn)

            if not fn in functions:
                functions[fn] = []

            func_data = LuaFuncData(description, view, deprecated_message, example)
            functions[fn].append(func_data)

    # we have to do this after we've seen all the line numbers
    for funcs_data in functions.values():
        for func_data in funcs_data:
            func_data.transform_all_links(line_nums)

    return functions


def generate_example_markdown(example: str | None):
    if example is None:
        return ""

    return f"""
#### Example:
```lua
{example}
```
"""


def generate_deprecated_markdown(deprecated_message: str | None):
    if deprecated_message is None:
        return ""

    return f'<span style="background-color: red; padding: 4px">This function is deprecated. {deprecated_message}</span>'


def generate_view_markdown(view: str | None):
    if view is None:
        return ""

    return f"""```luau
{view}
```"""


def generate_function_html(fn: LuaFunc, fn_data: LuaFuncData) -> str:
    example_markdown = generate_example_markdown(fn_data.example)
    deprecated_markdown = generate_deprecated_markdown(fn_data.dep_message)
    view = generate_view_markdown(fn_data.view)

    return parse_markdown(
        f"""
---

# {f'<a id="{fn.internal_name()}">'}{fn.display_name()}</a>

{deprecated_markdown}

{fn_data.description}


{view}

{example_markdown}

"""
    )


def add_header(html: StringAccumulator):
    html.add(
        """
    <!DOCTYPE html>
    <html lang="en">
        <head>
            <meta charset="UTF-8" />
            <meta http-equiv="X-UA-Compatible" content="IE=edge" />
            <meta name="viewport" content="width=device-width, initial-scale=1.0" />
            <link href="css/styles.css" type="text/css" rel="stylesheet">
            <link href="css/pygments.css" type="text/css" rel="stylesheet">
            <title>Mupen Lua API Docs</title>
        </head>
        <body>
            <div class="sidebar">
            <div class="sidebarheader">
                <div class="logo">
                    <a href="index.html"><img src="img/mupen_logo.png"></a>
                </div>
                <div class="logolabel">
                    mupen64-rr-lua docs
                </div>
            </div>
    """
    )


def add_sidebar(
    html: StringAccumulator, cpp_functions: CppFuncs, skipped_functions: list[LuaFunc]
):
    # loop over function types (emu, wgui)
    for module, funcs in cpp_functions.items():
        html.add(
            f"""
            <div class="collapsible">
                <a href="#{module}Funcs">{module.upper()} FUNCTIONS</a>
            </div>
            """
        )
        html.add('<div class="funcList">')
        for fn in funcs:
            if fn in skipped_functions:
                continue

            internal_name = fn.internal_name()
            display_name = fn.display_name().upper()

            html.add(
                f"""
                <a href="#{internal_name}">
                    <button class="funcListItem" onclick="highlightFunc('{internal_name}')">{display_name}</button>
                </a>
                """
            )
        html.add("</div>")  # closes div.funcList
    html.add("</div>")  # closes div.sidebar


def add_function(html: StringAccumulator, fn: LuaFunc, fn_data: LuaFuncData):
    html.add(f'<div name="{fn.internal_name()}">')
    html.add(generate_function_html(fn, fn_data))
    html.add("</div>")


def add_body(
    html: StringAccumulator,
    cpp_functions: CppFuncs,
    lua_functions: JsonFuncs,
    skipped_functions: list[LuaFunc],
    included_aliases: list[str],
):
    used_lua_functions: list[LuaFunc] = []

    html.add('<div class="docBody">')
    for module, funcs in cpp_functions.items():
        # create the section header

        html.add(
            parse_markdown(
                f'---\n# <a id="{module}Funcs">{module.upper()}</a> FUNCTIONS'
            )
        )

        for fn in funcs:
            if fn in skipped_functions:
                continue

            if fn not in lua_functions:
                print(f"c++ function {fn.display_name()} wasn't in lua functions")
                fn_data = LuaFuncData("?", "?", None, None)

                add_function(html, fn, fn_data)
                continue

            for fn_data in lua_functions[fn]:
                add_function(html, fn, fn_data)

            used_lua_functions.append(fn)

    if len(included_aliases) > 0:
        html.add(parse_markdown("---\n# OTHER INFORMATION"))

    # add aliases
    for alias in included_aliases:
        fn = LuaFunc("global", alias)
        for fn_data in lua_functions[fn]:
            add_function(html, fn, fn_data)

    html.add("</div>")  # closed div.docBody

    # make sure every lua function was used
    for key in lua_functions.keys():
        if not key in used_lua_functions:
            print(f"lua function wasn't used: {key}")


def add_javascript(html: StringAccumulator):
    html.add(f"<script src='js/index.js'></script>")


def add_footer(html: StringAccumulator):
    html.add("</body></html>")


def write_output(data: str):
    with open("docs/index.html", "w", encoding="utf-8") as file:
        file.write(
            minify_html.minify(
                data,
                keep_html_and_head_opening_tags=True,
                minify_css=True,
                keep_closing_tags=True,
                remove_processing_instructions=True,
            )
        )

    with open("docs/index-no-min.html", "w", encoding="utf-8") as file:
        file.write(data)


def ensure_working_dir():
    cwd = os.getcwd()
    if cwd.endswith("script"):
        os.chdir("../")


def main():
    # Config
    api_filepath = "mupen64-rr-lua/src/api.lua"
    cpp_filepath = "mupen64-rr-lua/src/Views.Win32/lua/LuaRegistry.cpp"
    docs_filepath = "export/doc.json"
    skipped_functions: list[LuaFunc] = []
    included_aliases: list[str] = []

    ensure_working_dir()

    cpp_functions = read_funcs_from_cpp_file(cpp_filepath)
    lua_functions = read_funcs_from_json_file(
        docs_filepath, api_filepath, included_aliases
    )

    cpp_functions["os"] = [LuaFunc("os", "exit")]

    html = StringAccumulator()

    add_header(html)
    add_sidebar(html, cpp_functions, skipped_functions)
    add_body(html, cpp_functions, lua_functions, skipped_functions, included_aliases)
    add_javascript(html)
    add_footer(html)
    write_output(html.retrieve())


if __name__ == "__main__":
    main()
