function init()
	dprintf("Spawning mad block!")
	local trigger_size = {0, -256, 32, 256}
	game:spawn_trigger(trigger_size, on_danger_init, on_danger_zone_enter, on_danger_zone_exit)
end

function think(delta_ms)
		
end

function on_danger_init(trigger)
	instance:add_child(trigger)
end

function on_danger_zone_enter(character, trigger)
	dprintf("A character entered. Increasing velocity!")
	character:add_velocity(0, 50)
end

function on_danger_zone_exit(entity, trigger)
	dprintf("A character exited...")
end
