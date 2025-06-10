# 2025-06-08T22:39:42.193860100
import vitis

client = vitis.create_client()
client.set_workspace(path="pstest")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="xusbps_intr_example")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

client.delete_component(name="xgpiops_polled_example")

client.delete_component(name="componentName")

client.delete_component(name="componentName")

client.delete_component(name="componentName")

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

