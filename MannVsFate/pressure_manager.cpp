#include "pressure_manager.h"
#include "currency_manager.h"
#include <cmath>

pressure_manager::pressure_manager(currency_manager& cm)
	: mission_currency(cm),
	pressure(0.0f),
	players(4),
	base_pressure_decay_rate(600), // 700
	pressure_decay_rate_multiplier(0.0175f), // 0.0225f
	pressure_decay_rate_multiplier_in_time(3.0f),
	bot_path_length(1.0f),
	pps_factor_tfbot(1.0f),
	pps_factor_tank(0.05f),
	player_exponent(1.0f) // 1.175f;
{}

float pressure_manager::get_pressure() const
{
	return pressure;
}

float pressure_manager::get_pressure_decay_rate() const
{
	return pressure_decay_rate;
}

void pressure_manager::set_players(int in)
{
	players = in;
}

int pressure_manager::get_players() const
{
	return players;
}

void pressure_manager::multiply_pressure_decay_rate_multiplier(float multiplier)
{
	pressure_decay_rate_multiplier *= multiplier;
}

void pressure_manager::set_pressure_decay_rate_multiplier_in_time(float in)
{
	pressure_decay_rate_multiplier_in_time = in;
}

void pressure_manager::set_bot_path_length(float in)
{
	bot_path_length = in;
}

void pressure_manager::reset_pressure()
{
	pressure = 0.0f;
}

void pressure_manager::step_through_time(int& t)
{
	// This loop will increment time until there's no pressure left.
	// When there's no pressure left, we can either generate more WaveSpawns or end the wave.
	while (pressure > 0.0f)
	{
		// Increment time.
		++t;

		// How much the pressure will be increased based on the active WaveSpawns.
		float pressure_increase = 0.0f;

		// Iterate through the wavespawns to instantiate any new pressure bubbles.
		for (virtual_wavespawn& ws : wavespawns)
		{
			ws.time_until_next_spawn -= 1.0f;

			while (ws.spawns_remaining != 0 && ws.time_until_next_spawn <= 0.0f)
			{
				ws.spawns_remaining -= 1;
				ws.time_until_next_spawn += ws.wait_between_spawns;

				// Instantiate a new virtual enemy to represent the newly spawned enemy.
				add_virtual_spawn(ws);
			}
		}

		// Iterate through all existing virtual enemies and adjust the pressure accordingly.
		for (unsigned int i = 0; i < spawns.size(); ++i)
		{
			virtual_spawn& spawn = spawns.at(i);
			spawn.decrement_time();
			if (spawn.is_dead())
			{
				// The entirety of the spawn just died.

				// Collect the spawn's currency.
				int spawn_currency = spawn.get_currency();
				mission_currency.add_currency(spawn_currency);
				mission_currency.add_approximated_additional_currency(spawn_currency);

				// Since the virtual spawn is dead, remove it from the spawns list.
				spawns.erase(spawns.begin() + i);
				// Move i backwards so we don't end up skipping an element of the vector.
				// This is necessary because we just deleted an element.
				--i;
			}
			else
			{
				// The spawn isn't dead yet, so add some pressure.
				pressure += spawn.get_pressure_per_second();
			}
		}

		// Since time has passed, reduce the pressure.

		//pressure -= pressure_decay_rate * pressure_decay_rate_multiplier_in_time;

		unsigned int active_spawns = spawns.size();
		pressure -= pressure_decay_rate * pressure_decay_rate_multiplier_in_time / (active_spawns * 0.2f + 1);
	}
}

void pressure_manager::add_virtual_wavespawn(const virtual_wavespawn& ws)
{
	wavespawns.emplace_back(ws);
	add_virtual_spawn(ws);
}

void pressure_manager::add_virtual_spawn(const virtual_wavespawn& ws)
{
	// Perform some preliminary calculations for the virtual spawn.
	const int rounded_time_to_kill = static_cast<int>(std::ceil(ws.time_to_kill));
	const float pressure_per_second = ws.effective_pressure / rounded_time_to_kill;

	// No pressure per second per second.

	//virtual_spawn spawn(rounded_time_to_kill, pressure_per_second, ws.currency_per_spawn);

	// Pressure per second per second is active.

	float pps_multiplier;
	if (ws.is_tank)
	{
		pps_multiplier = pps_factor_tank;
	}
	else
	{
		pps_multiplier = pps_factor_tfbot;
	}

	virtual_spawn spawn(rounded_time_to_kill, pressure_per_second * pps_multiplier, ws.effective_pressure, ws.currency_per_spawn);

	// Add the spawn to the spawns collection.
	spawns.emplace_back(spawn);

	// Add the initial pressure of the spawn.
	pressure += spawn.get_pressure_per_second();
}

void pressure_manager::calculate_pressure_decay_rate()
{
	pressure_decay_rate = (mission_currency.get_currency_pressure() + base_pressure_decay_rate)
		* get_effective_players() * pressure_decay_rate_multiplier * bot_path_length;
}

float pressure_manager::get_pressure_decay_rate_per_player() const
{
	const float return_value = pressure_decay_rate / get_effective_players();
	return return_value;
}

float pressure_manager::get_effective_players() const
{
	return pow(static_cast<float>(players), player_exponent);
}