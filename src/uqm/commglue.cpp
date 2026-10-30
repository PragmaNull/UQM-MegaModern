//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define DEBUG_STARSEED
#include "commglue.h"

#include "battle.h"
// For instantVictory
#include "races.h"
#include "lua/luacomm.h"
#include "core/log/log.h"
#include "core/string/StringUtils.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include "libs/math/random.h"

uint16_t RoboTrack[NUM_ROBO_TRACKS];

static int NPCNumberPhrase(int number, const char* fmt, char** ptrack);

// Scans forward until outside of the interpolation then returns the start
// of the new section of text.
char* ScanInterpolation(char* start)
{
	uint16_t depth = 1;
	if (!start || start[0] != '<')
	{
		return start;
	}
	start++;
	while (*start && depth > 0)
	{
		if (start[0] == '<' && start[1] == '%')
		{
			depth++;
		}
		if (start[0] == '%' && start[1] == '>')
		{
			depth--;
		}
		start++;
	}
	// If we're not on a null character, it's the >, roll forward.
	if (*start)
	{
		start++;
	}
	return start;
}

// Will the chunk between [end - start] require robot voicing?
bool RoboInterpolation(char* start, char* end)
{
	static constexpr const char* RoboPhrases[] = {
		"getPoint",
		"getStarName",
		"getConstellation",
		"getColor",
		"swapIfSeeded"};

	for (const char* phrase : RoboPhrases)
	{
		if (char* result {strstr(start, phrase)}; result && result < end)
		{
			return true;
		}
	}
	return false;
}

// This will write to buffer the interpolated chunk, while returning a new
// "start" value from the original string where interpolation ended.
char* InterpolateChunk(uqstl::span<char> buffer, char* start)
{
	char* end = start;
	char str_buf[MAX_INTERPOLATE] = "";
	char* pStr;
	uint16_t buffsize = 0;
	bool done = false;

	while ((end = strstr(start, "<%")) && !done)
	{
		// Copy over plain text part
		if (end != start)
		{
			buffsize += end - start;
			if (buffsize > MAX_INTERPOLATE)
			{
				fmt::print(stderr, "String too long to interpolate.\n");
				return nullptr;
			}
			uqm::strncpy_safe(buffer, {start, end});
			buffer = buffer.subspan(end - start);
			start = end;
		}

		// Next we grab only the smallest chunk we can interpolate
		end = ScanInterpolation(start);
		if (RoboInterpolation(start, end))
		{
			// This requires robo-interpolation.  If anything else was
			// already read, we return and handle that first.  Otherwise,
			// handle this interpolation, but then we're done.
			if (buffsize > 0)
			{
				return start;
			}
			done = true;
		}

		uqm::strncpy_safe(str_buf, {start, end});
		const uqstl::string interpolated {luaUqm_comm_stringInterpolate(str_buf)};
		buffsize += (uint16_t)interpolated.size();
		if (buffsize > MAX_INTERPOLATE)
		{
			fmt::print(stderr, "String too long to interpolate.\n");
			return nullptr;
		}
		uqm::strncpy_safe(buffer, interpolated);
		buffer = buffer.subspan(end - start);
		start = end;
	}
	// If we ended because of robointerpolation...
	if (done)
	{
		return start;
	}
	// Otherwise we're done with interpolation, write the remainder
	// to buffer and return
	buffsize += (uint16_t)strlen(start);
	if (buffsize > MAX_INTERPOLATE)
	{
		fmt::print(stderr, "String too long to interpolate.\n");
		return nullptr;
	}
	uqm::strncpy_safe(buffer, start);
	return nullptr;
}

// Creates the file name of subclip # clip_number, and prints it to buffer.
// Assumes track names end in ".ogg".
void GetSubClip(uqstl::span<char> buffer, char* pClip, uint16_t clip_number)
{
	char* pStr = strstr(pClip, ".ogg");
	if (!pStr)
	{
		// Fall through passing back the whole thing
		uqm::strncpy_safe(buffer, pClip);
		return;
	}
	fmt::format_to_sz_n(buffer, "{}{}.ogg", std::string_view(pClip, pStr - pClip), 'a' + clip_number);
}

// The CallbackFunction is queued and executes synchronously
// on the Starcon2Main thread
void NPCPhrase_cb(int index, CallbackFunction cb)
{
	char* pStr;
	char* pClip;
	char* pTimeStamp;
	uint16_t clip_number = 0;
	uint16_t i;

	if (index == 0)
	{
		return;
	}

	pStr = (char*)GetStringAddress(
		SetAbsStringTableIndex(CommData.ConversationPhrases, index - 1));
	pClip = GetStringSoundClip(
		SetAbsStringTableIndex(CommData.ConversationPhrases, index - 1));
	pTimeStamp = GetStringTimeStamp(
		SetAbsStringTableIndex(CommData.ConversationPhrases, index - 1));

	if (!StarSeed)
	{
		if (luaUqm_comm_stringNeedsInterpolate(pStr))
		{
			const uqstl::string interpolated {luaUqm_comm_stringInterpolate(pStr)};
			SpliceTrack(pClip, interpolated.c_str(), pTimeStamp, cb);
		}
		else
		{
			SpliceTrack(pClip, pStr, pTimeStamp, cb);
		}
		return;
	}

	// From here on, we are doing StarSeed robo-interpolation.
	static STRING RoboPhrases = nullptr;
	if (!RoboPhrases)
	{
		RoboPhrases = CaptureStringTable(
			LoadStringTableInstance("comm.robot.dialogue"));
	}
	for (i = 0; RoboTrack[i] && i < NUM_ROBO_TRACKS; i++)
	{
		RoboTrack[i] = 0;
	}
#ifdef DEBUG_STARSEED_TRACE_TIMESTAMP
	// This code will roll the "SENSE_KOHRAH_VICTORY" dialog and timestamps
	// instead of the correct ones for debugging or track syncing purposes.
	STRING Pkunk = CaptureStringTable(
		LoadStringTableInstance("comm.pkunk.dialogue"));
	pStr = (char*)GetStringAddress(
		SetAbsStringTableIndex(Pkunk, 43));
	pClip = GetStringSoundClip(
		SetAbsStringTableIndex(Pkunk, 43));
	pTimeStamp = GetStringTimeStamp(
		SetAbsStringTableIndex(Pkunk, 43));
#endif

	// Switch to alternate time stamps if they exist
	if (pTimeStamp)
	{
		if (strstr(pTimeStamp, ";"))
		{
			pTimeStamp = strstr(pTimeStamp, ";") + 1;
		}
	}

#ifdef DEBUG_STARSEED
	fmt::print(stderr, "Received string...\n<<\n{}\n>>\n", pStr);
#endif

	// Here we will loop through and get the string up to the smallest
	// interpolation, then chunk it, then repeat until done.
	do
	{
		// Get fresh buffers every loop
		char str_buf[MAX_INTERPOLATE] = "";
		char clip_buf[MAX_CLIPNAME] = "";
#if 0
		// Gotta test and make sure we don't need this intensity
			for (i = 0; i < MAX_INTERPOLATE; i++)
				str_buf[i] = '\0';
			for (i = 0; i < MAX_CLIPNAME; i++)
				clip_buf[i] = '\0';
#endif
		// InterpolateChunk returns a pointer to the start of the next
		// chunk, or nullptr if done.  Writes the interpolation to str_buf.
		pStr = InterpolateChunk(str_buf, pStr);
#ifdef DEBUG_STARSEED
		fmt::print(stderr, "Chunk\n<<{}>>\n", str_buf);
#endif
		if (!RoboTrack[0])
		{
			if (clip_number == 0 && !pStr)
			{
#ifdef DEBUG_STARSEED
				fmt::print(stderr, "Regular splicetrack.\n");
#endif
				// There's no sub-clips here, return regular clip
				SpliceTrack(pClip, str_buf, pTimeStamp, cb);
			}
			else
			{
#ifdef DEBUG_STARSEED
				fmt::print(stderr, "Subclip splicetrack.\n");
#endif
				// This is a subclip of the main dialog
				GetSubClip(clip_buf, pClip, clip_number);
				SpliceTrack(clip_buf, str_buf, pTimeStamp, cb);
				// Advance to the next timestamp if it exists
				if (pTimeStamp)
				{
					if (strstr(pTimeStamp, ";"))
					{
						pTimeStamp = strstr(pTimeStamp, ";") + 1;
					}
					else
					{
						pTimeStamp = nullptr;
					}
				}
				clip_number++;
			}
		}
		else
		{
			// This requires one or more robo-tracks or swap-if subclips
			// which we will MultiSplice into the main track.
			//char *tracks[NUM_ROBO_TRACKS + 1] =
			//{ [0 ... NUM_ROBO_TRACKS] = nullptr };
			char* tracks[NUM_ROBO_TRACKS + 1] = {nullptr};
			char subClips[NUM_ROBO_TRACKS][MAX_CLIPNAME] {};
			for (i = 0; i < NUM_ROBO_TRACKS && RoboTrack[i]; i++)
			{
				if (RoboTrack[i] == (uint16_t)~0)
				{
					// ~0 is a subclip whose name is based off the primary clip
					// We need to allocate a temp buffer and clean it up later
#ifdef DEBUG_STARSEED
					fmt::print(stderr, "Allocating for track {}.\n", i);
#endif
					tracks[i] = subClips[i];
					GetSubClip({tracks[i], MAX_CLIPNAME}, pClip, clip_number);
#ifdef DEBUG_STARSEED
					fmt::print(stderr, "RoboTrack[{}] = <<{}>>.\n", i, tracks[i]);
#endif
					clip_number++;
				}
				else if (RoboTrack[i] > 0)
				{
					// Otherwise the robo-track is an index into robo-phrases
					tracks[i] = GetStringSoundClip(
						SetAbsStringTableIndex(RoboPhrases,
											   RoboTrack[i] - 1));
#ifdef DEBUG_STARSEED
					fmt::print(stderr, "RoboTrack[{}] = <<{}>>.\n", i, tracks[i]);
#endif
				}
				else
				{
					tracks[i] = nullptr;
				}
			}
#ifdef DEBUG_STARSEED
			fmt::print(stderr, "Splice Multitrack string <<{}>>.\n", str_buf);
#endif
			SpliceMultiTrack(tracks, str_buf);
			for (i = 0; i < NUM_ROBO_TRACKS && RoboTrack[i]; i++)
			{
				tracks[i] = nullptr;
				RoboTrack[i] = 0;
			}
		}
	} while (pStr);
}

// Special case variant: prevents page breaks.
void NPCPhrase_splice(int index)
{
	char* pStr;
	void* pClip;

	assert(index >= 0);
	if (index == 0)
	{
		return;
	}

	pStr = (char*)GetStringAddress(
		SetAbsStringTableIndex(CommData.ConversationPhrases, index - 1));
	pClip = GetStringSoundClip(
		SetAbsStringTableIndex(CommData.ConversationPhrases, index - 1));

	if (!pClip)
	{ // Just appending some text
		SpliceTrack(nullptr, pStr, nullptr, nullptr);
	}
	else
	{ // Splicing in some voice
		char* tracks[] = {nullptr, nullptr};

		tracks[0] = (char*)pClip;
		SpliceMultiTrack(tracks, pStr);
	}
}

void NPCNumber(int number, const char* fmt)
{
	char buf[32] {};

	if (!fmt)
	{
		fmt = "{}";
	}

	if (CommData.AlienNumberSpeech)
	{
		NPCNumberPhrase(number, fmt, nullptr);
		return;
	}

	// just splice in the subtitle text
	fmt::format_to_sz_n(buf, fmt::runtime(fmt), number);
	SpliceTrack(nullptr, buf, nullptr, nullptr);
}

static int
NPCNumberPhrase(int number, const char* fmt, char** ptrack)
{
#define MAX_NUMBER_TRACKS 20
	NUMBER_SPEECH speech = CommData.AlienNumberSpeech;
	uint16_t i;
	int queued = 0;
	int toplevel = 0;
	char* TrackNames[MAX_NUMBER_TRACKS];
	char numbuf[60] {};
	const SPEECH_DIGIT* dig = nullptr;

	if (!speech)
	{
		return 0;
	}

	if (!ptrack)
	{
		toplevel = 1;
		if (!fmt)
		{
			fmt = "{}";
		}
		fmt::format_to_n(numbuf, sizeof(numbuf) - 1, fmt::runtime(fmt), number);
		ptrack = TrackNames;
	}

	for (i = 0; i < speech->NumDigits; ++i)
	{
		int quot;

		dig = speech->Digits + i;
		quot = number / dig->Divider;

		if (quot == 0)
		{
			continue;
		}
		quot -= dig->Subtrahend;
		if (quot < 0)
		{
			continue;
		}

		if (dig->StrDigits)
		{
			uint16_t index;

			assert(quot < 10);
			index = dig->StrDigits[quot];
			if (index == 0)
			{
				continue;
			}
			index -= 1;

			*ptrack++ = GetStringSoundClip(SetAbsStringTableIndex(
				CommData.ConversationPhrases, index));
			queued++;
		}
		else
		{
			int ctracks = NPCNumberPhrase(quot, nullptr, ptrack);
			ptrack += ctracks;
			queued += ctracks;
		}

		if (dig->Names != 0)
		{
			SPEECH_DIGITNAME* name;

			for (name = dig->Names; name->Divider; ++name)
			{
				if (number % name->Divider <= name->MaxRemainder)
				{
					*ptrack++ = GetStringSoundClip(
						SetAbsStringTableIndex(
							CommData.ConversationPhrases, name->StrIndex - 1));
					queued++;
					break;
				}
			}
		}
		else if (dig->CommonNameIndex != 0)
		{
			*ptrack++ = GetStringSoundClip(SetAbsStringTableIndex(
				CommData.ConversationPhrases, dig->CommonNameIndex - 1));
			queued++;
		}

		number %= dig->Divider;
	}

	if (toplevel)
	{
		if (queued == 0)
		{ // nothing queued, say "zero"
			assert(number == 0);
			*ptrack++ = GetStringSoundClip(SetAbsStringTableIndex(
				CommData.ConversationPhrases, dig->StrDigits[number] - 1));
		}
		*ptrack++ = nullptr; // term

		SpliceMultiTrack(TrackNames, numbuf);
	}

	return queued;
}

void construct_response(uqstl::span<char> buf, int R /* promoted from RESPONSE_REF */, ...)
{
	char* buf_start = buf.data();
	char* name;
	va_list vlist;

	va_start(vlist, R);

	auto destBuf {buf};
	do
	{
		if (destBuf.empty())
		{
			uqm::log::critical("Error: construct_response buffer size of {} exceeded, please increase!", buf.size());
			exit(EXIT_FAILURE);
		}
		STRING S;

		S = SetAbsStringTableIndex(CommData.ConversationPhrases, R - 1);

		uint16_t len = uqm::strncpy_safe(destBuf, (char*)GetStringAddress(S));
		destBuf = destBuf.subspan(len);

		name = va_arg(vlist, char*);

		if (name)
		{
			const uint32_t nameLen = strlen(name);
			len = uqm::strncpy_safe(destBuf, {name, nameLen});
			destBuf = destBuf.subspan(len);

			/*
			if ((R = va_arg (vlist, RESPONSE_REF)) == (RESPONSE_REF)-1)
				name = 0;
			*/

			R = va_arg(vlist, int);
			if (R == ((RESPONSE_REF)-1))
			{
				name = 0;
			}
		}
	} while (name);
	va_end(vlist);
}

void setSegue(Segue segue)
{
	switch (segue)
	{
		case Segue_peace:
			SET_GAME_STATE(BATTLE_SEGUE, 0);
			break;
		case Segue_hostile:
			SET_GAME_STATE(BATTLE_SEGUE, 1);
			break;
		case Segue_victory:
			instantVictory = true;
			SET_GAME_STATE(BATTLE_SEGUE, 1);
			break;
		case Segue_defeat:
			SET_GAME_STATE(BATTLE_SEGUE, 0);
			GLOBAL_SIS(CrewEnlisted) = (uint16_t)~0;
			GLOBAL(CurrentActivity) |= CHECK_RESTART;
			break;
	}
}

Segue getSegue(void)
{
	if (GET_GAME_STATE(BATTLE_SEGUE) == 0)
	{
		if (GLOBAL_SIS(CrewEnlisted) == (uint16_t)~0 && (GLOBAL(CurrentActivity) & CHECK_RESTART))
		{
			return Segue_defeat;
		}
		else
		{
			return Segue_peace;
		}
	}
	else /* GET_GAME_STATE(BATTLE_SEGUE) == 1) */
	{
		if (instantVictory)
		{
			return Segue_victory;
		}
		else
		{
			return Segue_hostile;
		}
	}
}

LOCDATA*
init_race(CONVERSATION comm_id)
{
	switch (comm_id)
	{
		case ARILOU_CONVERSATION:
			return init_arilou_comm();
		case BLACKURQ_CONVERSATION:
			return init_blackurq_comm();
		case CHMMR_CONVERSATION:
			return init_chmmr_comm();
		case COMMANDER_CONVERSATION:
			if (!GET_GAME_STATE(STARBASE_AVAILABLE))
			{
				return init_commander_comm();
			}
			else
			{
				return init_starbase_comm();
			}
		case DRUUGE_CONVERSATION:
			return init_druuge_comm();
		case ILWRATH_CONVERSATION:
			return init_ilwrath_comm();
		case MELNORME_CONVERSATION:
			return init_melnorme_comm();
		case MYCON_CONVERSATION:
			return init_mycon_comm();
		case ORZ_CONVERSATION:
			return init_orz_comm();
		case PKUNK_CONVERSATION:
			return init_pkunk_comm();
		case SHOFIXTI_CONVERSATION:
			return init_shofixti_comm();
		case SLYLANDRO_CONVERSATION:
			return init_slyland_comm();
		case SLYLANDRO_HOME_CONVERSATION:
			return init_slylandro_comm();
		case SPATHI_CONVERSATION:
			if (!(GET_GAME_STATE(GLOBAL_FLAGS_AND_DATA) & (1 << 7)))
			{
				return init_spathi_comm();
			}
			else
			{
				return init_spahome_comm();
			}
		case SUPOX_CONVERSATION:
			return init_supox_comm();
		case SYREEN_CONVERSATION:
			return init_syreen_comm();
		case TALKING_PET_CONVERSATION:
			return init_talkpet_comm();
		case THRADD_CONVERSATION:
			return init_thradd_comm();
		case UMGAH_CONVERSATION:
			return init_umgah_comm();
		case URQUAN_CONVERSATION:
			return init_urquan_comm();
		case UTWIG_CONVERSATION:
			return init_utwig_comm();
		case VUX_CONVERSATION:
			return init_vux_comm();
		case YEHAT_REBEL_CONVERSATION:
			return init_rebel_yehat_comm();
		case YEHAT_CONVERSATION:
			return init_yehat_comm();
		case ZOQFOTPIK_CONVERSATION:
			return init_zoqfot_comm();
		default:
			return init_chmmr_comm();
	}
}

RESPONSE_REF
phraseIdStrToNum(const char* phraseIdStr)
{
	STRING phrase = GetStringByName(GetStringTable(
										CommData.ConversationPhrases),
									phraseIdStr);
	if (phrase == nullptr)
	{
		return (RESPONSE_REF)-1;
	}

	return GetStringTableIndex(phrase) + 1;
	// Index 0 is for NULL_PHRASE, hence the '+ 1"
}

const char*
phraseIdNumToStr(RESPONSE_REF response)
{
	STRING phrase = SetAbsStringTableIndex(
		CommData.ConversationPhrases, response - 1);
	// Index 0 is for NULL_PHRASE, hence the '- 1'.
	if (phrase == nullptr)
	{
		return nullptr;
	}
	return GetStringName(phrase);
}
