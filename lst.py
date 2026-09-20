Import("env")
import os

# Function to generate the .lst file
def generate_lst(source, target, env):
    # Dynamically find the path to avr-objdump
    objcopy = env.subst("$OBJCOPY")
    objdump = objcopy.replace("avr-objcopy", "avr-objdump").replace("objcopy", "objdump")
    
    elf_file = str(target[0])
    lst_file = elf_file.replace(".elf", ".lst")

    # Run objdump: -d (disassemble), -S (source intermixed), -C (demangle)
    # For ATtiny, -m avr is usually implied, but you can add it if needed
    cmd = f'"{objdump}" -d -S -C "{elf_file}" > "{lst_file}"'
    
    print(f"Generating assembly listing: {lst_file}")
    return env.Execute(cmd)

# Attach the action to run after the .elf is built
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", generate_lst)
