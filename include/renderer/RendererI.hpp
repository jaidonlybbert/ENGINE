#include<memory>
#include "logger/Logging.hpp"

class RenderAdapterI
{
public:
	virtual void* allocateDeviceMemory(const std::size_t size_bytes, const std::size_t swap_count) = 0;
	virtual void deallocateDeviceMemory(void* devMemoryPtr) = 0;

	template<typename T>
	std::shared_ptr<T> allocateDeviceMemoryAsType(const std::size_t size_bytes, const std::size_t swap_count)
	{
		auto* devMemoryPtr = this->allocateDeviceMemory(size_bytes, swap_count);
		std::shared_ptr<T> devSPtr(
			static_cast<T*>(devMemoryPtr),
			[this](T* p) {
				ENG_LOG_TRACE("Cleaning up memory at " << p << std::endl);
				this->deallocateDeviceMemory(p);
			}
		);
	}
};
