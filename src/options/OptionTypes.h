#pragma once
#ifndef UQM_OPTIONS_OPTIONTYPES_H
#define UQM_OPTIONS_OPTIONTYPES_H

#include "core/stl/stl.h"
#include "core/types/enum.h"
#include "core/string/StringUtils.h"
#include "options/OptionDefs.h"

namespace uqm
{

///////////////////////////////////////////////////////////////////////////////////////////////////
//* Option types
//*
//* These are the types of options used by the setup menu and config file. They are designed to be
//* simple and easy to use, but they don't have any built-in validation or range checking, so be
//* careful when using them.
///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct OptionT
{
	using value_type = T;

	T value {};
	bool set {};

	constexpr OptionT() = default;
	constexpr explicit OptionT(T defaultValue)
		: value(defaultValue)
	{
	}

	T operator=(T newValue)
	{
		value = newValue;
		set = true;
		return newValue;
	}

	const T& operator*() const
	{
		return value;
	}
	operator const T&() const
	{
		return value;
	}

	T& edit()
	{
		set = true;
		return value;
	}

	const char* toString() const;
};

template <typename T>
struct OptionListValueT
{
	using value_type = T;
	uqgsl::czstring name {};
	T value {};
};

using BoolOption = OptionT<bool>;
using IntOption = OptionT<int>;
using FloatOption = OptionT<float>;
using EmulationOption = OptionT<EmulationMode>;

static inline constexpr const char* OptionTrueText {"on"};
static inline constexpr const char* OptionFalseText {"off"};

template <typename T>
inline const char* OptionT<T>::toString() const
{
	return ::uqm::toString(value);
}

template <>
inline const char* OptionT<bool>::toString() const
{
	return value ? OptionTrueText : OptionFalseText;
}

} // namespace uqm

#endif // UQM_OPTIONS_OPTIONTYPES_H
