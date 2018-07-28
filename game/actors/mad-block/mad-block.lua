function init()
	dprintf("Spawning mad block!")
	local trigger_size = {0, -256, 32, 256}
	game:spawn_trigger(trigger_size, on_danger_init, on_danger_zone_enter, on_danger_zone_exit)
end

walk_dir = -1
function think(delta_us)
	if (instance.is_tweening) then
		return
	end

	instance:walk_to_rel(walk_dir * 100, 0)
	walk_dir = walk_dir * -1
end

function on_danger_init(trigger)
	instance:add_child(trigger)
end

function on_danger_zone_enter(character, trigger)
	dprintf("A character entered. Increasing velocity!")
	game:play_sound("sound/whoa.wav");
	character.gravity_ps2 = -character.gravity_ps2
end

function on_danger_zone_exit(character, trigger)
	character.gravity_ps2 = -character.gravity_ps2
end
