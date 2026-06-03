Import("env")  # must be first — SCons injects 'env' via this call
import os

project_dir = env.subst("$PROJECT_DIR")
src_dir     = os.path.join(project_dir, "src")
shim_path = os.path.join(src_dir, "main.cpp")
os.remove(shim_path)