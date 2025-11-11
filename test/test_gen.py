import sys
import os
import argparse
import jinja2

import pycparser
from pycparser import c_ast, c_generator, parse_file
import pycparser_fake_libc

fake_libc_arg = "-I" + pycparser_fake_libc.directory

class FuncDefVisitor(c_ast.NodeVisitor):
    def __init__(self):
        self.generator = c_generator.CGenerator()  # generate C code from AST
        self.setup = None
        self.test_protos = []
        self.test_suite = []

    def visit_FuncDef(self, node):
        if node.decl.name.startswith("test_"):
            proto = self.generator.visit(node.decl) + ';'
            self.test_protos.append(proto)
            self.test_suite.append(node.decl.name)

        if node.decl.name.startswith("setup_"):
            proto = self.generator.visit(node.decl) + ';'
            self.test_protos.append(proto)
            self.setup = node.decl.name


    def finalize(self, test_suite_name):
        # Generate test array entries
        test_suite_entries = [
            f"    {{{self.setup}, {name}, \"{name}\"}},\n" for name in self.test_suite
        ]
        test_suite_arr = "".join(test_suite_entries)
        test_suite_arr = (
            f"static const test_t {test_suite_name}_arr[] = {{\n"
            f"{test_suite_arr}"
            f"}};\n"
        )

        # Generate test suite struct entry
        test_suite_struct = (
            f'{{ "{test_suite_name}", {test_suite_name}_arr, {len(self.test_suite)} }}'
        )

        # Join prototypes
        test_protos_str = "\n".join(self.test_protos)

        return test_protos_str, test_suite_arr, test_suite_struct


def print_func_defs(file, cpp_args):
    cpp_args.append(fake_libc_arg)

    cpp_args.append('-E')

    ast = parse_file(file, use_cpp=True, cpp_path="gcc", cpp_args=cpp_args)
    v = FuncDefVisitor()
    v.visit(ast)
    test_name = os.path.basename(file).replace("test_", "").replace(".c", "")
    test_protos, test_suite_arr, test_suite_struct= v.finalize(test_name)

    return test_protos, test_suite_arr, test_suite_struct

if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--cflags",
        nargs=1,
        help="CFLAGS to pass to c preprocessor"
    )

    parser.add_argument(
        "--test_sources",
        nargs='+',
        help="Tests to execute"
    )
 
    args = parser.parse_args()
    
    # Build required replacements
    all_test_protos = ""
    all_test_suite_arrs = ""
    all_test_suite_structs = ""
    num_test_suites = len(args.test_sources)

    for test in args.test_sources:
        test_protos, test_suite_arr, test_suite_struct = print_func_defs(test, args.cflags[0].split(' '))
        
        all_test_protos += test_protos
        all_test_suite_arrs += test_suite_arr + "\n"
        all_test_suite_structs += test_suite_struct + ", "

    # Insert using jinja2
    jinja2.TemplateLoader = jinja2.FileSystemLoader(os.path.abspath("./test"))
    env = jinja2.Environment(loader = jinja2.TemplateLoader)
    template = env.get_template("test_main.c.j2")
    print(template.render(all_test_protos=all_test_protos,
                          all_test_suite_arrs=all_test_suite_arrs, 
                          all_test_suite_structs=all_test_suite_structs,
                          num_test_suites=num_test_suites))


