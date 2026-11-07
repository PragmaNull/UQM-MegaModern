// Runs work from the ImGui windows on the game thread.

#include "gamethread.h"
#include "libs/callback/callback.h"


namespace uqm
{
void runOnGameThread(uqstl::function<void()> task)
{
	auto owned {uqstl::make_unique<uqstl::function<void()>>(uqstl::move(task))};
	Callback_add(
		[](CallbackArg arg) {
			const uqstl::unique_ptr<uqstl::function<void()>> task {static_cast<uqstl::function<void()>*>(arg)};
			(*task)();
		},
		owned.release());
}

} // namespace uqm
