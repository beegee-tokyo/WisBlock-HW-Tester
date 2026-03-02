import os
import sys
import struct

Import("env")

def copy_esp32(source, target, env):
    print("#########################################################")
    print("Copy ESP32 bootloader and partition table")
    print("#########################################################")
    
    new_file = "./Generated/WB_HW_Test_%s_%s_Bootloader_V%s.%s.%s.bin" % (version_tag_5,version_tag_4,version_tag_1,version_tag_2,version_tag_3)
    source_file = "./.pio/build/rak3312/bootloader.bin"
    # print('Dest: ' +new_file)
    # print('Src:  ' +source_file)

    if os.path.isfile(new_file):
        try:
            os.remove(new_file)
        except:
            print('Cannot delete '+new_file)

    try:
        os.rename(source_file, new_file)
        print(f"Created '{new_file}'")
    except OSError as e:
        print(f"An error occurred: {e}")

    new_file = "./Generated/WB_HW_Test_%s_%s_Partitions_V%s.%s.%s.bin" % (version_tag_5,version_tag_4,version_tag_1,version_tag_2,version_tag_3)
    source_file = "./.pio/build/rak3312/partitions.bin"
    # print('Dest: ' +new_file)
    # print('Src:  ' +source_file)
    
    if os.path.isfile(new_file):
        try:
            os.remove(new_file)
        except:
            print('Cannot delete '+new_file)

    try:
        os.rename(source_file, new_file)
        print(f"Created '{new_file}'")
    except OSError as e:
        print(f"An error occurred: {e}")

my_flags = env.ParseFlags(env['BUILD_FLAGS'])
defines = {k: v for (k, v) in my_flags.get("CPPDEFINES")}

version_tag_1 = defines.get("SW_VERSION_1")
version_tag_2 = defines.get("SW_VERSION_2")
version_tag_3 = defines.get("SW_VERSION_3")
version_tag_4 = defines.get("FREQ_VERSION")
version_tag_5 = defines.get("CORE_VERSION")

if version_tag_5 == "RAK3312":
    # Add callback after .hex file was created
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_esp32)
    # print("Prepare for RAK3312")
