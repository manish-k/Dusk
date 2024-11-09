#pragma once

#include<memory>

namespace dusk
{
	template<typename T>
	using UniqueP = std::unique_ptr<T>;

	template<typename T>
	using SharedP = std::shared_ptr<T>;

	template<typename T, typename... Args>
	constexpr UniqueP<T> createUniqueP(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T, typename... Args>
	constexpr SharedP<T> createSharedP(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
}