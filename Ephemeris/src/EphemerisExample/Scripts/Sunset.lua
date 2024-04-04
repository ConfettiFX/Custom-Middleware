--[[ Move camera along a path --]]

local sunKeyFrames = {
	{ t = 0.0, azimuth = 106.5, elevation = 200 },
	{ t = 7.0, azimuth = 106.5, elevation = 181.2 },
	{ t = 9.0, azimuth = 106.5, elevation = 181.2 },
	{ t = 12.0, azimuth = 42, elevation = 181.2 },
	{ t = 15.0, azimuth = 106.5, elevation = 181.2 },
	{ t = 16.0, azimuth = 106.5, elevation = 181.2 },
	{ t = 18.0, azimuth = 106.5, elevation = 178.0 }
}

loader.ResetFirstFrame()

currentTime = 0.0
nbSunKeyFrame = 7
sunKeyIndex = 1
cloudKeyIndex = 1

function Update(dt)
	prevSunKeyFrame = sunKeyFrames[sunKeyIndex]
	nextSunKeyFrame = sunKeyFrames[sunKeyIndex + 1]
	currentTime = currentTime + dt

	local sunLerpCoef = (currentTime - prevSunKeyFrame.t) / (nextSunKeyFrame.t - prevSunKeyFrame.t)
	sunLerpCoef = math.min(sunLerpCoef, 1.0)

	loader.AnimateSun(
		prevSunKeyFrame.azimuth, prevSunKeyFrame.elevation,
		-- next key frame
		nextSunKeyFrame.azimuth, nextSunKeyFrame.elevation,
		-- lerpcoef
		sunLerpCoef
	)

	-- Stop at first end of keys
	if currentTime > nextSunKeyFrame.t then
		sunKeyIndex = sunKeyIndex + 1
		if sunKeyIndex >= nbSunKeyFrame then
			loader.StopCurrentScript()
		end
	end
end


function Exit(dt)
    loader.SetWeatherTextureDistance(0);
end