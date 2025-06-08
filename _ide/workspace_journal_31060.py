# 2025-06-08T10:41:09.745446800
import vitis

client = vitis.create_client()
client.set_workspace(path="pstest")

platform = client.get_component(name="platform")
status = platform.build()

client.delete_component(name="xuartps_hello_world_example")

client.delete_component(name="componentName")

status = platform.build()

comp = client.get_component(name="xuartps_hello_world_example")
comp.build()

status = platform.build()

comp = client.get_component(name="xgpiops_polled_example")
comp.build()

status = platform.build()

comp.build()

status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../design_1_wrapper.xsa")

status = platform.build()

status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../design_1_wrapper.xsa")

status = platform.build()

comp.build()

status = platform.build()

comp.build()

client.delete_component(name="xgpiops_polled_example")

client.delete_component(name="componentName")

status = platform.build()

comp = client.get_component(name="xuartps_hello_world_example")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.update_hw(hw_design = "$COMPONENT_LOCATION/../design_1_wrapper.xsa")

status = platform.build()

status = platform.build()

comp.build()

client.delete_component(name="xuartps_hello_world_example")

comp = client.create_app_component(name="app_component",platform = "$COMPONENT_LOCATION/../platform/export/platform/platform.xpfm",domain = "standalone_ps7_cortexa9_0")

status = platform.build()

comp = client.get_component(name="app_component")
comp.build()

status = platform.build()

comp.build()

client.delete_component(name="app_component")

client.delete_component(name="componentName")

status = platform.build()

domain = platform.get_domain(name="standalone_ps7_cortexa9_0")

status = domain.set_config(option = "os", param = "standalone_stdin", value = "ps7_uart_1")

status = domain.set_config(option = "os", param = "standalone_stdout", value = "ps7_uart_1")

status = platform.build()

status = platform.build()

comp = client.get_component(name="xuartps_hello_world_example")
comp.build()

domain = platform.get_domain(name="zynq_fsbl")

status = domain.set_config(option = "os", param = "standalone_stdin", value = "ps7_uart_1")

status = domain.set_config(option = "os", param = "standalone_stdout", value = "ps7_uart_1")

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

