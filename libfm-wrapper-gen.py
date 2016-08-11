#!/usr/bin/env python3
import sys
import subprocess
import os
import re
from collections import deque
import copy


license_text = """/*
 * Copyright (C) 2016 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
"""

copy_ctor_templ = """
  {CPP_CLASS_NAME}({CPP_CLASS_NAME}& other) {{
    if(dataPtr_ != nullptr) {{
      {FREE_FUNC}(dataPtr_);
    }}
    dataPtr_ = other.dataPtr_ != nullptr ? static_cast<{C_STRUCT_NAME}*>({COPY_FUNC}(other.dataPtr_)) : nullptr;
  }}
"""

default_ctor_templ = """
  {CPP_CLASS_NAME}({C_STRUCT_NAME}* dataPtr) {{
    if(dataPtr_ != nullptr) {{
      {FREE_FUNC}(dataPtr_);
    }}
    dataPtr_ = dataPtr != nullptr ? static_cast<{C_STRUCT_NAME}*>({COPY_FUNC}(dataPtr)) : nullptr;
  }}
"""

class_templ = """
class {CPP_CLASS_NAME}{INHERIT} {{
public:

  // default constructor
  {CPP_CLASS_NAME}(): dataPtr_(nullptr) {{
  }}

{CTORS}

  // destructor
  ~{CPP_CLASS_NAME}() {{
    if(dataPtr_ != nullptr) {{
      {FREE_FUNC}(dataPtr_);
    }}
  }}

  // create a wrapper for the data pointer without increasing the reference count
  static {CPP_CLASS_NAME}* wrap({C_STRUCT_NAME}* dataPtr) {{
    {CPP_CLASS_NAME}* obj = new {CPP_CLASS_NAME}();
    obj->dataPtr_ = dataPtr;
    return obj;
  }}

  // disown the managed data pointer
  {C_STRUCT_NAME}* takeDataPtr() {{
    {C_STRUCT_NAME}* data = dataPtr_;
    dataPtr_ = nullptr;
    return data;
  }}

  // get the raw pointer wrapped
  {C_STRUCT_NAME}* dataPtr() {{
    return dataPtr_;
  }}

  // automatic type casting
  operator {C_STRUCT_NAME}*() {{
    return dataPtr_;
  }}

    // methods
{METHODS}
{EXTRA_CODE}
private:
  {C_STRUCT_NAME}* dataPtr_; // data pointer for the underlying C struct
}};
"""

method_templ = """
  {METHOD_DECL} {{
    {FUNC_BODY};
  }}
"""

header_templ = """{LICENSE}
#ifndef {HEADER_GUARD}
#define {HEADER_GUARD}

#include <libfm/fm.h>
#include <QObject>
#include <QtGlobals>

namespace Fm {{

{CLASSES}

}}

#endif // {HEADER_GUARD}
"""


def camel_case_to_lower(identifier):
    part_begin = 0
    parts = []
    for i in range(1, len(identifier) - 1):
        if identifier[i].islower() and identifier[i + 1].isupper():
            part = identifier[part_begin:i + 1].lower()
            part_begin = i + 1
            parts.append(part)
    if part_begin < len(identifier):
        part = identifier[part_begin:].lower()
        parts.append(part)
    return "_".join(parts)


def lower_case_to_camel(identifier, capitalize_first=False):
    parts = [part.capitalize() for part in identifier.split("_")]
    if not capitalize_first:
        parts[0] = parts[0].lower()
    return "".join(parts)


class Variable:
    def __init__(self, type_name="", name=""):
        self.type_name = type_name
        self.name = name

    def from_string(self, decl):
        decl = decl.strip()
        # split type name and variable name at the last space
        parts = decl.rsplit(maxsplit=1)
        if parts:
            self.type_name = parts[0].strip()
            if len(parts) > 1:
                self.name = parts[1]

            # fix pointer type
            if self.name:
                stars = 0
                while self.name[stars] == "*":
                    stars += 1
                # attach * to type names
                if stars:
                    self.type_name += self.name[0:stars]
                    self.name = self.name[stars:]
            # print("DECL", self.type_name, " -> ", self.name)

    def to_string(self):
        return "{0} {1}".format(self.type_name, self.name)


glib_to_cpp_type = {
    "gboolean": "bool",
    "gint": "int",
    "guint": "unsigned int"
}


class Method:
    regex_pattern = re.compile(r'''
        ^(\w+)      # return type
        ([\s\*]+)   # space or *
        (\w+)       # function name
        \s*         # space
        \((.*)\);?  # (arg1, arg2, ...);
        ''', re.MULTILINE|re.ASCII|re.VERBOSE)

    def __init__(self, regex_match=None):
        self.is_static = False
        self.is_ctor = False
        self.return_type = "void"
        self.name = ""
        self.args = []  # list of Variable
        if regex_match:
            self.return_type = regex_match[0]
            stars = regex_match[1].strip()
            if stars:
                self.return_type += stars
            self.name = regex_match[2]
            # parse the declaration
            args = regex_match[3]
            for arg_decl in args.split(","):
                var = Variable()
                var.from_string(arg_decl)
                self.args.append(var)

    def is_getter(self):
        return True if "_get" in self.name else False

    def is_const_return(self):
        return True if "const" in self.return_type else False

    def has_str_return(self):
        return "char*" in self.return_type

    def has_str_args(self):
        for arg in self.args:
            if "char*" in arg.type_name:
                return True
        return False

    def generate_qstring_helpers(self):
        helpers = []
        if self.has_str_args():
            helper = copy.deepcopy(self)
            for arg in helper.args:
                if "char*" in arg.type_name:
                    arg.type_name = "const QString&"
            helpers.append(helper)

        if self.has_str_return():
            for helper in helpers:
                helper2 = copy.deepcopy(helper)
                helper2.name += "_QString"
                helpers.append(helper2)
        return helpers

    def to_string(self, skip_prefix, camel_case=True, skip_this_ptr=True, name=None, ret_type=None):
        if not name:
            name = self.name[skip_prefix:]
            if camel_case:
                name = lower_case_to_camel(name)

        if not ret_type:
            ret_type = glib_to_cpp_type.get(self.return_type, self.return_type)
            if self.is_ctor:  # constructor has no return type
                ret_type = ""

        args = self.args
        if skip_this_ptr and not self.is_static:
            args = self.args[1:]  # strip this pointer from arguments

        if args:
            args = ", ".join([arg.to_string() for arg in args])
        else:
            args = "void"

        method_decl = "{ret}{name}({args})".format(ret=ret_type + " " if ret_type else "",
                                                   name=name,
                                                   args=args)
        if self.is_static and not self.is_ctor:
            method_decl = "static " + method_decl
        return method_decl

    def invoke(self, this_ptr=None):
        arg_names = []
        for arg in self.args:
            if arg.type_name == "const QString&":
                arg_names.append("{ARG}.toUtf8().constData()")
            else:
                arg_names.append(arg.name)

        arg_names = [a.name for a in self.args if a.name]
        if this_ptr and not self.is_static:
            arg_names = [this_ptr] + arg_names[1:]  # skip this pointer
        invoke = "{func}({args})".format(func=self.name,
                                        args=", ".join(arg_names) if arg_names else "")
        return invoke


custom_ctor_names = [
    "fm_folder_config_open"
]

custom_free_func_names = [
    "fm_folder_config_close"
]


class Class:
    regex_pattern = re.compile(r'typedef\s+struct\s+(\w+)\s+(\w+)', re.ASCII)

    def __init__(self, regex_match=None):
        if regex_match:
            self.name = regex_match[1]
            self.method_name_prefix = camel_case_to_lower(self.name) + "_"
        else:
            self.name = ""
            self.method_name_prefix = ""
        self.method_name_prefix_len = len(self.method_name_prefix)
        self.is_gobject = False
        self.is_ref_counted = False  # has reference counting
        self.methods = []  # list of Method
        # self.data_members = []  # list of Variable
        self.signals = []  # list of Method
        self.ctors = []
        self.copy_func = None
        self.free_func = None
        self.cpp_class_name = self.name[2:]  # skip Fm prefix
        self.ptr_type = self.name + "*"

    def add_method(self, method):
        print(self.name, method.name, method.return_type)
        if method.return_type == "GType":
            # avoid adding _get_type()
            self.is_gobject = True  # this struct is a GObject class
            self.is_ref_counted = True
            return
        this_type = self.name + "*"
        if not method.args or method.args[0].type_name != this_type:
            method.is_static = True

        if "_new" in method.name or method.name in custom_ctor_names and method.is_static:
            # this is a constructor
            method.is_ctor = True
            self.ctors.append(method)
        elif method.name.endswith("_ref"):  # copy method
            self.copy_func = method
            self.is_ref_counted = True
        elif method.name.endswith("_unref"):  # free method
            self.free_func = method
        elif method.name in custom_free_func_names:
            self.free_func = method
        else:  # normal method
            self.methods.append(method)

    def generate_method_def(self, method):
        # ordinary methods
        invoke = method.invoke("dataPtr_")
        ret_type = None
        if method.return_type != "void":
            if method.return_type == self.ptr_type:  # returns Class*
                # wrap in our C++ wrapper
                ret_type = self.cpp_class_name
                if method.is_getter():  # do not take ownership for getters
                    invoke = "{CPP_CLASS}({DATA})".format(CPP_CLASS=self.cpp_class_name, DATA=invoke)
                else:  # take ownership
                    invoke = "{CPP_CLASS}::wrap({DATA})".format(CPP_CLASS=self.cpp_class_name, DATA=invoke)
            elif method.return_type == "QString":  # QString wrapper
                invoke = "QString::fromUtf8({DATA})".format(DATA=invoke)
            invoke = "return " + invoke
        method_def = method_templ.format(
            METHOD_DECL=method.to_string(skip_prefix=self.method_name_prefix_len, camel_case=True, ret_type=ret_type),
            FUNC_BODY=invoke
        )
        return method_def

    def to_string(self):
        # ordinary methods
        method_defs = []
        for method in self.methods:
            method_defs.append(self.generate_method_def(method))
            """
            if method.has_str_args() or method.has_str_return():
                helpers = method.generate_qstring_helpers()
                for helper in helpers:
                    method_defs.append(self.generate_method_def(helper)
            """

        # constructors
        ctors = []
        for ctor in self.ctors:
            ctor_def = method_templ.format(
                METHOD_DECL=ctor.to_string(skip_prefix=self.method_name_prefix_len, name=self.name),
                FUNC_BODY="dataPtr_ = " + ctor.invoke("dataPtr_")
            )
            ctors.append(ctor_def)

        inherit = extra_code = ""
        # special handling for GObjects
        if self.is_gobject:
            # FIXME: should we add code for signal handling for GObjects?
            inherit = ""  # ": public QObject"
            copy_func = "g_object_ref"
            free_func = "g_object_unref"
            '''
            extra_code = gobject_templ.format(
                SIGNALS=""
            )
            '''
        else:
            copy_func = self.copy_func.name if self.copy_func else ""
            free_func = self.free_func.name if self.free_func else ""

        # FIXME: if copy_func and free_func are empty, we should disable copy constructors
        # FIXME: if no constructors are found, we should make default ctor private
        # TODO: implement move constructors
        #       correct inheritence for GObject derived classses?

        # output the C++ class
        return class_templ.format(
            CPP_CLASS_NAME=self.cpp_class_name,
            INHERIT=inherit,
            CTORS="\n".join(ctors) if ctors else "",
            COPY_FUNC=copy_func,
            FREE_FUNC=free_func,
            METHODS="\n".join(method_defs),
            C_STRUCT_NAME=self.name,
            EXTRA_CODE=extra_code
        )


def generate_cpp_wrapper(c_header_file, file_base_name):
    print(c_header_file)
    try:
        with open(c_header_file, "r") as f:
            c_source_code = f.read()
            define_pattern = re.compile(r'#define\s+(\w+)', re.ASCII)
            # for m in define_pattern.findall(c_source_code):
            #     print("define", m)

            # find all struct names
            structs = []
            for m in Class.regex_pattern.findall(c_source_code):
                # print("struct", m)
                struct = Class(m)
                structs.append(struct)

            if not structs:  # no object class found in this header
                return ""

            # find all function names
            methods = deque()
            for m in Method.regex_pattern.findall(c_source_code):
                method = Method(m)
                methods.append(method)

            # sort struct by length of their names in descending order
            structs.sort(key=lambda struct: len(struct.name), reverse=True)

            # add methods to structs
            while methods:
                method = methods.pop()
                for struct in structs:
                    if method.name.startswith(struct.method_name_prefix):
                        struct.add_method(method)
                        break

            classes = []
            for struct in structs:
                # only generate wrapper for classes which have methods
                if struct.methods:
                    classes.append(struct.to_string())

            # output
            header_guard = "__LIBFM_QT_{0}__".format(file_base_name.replace("-", "_").replace(".", "_").upper())
            cpp_source_code = header_templ.format(LICENSE=license_text, CLASSES="\n\n".join(classes), HEADER_GUARD=header_guard)
    except IOError:
        cpp_source_code = ""
    return cpp_source_code


def main(argv):
    if len(argv) < 3:
        print("Usage:\nlibfm-wrapper-gen.py <libfm src dir> <output dir>")
        return

    excluded_headers = [
        "fm-module.h",
        "fm-marshal.h"
    ]

    headers = []
    libfm_src_tree = sys.argv[1]
    for subdir in ("src/base", "src/job"):
        dirpath = os.path.join(libfm_src_tree, subdir)
        for filename in os.listdir(dirpath):
            if filename.endswith(".h"):
                if filename not in excluded_headers:
                    headers.append(os.path.join(dirpath, filename))

    output_dir = sys.argv[2]
    for header in headers:
        file_base_name = os.path.basename(header)
        output_filename = os.path.join(output_dir, file_base_name[3:].replace("-", ""))  # skip fm- and remove all '-'
        with open(output_filename, "w") as output_file:
            cpp_source_code = generate_cpp_wrapper(header, file_base_name)
            output_file.write(cpp_source_code)

if __name__ == "__main__":
    main(sys.argv)
