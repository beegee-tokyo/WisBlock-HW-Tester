import datetime

Import("env")

my_flags = env.ParseFlags(env['BUILD_FLAGS'])
defines = {k: v for (k, v) in my_flags.get("CPPDEFINES")}

build_tag = "RAK_"

version_tag_1 = defines.get("SW_VERSION_1")
version_tag_2 = defines.get("SW_VERSION_2")
version_tag_3 = defines.get("SW_VERSION_3")
version_tag_4 = defines.get("FREQ_VERSION")
version_tag_5 = defines.get("CORE_VERSION")
build_date = datetime.datetime.now().strftime('%Y.%m.%d.%H.%M.%S')

env.Replace(PROGNAME="../../../Generated/WB_HW_Test_%s_%s_V%s.%s.%s" % (version_tag_5,version_tag_4,version_tag_1,version_tag_2,version_tag_3))
