# 2025-06-08T16:46:00.621980300
import vitis

client = vitis.create_client()
client.set_workspace(path="pstest")

platform = client.get_component(name="platform")
status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../design_1_wrapper.xsa")

status = platform.build()

vitis.dispose()

