import os
Import("env")
proj_name = '"' + os.path.basename(os.path.normpath(env["PROJECT_DIR"])) + '"'
env.Append(CPPDEFINES=[
  ("PROJECT_NAME", "\\\"" + proj_name + "\\\"")
])