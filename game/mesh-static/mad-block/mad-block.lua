is_mad = false
speed_kmh = 120
ending_spot = [120, 120]

function init(instance, params)
	create_trigger("danger_zone", trigger_rect, on_danger_zone)
	instance.set_acceleration(0.0)
end

function think(instance, delta_ms)
	if is_mad then
		vx = 100
		vy = 0
		instance.set_velocity(vx, vy)
	else
		instance.set_velocity(0, 0)
	end
		
end

function on_danger_zone(entity)
	if  entity.is_player() then
		is_mad = true
	end
end
