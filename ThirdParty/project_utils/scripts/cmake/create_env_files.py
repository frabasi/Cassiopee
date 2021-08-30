import sys
import os
import ast
import configparser

# retrieve information from file generated by cmake
cmake_vars_file = sys.argv[1]
cmake_vars_file_dir = os.path.dirname(cmake_vars_file)

with open(cmake_vars_file, 'r') as f:
  cmake_vars = ast.literal_eval(f.read())

# extract cmake variables
project_name = cmake_vars["PROJECT_NAME"]
install_path = cmake_vars["CMAKE_INSTALL_PREFIX"]
dependency_string = cmake_vars["DEPENDENCIES_STRING"]
thirdparty_dependency_string = cmake_vars["THIRDPARTY_DEPENDENCIES_STRING"]
cxx_release = cmake_vars["CMAKE_CXX_FLAGS_RELEASE"]
defs = cmake_vars["COMPILE_DEFS"]

# additional parsing
defs_list = defs.strip().split(",")
defs = {}
if defs_list != ['']:
  for definition in defs_list:
    key,val = definition.split("=")
    defs[key] = val


deps = []
for dep in dependency_string+thirdparty_dependency_string:
  dep_name = dep.split(" ")[0]
  deps += [dep_name]


# source
source_string = ""
source_string += "export LD_LIBRARY_PATH="+install_path+"/lib:$LD_LIBRARY_PATH\n"
source_string += "export PYTHONPATH="+install_path+"/lib:$PYTHONPATH\n"
source_string += "export PYTHONPATH="+install_path+"/py:$PYTHONPATH\n"

with open(cmake_vars_file_dir+"/source_"+project_name+".sh", 'w') as f:
  f.write(source_string)

# ini file
config = configparser.ConfigParser()
config[project_name] = {
 "libs" : [project_name],
 "uses" : deps,
 "cxx_release" : [cxx_release],
 "defs" : defs,
 "paths"    : [install_path],
 "binpaths" : [install_path+"/bin"],
 "incpaths" : [install_path+"/include"],
 "libpaths" : [install_path+"/lib"],
 "pypaths"  : [install_path+"/py"],
}

with open(cmake_vars_file_dir+"/config_"+project_name+".ini", 'w') as f:
  config.write(f)
