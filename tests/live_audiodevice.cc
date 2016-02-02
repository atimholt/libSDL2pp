#include <atomic>
#include <algorithm>

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>

#include "testing.h"
#include "movetest.hh"

using namespace SDL2pp;

BEGIN_TEST(int, char*[])
	SDL sdl(SDL_INIT_AUDIO);

	static constexpr int samplerate = 48000;

	AudioSpec spec(samplerate, AUDIO_S16SYS, 1, 4096);

	std::atomic_long callback_requests(0);

	AudioDevice device(NullOpt, 0, spec, [&callback_requests](Uint8* stream, int len) {
				std::fill(stream, stream + len, 0);
				++callback_requests;
			});

	MOVE_TEST(AudioDevice, device, Get, 0)

	{
		// Default state
		EXPECT_TRUE(device.GetStatus(), SDL_AUDIO_PAUSED);
		EXPECT_TRUE(callback_requests == 0);

		long saved_reqs = callback_requests;

		// Unpause
		device.Pause(false);
		EXPECT_TRUE(device.GetStatus(), SDL_AUDIO_PLAYING);

		SDL_Delay(1000);
		EXPECT_TRUE(callback_requests > saved_reqs);

		// Pause
		device.Pause(true);
		EXPECT_TRUE(device.GetStatus(), SDL_AUDIO_PLAYING);

		saved_reqs = callback_requests;

		SDL_Delay(1000);
		EXPECT_TRUE(callback_requests == saved_reqs);

		device.Pause(false);

		{
			// Lock
			AudioDevice::LockHandle lock = device.Lock();
			saved_reqs = callback_requests;

			SDL_Delay(1000);

			EXPECT_TRUE(callback_requests == saved_reqs);

			{
				// Recursive lock
				AudioDevice::LockHandle lock1(lock);

				AudioDevice::LockHandle lock2, lock4;

				lock2 = lock1;

				AudioDevice::LockHandle lock3(std::move(lock1));

				lock4 = std::move(lock2);

				SDL_Delay(1000);
			}

			EXPECT_TRUE(callback_requests == saved_reqs);
		}

		// Unlocked
		SDL_Delay(1000);
		EXPECT_TRUE(callback_requests > saved_reqs);

		// Change callback
		device.ChangeCallback([&callback_requests](Uint8* stream, int len) {
				std::fill(stream, stream + len, 0);
				--callback_requests;
			});

		saved_reqs = callback_requests;

		SDL_Delay(1000);
		EXPECT_TRUE(callback_requests < saved_reqs);
	}
END_TEST()
