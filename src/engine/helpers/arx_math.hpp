namespace arx
{
	template<typename returnType, typename originalType>
	constexpr inline auto cnv(originalType& origin) -> returnType&
	{
		static_assert(sizeof(returnType) == sizeof(originalType));
		return reinterpret_cast<returnType&>(origin);
	}

	template<typename returnType, typename originalType>
	constexpr inline auto cnv(originalType&& origin) -> returnType&&
	{
		static_assert(sizeof(returnType) == sizeof(originalType));
		return reinterpret_cast<returnType&&>(origin);
	}

	template<typename returnType, typename originalType>
	constexpr inline auto cnv(const originalType& origin) -> const returnType&
	{
		static_assert(sizeof(returnType) == sizeof(originalType));
		return reinterpret_cast<const returnType&>(origin);
	}
}