function init(instance)
	game:spawn_trigger({200, 100, 256, 128}, on_danger_zone_enter, on_danger_zone_exit)
end

function think(instance, delta_ms)
		
end

function on_danger_zone_enter(character, trigger)
	print("A character entered. Increasing velocity!")
	character:add_velocity(0, 50)
end

function on_danger_zone_exit(entity, trigger)
	print("A character exited...")
end
