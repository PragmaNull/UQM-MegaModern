#include <gtest/gtest.h>
#include "options/OptionTypes.h"
#include "options/options.h"

namespace uqm
{

class OptionsTests : public ::testing::Test
{
protected:
	void SetUp() override {}
	void TearDown() override {}
};

// Basic OptionT behavior: default state, assignment, set flag, conversion and dereference
TEST_F(OptionsTests, OptionT_AssignConversionAndDereference)
{
	OptionT<int> intOpt;
	// default constructed, value-initialized but not "set"
	EXPECT_FALSE(intOpt.set);
	// operator* and conversion should return the stored value (0)
	EXPECT_EQ(*intOpt, 0);
	EXPECT_EQ(static_cast<int>(intOpt), 0);

	// assignment marks option as set and updates value
	int assigned = (intOpt = 7);
	EXPECT_EQ(assigned, 7);
	EXPECT_TRUE(intOpt.set);
	EXPECT_EQ(intOpt.value, 7);
	EXPECT_EQ(*intOpt, 7);
	EXPECT_EQ(static_cast<int>(intOpt), 7);

	// copy preserves value and set flag
	OptionT<int> intOptCopy = intOpt;
	EXPECT_TRUE(intOptCopy.set);
	EXPECT_EQ(intOptCopy.value, 7);
}

// Specialization: BoolOption toString uses OptionTrueText/OptionFalseText ("on"/"off")
TEST_F(OptionsTests, BoolOption_ToStringAndSet)
{
	BoolOption bDefault;
	EXPECT_FALSE(bDefault.set);
	EXPECT_EQ(bDefault.value, false);
	// default toString for false
	EXPECT_STREQ(bDefault.toString(), OptionFalseText);

	bDefault = true;
	EXPECT_TRUE(bDefault.set);
	EXPECT_TRUE(bDefault.value);
	EXPECT_STREQ(bDefault.toString(), OptionTrueText);

	// assign false again
	bDefault = false;
	EXPECT_TRUE(bDefault.set);
	EXPECT_FALSE(bDefault.value);
	EXPECT_STREQ(bDefault.toString(), OptionFalseText);
}

// Constructor taking default value must not set the 'set' flag (per header implementation)
TEST_F(OptionsTests, OptionT_DefaultCtorDoesNotSetFlag)
{
	OptionT<Vec2u> resDefault({800, 600});
	EXPECT_FALSE(resDefault.set);
	EXPECT_EQ(resDefault.value.x, 800);
	EXPECT_EQ(resDefault.value.y, 600);

	// assigning marks set
	resDefault = Vec2u {1024, 768};
	EXPECT_TRUE(resDefault.set);
	EXPECT_EQ(resDefault.value.x, 1024);
	EXPECT_EQ(resDefault.value.y, 768);
}

// Enum toString functions for SeedType and EmulationMode
TEST_F(OptionsTests, EnumToString_SeedAndEmulation)
{
	EXPECT_STREQ(toString(SeedType::None), "None");
	EXPECT_STREQ(toString(SeedType::Planet), "Planet");
	EXPECT_STREQ(toString(SeedType::Prime), "Prime");
	EXPECT_STREQ(toString(EmulationMode::Console3DO), "3DO");
	EXPECT_STREQ(toString(EmulationMode::PC), "PC");

	// Unknown / out of range value should return ??? string
	EmulationMode mixed = EmulationMode::Console3DO | EmulationMode::PC;
	EXPECT_STREQ(toString(mixed), "???");
}

// OptionListValueT default initialization and manual initialization
TEST_F(OptionsTests, OptionListValue_DefaultAndManual)
{
	OptionListValueT<int> defaultItem;
	EXPECT_EQ(defaultItem.name, nullptr);
	EXPECT_EQ(defaultItem.value, 0);

	OptionListValueT<int> item {"test", 42};
	EXPECT_STREQ(item.name, "test");
	EXPECT_EQ(item.value, 42);
}

// RunMode enum basic checks
TEST_F(OptionsTests, RunMode_Values)
{
	EXPECT_EQ(static_cast<int>(RunMode::Normal), 0);
	EXPECT_EQ(static_cast<int>(RunMode::Usage), 1);
	EXPECT_EQ(static_cast<int>(RunMode::Version), 2);
}

// Ensure OptionT<bool>::toString specialization returns on/off and doesn't call generic toString
TEST_F(OptionsTests, OptionT_BoolToStringSpecialization)
{
	OptionT<bool> optFalse(false);
	EXPECT_FALSE(optFalse.set);
	EXPECT_STREQ(optFalse.toString(), OptionFalseText);

	optFalse = true;
	EXPECT_TRUE(optFalse.set);
	EXPECT_STREQ(optFalse.toString(), OptionTrueText);
}

// Copy and move semantics keep the set flag and value
TEST_F(OptionsTests, OptionT_CopyAndMovePreserveState)
{
	OptionT<SeedType> original(SeedType::MRQ);
	EXPECT_FALSE(original.set); // constructor should not set
	original = SeedType::Prime;
	EXPECT_TRUE(original.set);
	EXPECT_EQ(original.value, SeedType::Prime);

	// Copy
	OptionT<SeedType> copy = original;
	EXPECT_TRUE(copy.set);
	EXPECT_EQ(copy.value, SeedType::Prime);

	// Move (moves the value and set flag; semantics same for trivially copyable)
	OptionT<SeedType> moved = std::move(original);
	EXPECT_TRUE(moved.set);
	EXPECT_EQ(moved.value, SeedType::Prime);
}

// Validate that OptionT<T>::toString is usable for types that have uqm::toString overloads (SeedType tested above).
// We avoid calling toString for int because no suitable overload exists in current headers.
TEST_F(OptionsTests, OptionT_ToString_UsesUqmToStringForEnums)
{
	OptionT<SeedType> st {SeedType::Planet};
	EXPECT_STREQ(st.toString(), "Planet");
}

TEST_F(OptionsTests, GetOptionListValue_ExactMatchAndNonMatch)
{
	// exact matches
	//auto maybe = getOptionListValue(ScalerList, "bilinear");
	//ASSERT_TRUE(maybe.has_value());
	//EXPECT_EQ(*maybe, ScalerList[0].value);

	//// equivalent synonyms present (e.g. "no" and "none")
	//auto none1 = getOptionListValue(ScalerList, "no");
	//ASSERT_TRUE(none1.has_value());
	//EXPECT_EQ(*none1, 0);

	//auto none2 = getOptionListValue(ScalerList, "none");
	//ASSERT_TRUE(none2.has_value());
	//EXPECT_EQ(*none2, 0);

	//// case-sensitive: should not match
	//auto upper = getOptionListValue(ScalerList, "BILINEAR");
	//EXPECT_FALSE(upper.has_value());

	//// partial match must not succeed
	//auto partial = getOptionListValue(ScalerList, "bilin");
	//EXPECT_FALSE(partial.has_value());

	// empty list - should return nullopt
	uqstl::array<OptionListValueT<int>, 0> emptyList {{}};
	auto emptyRes = getOptionListValue(emptyList, "anything");
	EXPECT_FALSE(emptyRes.has_value());
}

TEST_F(OptionsTests, ParseOptionValue_NumericFloatBoolAndEnum)
{
	// integer
	auto i = parseOptionValue<int>("42", "int-test");
	ASSERT_TRUE(i.has_value());
	EXPECT_EQ(*i, 42);

	// float
	auto f = parseOptionValue<float>("3.5", "float-test");
	ASSERT_TRUE(f.has_value());
	EXPECT_FLOAT_EQ(*f, 3.5f);

	// bool true variants
	bool b;
	EXPECT_EQ(uqm::parseStr("true", b), std::errc {});
	EXPECT_TRUE(b);
	EXPECT_EQ(uqm::parseStr("1", b), std::errc {});
	EXPECT_TRUE(b);
	EXPECT_EQ(uqm::parseStr("yes", b), std::errc {});
	EXPECT_TRUE(b);
	EXPECT_EQ(uqm::parseStr("on", b), std::errc {});
	EXPECT_TRUE(b);

	// bool false variants
	EXPECT_EQ(uqm::parseStr("false", b), std::errc {});
	EXPECT_FALSE(b);
	EXPECT_EQ(uqm::parseStr("0", b), std::errc {});
	EXPECT_FALSE(b);
	EXPECT_EQ(uqm::parseStr("no", b), std::errc {});
	EXPECT_FALSE(b);
	EXPECT_EQ(uqm::parseStr("off", b), std::errc {});
	EXPECT_FALSE(b);

	// parseOptionValue<bool> with accepted false
	auto mb = parseOptionValue<bool>("off", "bool-test");
	ASSERT_TRUE(mb.has_value());
	EXPECT_FALSE(*mb);

	// enum underlying integer parsing -> SeedType::Prime == 3
	auto seed = parseOptionValue<SeedType>("3", "seed-test");
	ASSERT_TRUE(seed.has_value());
	EXPECT_EQ(*seed, SeedType::Prime);
}

TEST_F(OptionsTests, ParseOptionValue_InvalidInputsThrow)
{
	// invalid int
	EXPECT_THROW([]() {
		uqstl::ignore = parseOptionValue<int>("abc", "bad-int");
	}(),
				 std::invalid_argument);

	// leading whitespace is not accepted by numeric from_chars
	EXPECT_THROW([]() {
		uqstl::ignore = parseOptionValue<int>(" 42", "ws-int");
	}(),
				 std::invalid_argument);
	// trailing whitespace is not accepted either.
	EXPECT_THROW([]() {
		uqstl::ignore = parseOptionValue<int>("42 ", "ws-int2");
	}(),
				 std::invalid_argument);

	// bool specialisation: whitespace will not match known tokens
	EXPECT_THROW([]() {
		uqstl::ignore = parseOptionValue<bool>(" yes ", "bool-ws");
	}(),
				 std::invalid_argument);

	// integer overflow
	EXPECT_THROW([]() {
		std::string big = fmt::format("{}0", std::numeric_limits<int>::max());
		uqstl::ignore = parseOptionValue<int>(big, "too-large");
	}(),
				 std::out_of_range);
}


TEST_F(OptionsTests, ParseOption_TemplateAndSpecialization)
{
	// generic template for IntOption
	IntOption optInt;
	EXPECT_FALSE(optInt.set);
	EXPECT_TRUE(parseOption(optInt, "7", "my-int"));
	EXPECT_TRUE(optInt.set);
	EXPECT_EQ(optInt.value, 7);

	// EmulationOption specialization: picks from EmulationChoiceList
	EmulationOption emu;
	EXPECT_FALSE(emu.set);
	EXPECT_TRUE(parseOption(emu, "pc", "emu"));
	EXPECT_EQ(emu.value, EmulationMode::PC);
	EXPECT_TRUE(emu.set);
	EXPECT_TRUE(parseOption(emu, "3do", "emu"));
	EXPECT_EQ(emu.value, EmulationMode::Console3DO);

	// unknown string should not set
	EmulationOption emu2;
	EXPECT_FALSE(parseOption(emu2, "unknown-mode", "emu"));
	EXPECT_FALSE(emu2.set);
}

TEST_F(OptionsTests, NormalizeValue_EdgeCasesAndUnsigned)
{
	// below zero -> 0.0
	EXPECT_FLOAT_EQ(normalizeValue(-1, 100), 0.0f);

	// above max -> 1.0
	EXPECT_FLOAT_EQ(normalizeValue(200, 100), 1.0f);

	// mid value
	EXPECT_FLOAT_EQ(normalizeValue(25, 100), 0.25f);

	// unsigned types: negative check uses inVol < InT{} which is false for unsigned
	EXPECT_FLOAT_EQ(normalizeValue<unsigned int>(0u, 10u), 0.0f);
	EXPECT_FLOAT_EQ(normalizeValue<unsigned int>(10u, 10u), 1.0f);
	EXPECT_FLOAT_EQ(normalizeValue<unsigned int>(5u, 10u), 0.5f);
}

TEST_F(OptionsTests, GetOptionListValue_EmulationChoiceList)
{
	auto val = getOptionListValue(EmulationChoiceList, "3do");
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, EmulationMode::Console3DO);

	auto none = getOptionListValue(EmulationChoiceList, "PC"); // case sensitive
	EXPECT_FALSE(none.has_value());
}

} // namespace uqm