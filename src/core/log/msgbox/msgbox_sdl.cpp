// Shows message boxes with SDL on platforms without a native implementation.

#include "msgbox.h"

#include <SDL3/SDL_messagebox.h>

namespace uqm
{

void displayMessageBox(uqgsl::czstring utf8Title, uqgsl::czstring utf8Message, MessageBoxType type)
{
	const SDL_MessageBoxFlags flags {[type]() -> SDL_MessageBoxFlags {
		switch (type)
		{
			case MessageBoxType::Warning:
				return SDL_MESSAGEBOX_WARNING;
			case MessageBoxType::Error:
				return SDL_MESSAGEBOX_ERROR;
			case MessageBoxType::Info:
			default:
				return SDL_MESSAGEBOX_INFORMATION;
		}
	}()};
	SDL_ShowSimpleMessageBox(flags, utf8Title, utf8Message, nullptr);
}

} // namespace uqm
