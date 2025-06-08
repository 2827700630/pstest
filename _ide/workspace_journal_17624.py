# 2025-06-08T22:02:44.464855200
import vitis

client = vitis.create_client()
client.set_workspace(path="pstest")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="xusbps_intr_example")
comp.build()

status = platform.build()

comp.build()

vitis.dispose()

