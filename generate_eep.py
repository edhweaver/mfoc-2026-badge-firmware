Import("env")
import os

def env_output_eeprom(target, source, env):
    # Retrieve paths to the compiled .elf and target .eep files
    elf_file = os.path.join(env.subst("$BUILD_DIR"), "${PROGNAME}.elf")
    eep_file = os.path.join(env.subst("$BUILD_DIR"), "${PROGNAME}.eep")

    # Execute the GNU avr-objcopy command to isolate the EEPROM data section
    print(f"--> Extracting EEPROM data section to: {eep_file}")
    env.Execute(f'"$OBJCOPY" -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 "{elf_file}" "{eep_file}"')

# Append this task immediately after the primary .elf executable is completely linked
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", env_output_eeprom)
