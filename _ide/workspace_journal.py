# 2025-06-08T12:49:41.257813
import vitis

client = vitis.create_client()
client.set_workspace(path="pstest")

client.delete_component(name="xuartps_hello_world_example")

client.delete_component(name="componentName")

platform = client.get_component(name="platform")
status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../design_1_wrapper.xsa")

status = platform.build()

comp = client.get_component(name="xgpiops_polled_example")
comp.build()

status = platform.build()

comp.build()

