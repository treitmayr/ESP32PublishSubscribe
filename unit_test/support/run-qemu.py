Import("env")

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"

def run_qemu(source, target, env):
    MERGED_BIN = env.subst("$BUILD_DIR/${PROGNAME}_merged.bin")
    cmd = ['qemu-system-xtensa', '-nographic', '-machine', 'esp32',
           '-drive', 'file=' + MERGED_BIN +',if=mtd,format=raw', '-no-reboot',
           '-serial', 'stdio', '-monitor', '/dev/null',
           '-nic', 'user,model=open_eth'
        ]

    last_line = ''
    import subprocess
    with subprocess.Popen(cmd, stdout=subprocess.PIPE, text=True, bufsize=1) as proc:
        for line in proc.stdout:
            line = line.rstrip()
            print(line)
            last_line = line
        proc.stdout.close()

    # communicate the overall test result to the outside
    return not (last_line == "OK")

# Add a post action that runs esptoolpy to merge available flash images
env.AddPostAction(APP_BIN, run_qemu)
