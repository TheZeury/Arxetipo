namespace arx
{
	template<typename returnType, typename originalType>
	constexpr returnType& cnv(originalType& origin)
	{
		static_assert(sizeof(returnType) == sizeof(originalType));
		return reinterpret_cast<returnType&>(origin);
	}

	template<typename returnType, typename originalType>
	constexpr returnType&& cnv(originalType&& origin)
	{
		static_assert(sizeof(returnType) == sizeof(originalType));
		return reinterpret_cast<returnType&&>(origin);
	}

	template<typename returnType, typename originalType>
	constexpr const returnType& cnv(const originalType& origin)
	{
		static_assert(sizeof(returnType) == sizeof(originalType));
		return reinterpret_cast<const returnType&>(origin);
	}
}