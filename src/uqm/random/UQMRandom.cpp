// The random number generator of the game.
#include "uqm/random/UQMRandom.h"

#include "core/log/log.h"
#include "uqm/session/GameSession.h"
#include "UQMGame.h"


namespace uqm
{
UQMRandom& random()
{
	if (GameSession* const session {UQMGame::getInstance().session()})
	{
		return session->random();
	}
	static UQMRandom s_noSession {};
	static bool s_reported {};
	if (!s_reported)
	{
		s_reported = true;
		log::error("The random number generator was used while no game session runs.");
	}
	return s_noSession;
}

} // namespace uqm
