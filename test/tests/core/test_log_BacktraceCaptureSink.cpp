#include <gtest/gtest.h>

#include "core/stl/string.h"
#include "core/stl/sstream.h"
#include "core/stl/stl.h"

#include "core/log/BacktraceCaptureSink.h"
#include <spdlog/details/log_msg.h>
#include <spdlog/common.h>

namespace uqm
{

class BacktraceCaptureSinkMock : public BacktraceCaptureSink
{
public:
	using BacktraceCaptureSink::at;
	using BacktraceCaptureSink::BacktraceCaptureSink;
	using BacktraceCaptureSink::getIndex;
	using BacktraceCaptureSink::sink_it_;
};


// Fixture name must match friend declaration in BacktraceCaptureSink so we
// can access private members for thorough testing.
class BacktraceCaptureSinkTests : public ::testing::Test
{
protected:
	void SetUp() override {}
	void TearDown() override {}
};

// Helper to create a log_msg with given payload text.
static spdlog::details::log_msg make_msg(const uqstl::string_view payload)
{
	const spdlog::details::log_msg msg {spdlog::source_loc {}, "test_logger", spdlog::level::info, payload};
	return msg;
}

// Read a spdlog::memory_buf_t into std::string
static std::string buf_to_string(const spdlog::memory_buf_t& buf)
{
	return std::string(buf.data(), buf.data() + buf.size());
}

TEST_F(BacktraceCaptureSinkTests, DefaultConstructionEmptyBuffer)
{
	const BacktraceCaptureSinkMock sink;

	// Initially index zero and all buffers empty
	EXPECT_EQ(sink.getIndex(), 0u);

	// getBufferLines on empty sink yields empty result
	uqstl::string out;
	sink.getBufferLines(out);
	EXPECT_TRUE(out.empty());
}

TEST_F(BacktraceCaptureSinkTests, SinkItStoresMessagePayloadSubstring)
{
	BacktraceCaptureSinkMock sink;
	auto msg = make_msg("hello-world-42");
	sink.sink_it_(msg);

	// m_index should have incremented
	EXPECT_EQ(sink.getIndex(), 1u);

	// The stored buffer at logical index 0 should contain the payload string.
	const auto& stored = sink.at(0);
	const std::string storedStr = buf_to_string(stored);
	EXPECT_NE(storedStr.find("hello-world-42"), std::string::npos);
}

TEST_F(BacktraceCaptureSinkTests, MultipleMessagesGetBufferLinesConcatenatesWithNewlines)
{
	BacktraceCaptureSinkMock sink;

	const std::vector<std::string> messages = {
		"first",
		"second-line",
		"third"};

	for (const auto& m : messages)
	{
		auto msg = make_msg(m);
		sink.sink_it_(msg);
	}

	uqstl::string out;
	sink.getBufferLines(out);

	// Expect three lines separated by '\n' and ending with newline for each
	std::stringstream ss(out);
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(ss, line))
	{
		lines.push_back(line);
	}

	EXPECT_EQ(lines.size(), messages.size());
	for (size_t i = 0; i < messages.size(); ++i)
	{
		EXPECT_NE(lines[i].find(messages[i]), std::string::npos);
	}
}

TEST_F(BacktraceCaptureSinkTests, OverwriteWhenFullKeepsLatestBacktraceSize)
{
	BacktraceCaptureSinkMock sink;

	const size_t total = BacktraceSize + 10u;
	for (size_t i = 0; i < total; ++i)
	{
		const std::string s {"msg-" + std::to_string(i)};
		auto msg = make_msg(s);
		sink.sink_it_(msg);
	}

	// m_index should equal total
	EXPECT_EQ(sink.getIndex(), total);

	// getBufferLines should return last BacktraceSize messages
	uqstl::string out;
	sink.getBufferLines(out);

	// Count lines
	size_t count = 0;
	for (const char c : out)
	{
		if (c == '\n')
		{
			++count;
		}
	}
	EXPECT_EQ(count, BacktraceSize);

	// Ensure the earliest returned line corresponds to total - BacktraceSize
	std::stringstream ss(out);
	std::string line;
	std::getline(ss, line);
	EXPECT_NE(line.find("msg-" + std::to_string(total - BacktraceSize)), std::string::npos);

	// Ensure the last returned line corresponds to the most recent message
	std::string lastLine;
	while (std::getline(ss, line))
	{
		lastLine = line;
	}
	EXPECT_NE(lastLine.find("msg-" + std::to_string(total - 1)), std::string::npos);
}

TEST_F(BacktraceCaptureSinkTests, EmptyPayloadProducesEmptyLine)
{
	BacktraceCaptureSinkMock sink;
	auto msg = make_msg("");
	sink.sink_it_(msg);

	uqstl::string out;
	sink.getBufferLines(out);

	// Should contain exactly one newline and no other content (aside from possible formatting text).
	// We check that at least a newline exists and that the stored buffer contains empty payload (formatter may add extras).
	EXPECT_EQ(sink.getIndex(), 1u);

	const auto& stored = sink.at(0);
	// Stored buffer may contain only formatter output; ensure newline present via getBufferLines
	EXPECT_FALSE(out.empty());
	EXPECT_EQ(std::count(out.begin(), out.end(), '\n'), 1);
}

TEST_F(BacktraceCaptureSinkTests, InterleavedPopulatedAndEmptyMessages)
{
	BacktraceCaptureSinkMock sink;

	// Push alternating non-empty and empty payloads up to a small number
	for (int i = 0; i < 8; ++i)
	{
		const std::string s {(i % 2 == 0) ? ("A-" + std::to_string(i)) : ""};
		auto msg = make_msg(s);
		sink.sink_it_(msg);
	}

	uqstl::string out;
	sink.getBufferLines(out);

	// Should contain 8 lines
	EXPECT_EQ(std::count(out.begin(), out.end(), '\n'), 8);

	// Verify that non-empty payloads are present
	for (int i = 0; i < 8; i += 2)
	{
		const std::string token = "A-" + std::to_string(i);
		EXPECT_NE(out.find(token), uqstl::string::npos);
	}
}

} // namespace uqm