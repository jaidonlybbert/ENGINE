import os


def set_mac_env(env, source_dir):
    vulkan_sdk = os.path.expanduser("~/VulkanSDK/1.4.335.1/macOS")
    env["VULKAN_SDK"] = vulkan_sdk
    # setting these will force KosmicKrisp - which seems broken as of now
    # env["VK_DRIVER_FILES"] = \
    #     f"{source_dir}/libkosmickrisp_icd.json"
    # existing_dyld = env.get("DYLD_LIBRARY_PATH", "")
    # env["DYLD_LIBRARY_PATH"] = f"{vulkan_sdk}/lib:{existing_dyld}".strip(":")
