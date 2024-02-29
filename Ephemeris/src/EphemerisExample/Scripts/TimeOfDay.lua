--[[ Move camera along a path --]]

local sunKeyFrames = {
	{ t = 0.0, azimuth = 76.4, elevation = 359 },
	{ t = 6.0, azimuth = 76.4, elevation = 359 },
	{ t = 20.0, azimuth = 76.4, elevation = 4 },
	{ t = 23.0, azimuth = 76.4, elevation = 4 }
}

local cloudKeyFrames = {
	{ t = 0.0, distance = -190000 },
	{ t = 6.0, distance = -190000 },
	{ t = 20.0, distance = 230000 },
	{ t = 23.0, distance = 260000 }
}

cameraPosition = { -41686, 9775, -80565 }
cameraLookAt = { -28160, 9484, -54898 }

loader.SetCameraPosition(cameraPosition[1], cameraPosition[2], cameraPosition[3])
loader.SetCameraLookAt(cameraLookAt[1], cameraLookAt[2], cameraLookAt[3])
loader.ResetFirstFrame()

currentTime = 0.0
nbSunKeyFrame = 4
nbCloudKeyFrame = 4
sunKeyIndex = 1
cloudKeyIndex = 1

function Update(dt)
	prevSunKeyFrame = sunKeyFrames[sunKeyIndex]
	nextSunKeyFrame = sunKeyFrames[sunKeyIndex + 1]
	prevCloudKeyFrame = cloudKeyFrames[cloudKeyIndex]
	nextCloudKeyFrame = cloudKeyFrames[cloudKeyIndex + 1]
	currentTime = currentTime + dt

	local sunLerpCoef = (currentTime - prevSunKeyFrame.t) / (nextSunKeyFrame.t - prevSunKeyFrame.t)
	local cloudLerpCoef = (currentTime - prevCloudKeyFrame.t) / (nextCloudKeyFrame.t - prevCloudKeyFrame.t)
	sunLerpCoef = math.min(sunLerpCoef, 1.0)
	cloudLerpCoef = math.min(cloudLerpCoef, 1.0)

	loader.AnimateSun(
		prevSunKeyFrame.azimuth, prevSunKeyFrame.elevation,
		-- next key frame
		nextSunKeyFrame.azimuth, nextSunKeyFrame.elevation,
		-- lerpcoef
		sunLerpCoef
	)

	loader.AnimateCloud(
		prevCloudKeyFrame.distance,
		-- next key frame
		nextCloudKeyFrame.distance,
		-- lerpcoef
		cloudLerpCoef
	)

	-- Stop at first end of keys
	if currentTime > nextSunKeyFrame.t then
		sunKeyIndex = sunKeyIndex + 1
		if sunKeyIndex >= nbSunKeyFrame then
			loader.StopCurrentScript()
		end
	end

	if currentTime > nextCloudKeyFrame.t then
		cloudKeyIndex = cloudKeyIndex + 1
		if cloudKeyIndex >= nbCloudKeyFrame then
			loader.StopCurrentScript()
		end
	end
end


function Exit(dt)
    loader.SetWeatherTextureDistance(0);
end