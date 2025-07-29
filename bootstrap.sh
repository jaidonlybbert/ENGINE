export VULKAN_SDK=/Users/jelly/VulkanSDK/1.3.283.0/macOS
export PATH=$VULKAN_SDK/bin:$PATH
export DYLD_LIBRARY_PATH=$VULKAN_SDK/lib
export VK_ICD_FILENAMES=$VULKAN_SDK/share/vulkan/icd.d/MoltenVK_icd.json
export VK_LAYER_PATH=$VULKAN_SDK/share/vulkan/explicit_layer.d
cd build
cmake -G "Ninja" -D CMAKE_CXX_COMPILER=clang++ -D CMAKE_C_COMPILER=clang .. &
wait
cmake --build . --config Debug &
wait
./Engine
