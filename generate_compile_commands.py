Import("env")
import shutil
import os

# This runs after the project environment is configured
def after_build(source, target, env):
    print("Generating compile_commands.json...")
    env.Execute("pio run -t compiledb")
    
    # Copy to project root
    build_dir = env.subst("$BUILD_DIR")
    src = os.path.join(build_dir, "compile_commands.json")
    dst = os.path.join(env.subst("$PROJECT_DIR"), "compile_commands.json")
    
    if os.path.exists(src):
        shutil.copy(src, dst)
        print(f"✓ Copied compile_commands.json to project root")
    else:
        print(f"✗ compile_commands.json not found at {src}")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", after_build)
