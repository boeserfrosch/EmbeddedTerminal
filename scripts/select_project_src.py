import os
from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

custom_project_src = env.GetProjectOption("custom_project_src", "").strip()
if custom_project_src:
    project_dir = env.subst("$PROJECT_DIR")
    selected_src_dir = os.path.join(project_dir, custom_project_src.replace("/", os.sep))
    env.Replace(PROJECT_SRC_DIR=selected_src_dir)
    print("[dev-target] PROJECT_SRC_DIR set to: {}".format(selected_src_dir))
