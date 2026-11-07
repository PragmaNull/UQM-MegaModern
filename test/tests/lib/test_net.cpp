// Tests for the TCP connections, with a listener and a connector on the loopback address.
#include <gtest/gtest.h>

#include "libs/network/Connection.h"
#include "libs/network/Connector.h"
#include "libs/network/Listener.h"
#include "libs/network/NetContext.h"


using namespace uqm::net;
using namespace std::chrono_literals;

namespace
{

constexpr size_t c_bufferSize {64};

using ErrorHandler = uqstl::function<void(std::error_code)>;

// Polls the context until 'done' is true. Returns false after 5 seconds.
bool pollUntil(Context& context, const uqstl::function<bool()>& done)
{
	const auto deadline {uqstl::chrono::steady_clock::now() + 5s};
	while (!done())
	{
		if (uqstl::chrono::steady_clock::now() > deadline)
		{
			return false;
		}
		context.poll(10ms);
	}
	return true;
}

// Polls the context without waiting until 'done' is true. Returns false after 5 seconds.
bool pollWithoutWaitUntil(Context& context, const uqstl::function<bool()>& done)
{
	const auto deadline {uqstl::chrono::steady_clock::now() + 5s};
	while (!done())
	{
		if (uqstl::chrono::steady_clock::now() > deadline)
		{
			return false;
		}
		context.poll(0ms);
	}
	return true;
}

// Polls the context for 'duration'.
void pollFor(Context& context, uqstl::chrono::milliseconds duration)
{
	const auto end {uqstl::chrono::steady_clock::now() + duration};
	while (uqstl::chrono::steady_clock::now() < end)
	{
		context.poll(10ms);
	}
}

uqstl::span<const std::byte> bytes(uqstl::string_view text)
{
	return uqstl::as_bytes(uqstl::span<const char> {text.data(), text.size()});
}

uqstl::string text(uqstl::span<const std::byte> data)
{
	return uqstl::string {reinterpret_cast<const char*>(data.data()), data.size()};
}

uqstl::function<void(Connection)> storeConnection(uqstl::optional<Connection>& target)
{
	return [&target](Connection connection) {
		target.emplace(uqstl::move(connection));
	};
}

ErrorHandler storeError(uqstl::optional<std::error_code>& target)
{
	return [&target](std::error_code error) {
		target = error;
	};
}

uqstl::function<void(Connection)> setFlagOnConnection(bool& flag)
{
	return [&flag](Connection) {
		flag = true;
	};
}

ErrorHandler setFlagOnError(bool& flag)
{
	return [&flag](std::error_code) {
		flag = true;
	};
}

void ignoreConnection(Connection)
{
}

void ignoreError(std::error_code)
{
}

// Appends the received data to 'target' and processes all of it.
Connection::ReceiveHandler appendTo(uqstl::string& target)
{
	return [&target](uqstl::span<const std::byte> data) {
		target += text(data);
		return uqstl::optional<size_t> {data.size()};
	};
}

// Adds each whole 4-byte packet to 'target'.
Connection::ReceiveHandler splitPackets(uqstl::vector<uqstl::string>& target)
{
	return [&target](uqstl::span<const std::byte> data) {
		size_t processed {};
		while (data.size() - processed >= 4)
		{
			target.push_back(text(data.subspan(processed, 4)));
			processed += 4;
		}
		return uqstl::optional<size_t> {processed};
	};
}

uqstl::optional<size_t> processAll(uqstl::span<const std::byte> data)
{
	return data.size();
}

uqstl::optional<size_t> rejectAll(uqstl::span<const std::byte>)
{
	return uqstl::nullopt;
}

ConnectOptions loopbackOptions(uint16_t port)
{
	return ConnectOptions {.host = "127.0.0.1", .service = uqstl::to_string(port), .ipv4Only = true};
}

// A port that no listener uses.
uint16_t unusedPort(Context& context)
{
	const Listener listener {context, ListenOptions {.ipv4Only = true}, ignoreConnection, ignoreError};
	return listener.port();
}

// The two ends of a loopback connection.
struct Loopback
{
	Context context {};
	uqstl::optional<Connection> server {};
	uqstl::optional<Connection> client {};
	uqstl::optional<std::error_code> error {};

	bool connect()
	{
		const Listener listener {context, ListenOptions {.ipv4Only = true}, storeConnection(server), storeError(error)};
		const Connector connector {context, loopbackOptions(listener.port()), storeConnection(client), storeError(error)};
		const bool done {pollUntil(context, [this] {
			return (server && client) || error;
		})};
		return done && !error;
	}
};

} // namespace

TEST(NetTest, ExchangesData)
{
	Loopback loopback {};
	ASSERT_TRUE(loopback.connect());

	uqstl::string serverReceived {};
	uqstl::string clientReceived {};
	loopback.server->start(c_bufferSize, appendTo(serverReceived), ignoreError);
	loopback.client->start(c_bufferSize, appendTo(clientReceived), ignoreError);

	EXPECT_FALSE(loopback.client->send(bytes("ping")));
	EXPECT_TRUE(pollUntil(loopback.context, [&] {
		return serverReceived == "ping";
	}));
	EXPECT_FALSE(loopback.server->send(bytes("pong")));
	EXPECT_TRUE(pollUntil(loopback.context, [&] {
		return clientReceived == "pong";
	}));
}

TEST(NetTest, ConnectsWithPollsThatDoNotWait)
{
	Context context {};
	uqstl::optional<Connection> server {};
	uqstl::optional<Connection> client {};
	const Listener listener {context, ListenOptions {.ipv4Only = true}, storeConnection(server), ignoreError};
	const Connector connector {context, loopbackOptions(listener.port()), storeConnection(client), ignoreError};

	EXPECT_TRUE(pollWithoutWaitUntil(context, [&] {
		return client && server;
	}));
}

TEST(NetTest, KeepsTheDataThatIsNotProcessed)
{
	Loopback loopback {};
	ASSERT_TRUE(loopback.connect());

	uqstl::vector<uqstl::string> packets {};
	loopback.server->start(c_bufferSize, splitPackets(packets), ignoreError);

	EXPECT_FALSE(loopback.client->send(bytes("aaaab")));
	EXPECT_TRUE(pollUntil(loopback.context, [&] {
		return packets.size() == 1;
	}));
	EXPECT_FALSE(loopback.client->send(bytes("bbbcccc")));
	EXPECT_TRUE(pollUntil(loopback.context, [&] {
		return packets.size() == 3;
	}));
	EXPECT_EQ(packets, (uqstl::vector<uqstl::string> {"aaaa", "bbbb", "cccc"}));
}

TEST(NetTest, ReportsThatThePeerClosedTheConnection)
{
	Loopback loopback {};
	ASSERT_TRUE(loopback.connect());

	uqstl::optional<std::error_code> closeError {};
	loopback.server->start(c_bufferSize, processAll, storeError(closeError));
	loopback.client.reset();

	EXPECT_TRUE(pollUntil(loopback.context, [&] {
		return closeError.has_value();
	}));
	EXPECT_FALSE(*closeError);
	EXPECT_FALSE(loopback.server->isOpen());
}

TEST(NetTest, ClosesTheConnectionOnInvalidData)
{
	Loopback loopback {};
	ASSERT_TRUE(loopback.connect());

	uqstl::optional<std::error_code> closeError {};
	loopback.server->start(c_bufferSize, rejectAll, storeError(closeError));

	EXPECT_FALSE(loopback.client->send(bytes("bad")));
	EXPECT_TRUE(pollUntil(loopback.context, [&] {
		return closeError.has_value();
	}));
	EXPECT_EQ(*closeError, std::errc::bad_message);
}

TEST(NetTest, DoesNotCallTheHandlersAfterClose)
{
	Loopback loopback {};
	ASSERT_TRUE(loopback.connect());

	uqstl::string received {};
	bool closed {};
	loopback.server->start(c_bufferSize, appendTo(received), setFlagOnError(closed));
	EXPECT_FALSE(loopback.client->send(bytes("data")));
	loopback.server.reset();
	loopback.client.reset();

	pollFor(loopback.context, 100ms);
	EXPECT_TRUE(received.empty());
	EXPECT_FALSE(closed);
}

TEST(NetTest, ReportsAConnectErrorWithoutRetry)
{
	Context context {};
	ConnectOptions options {loopbackOptions(unusedPort(context))};
	options.timeout = 500ms;

	uqstl::optional<std::error_code> error {};
	bool connected {};
	const Connector connector {context, options, setFlagOnConnection(connected), storeError(error)};

	EXPECT_TRUE(pollUntil(context, [&] {
		return error.has_value();
	}));
	EXPECT_FALSE(connected);
}

TEST(NetTest, RetriesUntilTheHostListens)
{
	Context context {};
	const uint16_t port {unusedPort(context)};
	ConnectOptions options {loopbackOptions(port)};
	options.timeout = 500ms;
	options.retryDelay = 100ms;

	uqstl::optional<Connection> client {};
	bool failed {};
	const Connector connector {context, options, storeConnection(client), setFlagOnError(failed)};
	pollFor(context, 300ms);
	EXPECT_FALSE(client.has_value());

	uqstl::optional<Connection> server {};
	const Listener listener {
		context, ListenOptions {.port = port, .ipv4Only = true},
		storeConnection(server), ignoreError
	};
	EXPECT_TRUE(pollUntil(context, [&] {
		return client && server;
	}));
	EXPECT_FALSE(failed);
}

TEST(NetTest, AcceptsIpv4OnADualStackListener)
{
	Context context {};
	uqstl::optional<Connection> server {};
	uqstl::optional<Connection> client {};
	const Listener listener {context, ListenOptions {}, storeConnection(server), ignoreError};
	ConnectOptions options {loopbackOptions(listener.port())};
	options.ipv4Only = false;
	const Connector connector {context, options, storeConnection(client), ignoreError};

	EXPECT_TRUE(pollUntil(context, [&] {
		return client && server;
	}));
}

TEST(NetTest, DoesNotCallTheHandlersAfterTheConnectorIsDestroyed)
{
	Context context {};
	bool called {};
	{
		const Listener listener {context, ListenOptions {.ipv4Only = true}, ignoreConnection, ignoreError};
		const Connector connector {context, loopbackOptions(listener.port()), setFlagOnConnection(called), setFlagOnError(called)};
	}

	pollFor(context, 100ms);
	EXPECT_FALSE(called);
}

TEST(NetTest, ReportsAListenError)
{
	Context context {};
	const Listener first {context, ListenOptions {.ipv4Only = true}, ignoreConnection, ignoreError};

	uqstl::optional<std::error_code> error {};
	const Listener second {
		context, ListenOptions {.port = first.port(), .ipv4Only = true},
		ignoreConnection, storeError(error)
	   };

	EXPECT_FALSE(error.has_value());
	EXPECT_TRUE(pollUntil(context, [&] {
		return error.has_value();
	}));
	EXPECT_EQ(second.port(), 0);
}
