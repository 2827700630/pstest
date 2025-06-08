# 2025-06-08T10:15:22.909184300
import vitis

client = vitis.create_client()
client.set_workspace(path="pstest")

advanced_options = client.create_advanced_options_dict(dt_overlay="0")

platform = client.create_platform_component(name = "platform",hw_design = "$COMPONENT_LOCATION/../design_1_wrapper.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0",generate_dtb = False,advanced_options = advanced_options,compiler = "gcc")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="xuartps_hello_world_example")
comp.build()

status = platform.build()

comp.build()

vitis.dispose()

