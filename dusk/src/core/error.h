#pragma once

namespace dusk
{
	enum class Error
	{
		Ok,
		Generic,
		TimeOut,
		BufferTooSmall,
		OutOfMemory,
		InitializationFailed,
		DeviceLost,
		MemoryMapFailed,
		NotFound,
		WrongVersion,
		NotSupported
	};
}