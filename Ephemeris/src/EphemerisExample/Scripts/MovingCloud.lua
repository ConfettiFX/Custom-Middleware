--[[ Displace the weather map --]]

loader.SetWeatherTextureDistance(0);

movingDirection = 1.0
maxDisplacement = 280000.0
speed = 10000.0

function Update(dt)
	current = loader.GetWeatherTextureDistance()
	if math.abs(current) > maxDisplacement then
		current = movingDirection * maxDisplacement * 0.95
		movingDirection = movingDirection * -1.0;
	end

	local delta = speed * dt;
	loader.SetWeatherTextureDistance(current + delta * movingDirection);
end


function Exit(dt)
    loader.SetWeatherTextureDistance(0);
end