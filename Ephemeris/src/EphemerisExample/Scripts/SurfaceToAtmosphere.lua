--[[ Move camera along a path --]]

local cameraKeyFrames = {
	{ t = 0.0, position = { 0, 6000, 0 }, lookAt = { 0, 6000, 100 } },
	{ t = 3.0, position = { 0, 6000, 0 }, lookAt = { 0, 6000, 100 } },
	{ t = 10.0, position = { 133500, 44000, 0 }, lookAt = { 133400, 44000, 0 } },
	{ t = 15.0, position = { 320000, 190000, -123000 }, lookAt = { 319540, 189760, -122800 } },
	{ t = 24.0, position = { 320000, 190000, -123000 }, lookAt = { 319540, 189760, -122800 } },
}

local sunKeyFrames = {
	{ t = 0.0, azimuth = -180.0, elevation = 210.0 },
	{ t = 3.0, azimuth = -180.0, elevation = 210.0 },
	{ t = 15.0, azimuth = -180.0, elevation = 210.0 },
	{ t = 19.0, azimuth = 90.0, elevation = 210.0 },
	{ t = 24.0, azimuth = 90.0, elevation = 142.0 }
}

local cloudKeyFrames = {
	{ t = 0.0, distance = 0.0 },
	{ t = 15.0, distance = 0.0 },
	{ t = 24.0, distance = 0.0 }
}

loader.SetCameraPosition(cameraKeyFrames[1].position[1], cameraKeyFrames[1].position[2], cameraKeyFrames[1].position[3])
loader.SetCameraLookAt(cameraKeyFrames[1].lookAt[1], cameraKeyFrames[1].lookAt[2], cameraKeyFrames[1].lookAt[3])
loader.ResetFirstFrame()

currentTime = 0.0
nbCameraKeyFrame = 5
nbSunKeyFrame = 5
nbCloudKeyFrame = 3
cameraKeyIndex = 1
sunKeyIndex = 1
cloudKeyIndex = 1

function Update(dt)
	prevCameraKeyFrame = cameraKeyFrames[cameraKeyIndex]
	nextCameraKeyFrame = cameraKeyFrames[cameraKeyIndex + 1]
	prevSunKeyFrame = sunKeyFrames[sunKeyIndex]
	nextSunKeyFrame = sunKeyFrames[sunKeyIndex + 1]
	prevCloudKeyFrame = cloudKeyFrames[cloudKeyIndex]
	nextCloudKeyFrame = cloudKeyFrames[cloudKeyIndex + 1]
	currentTime = currentTime + dt

	local cameraLerpCoef = (currentTime - prevCameraKeyFrame.t) / (nextCameraKeyFrame.t - prevCameraKeyFrame.t)
	local sunLerpCoef = (currentTime - prevSunKeyFrame.t) / (nextSunKeyFrame.t - prevSunKeyFrame.t)
	local cloudLerpCoef = (currentTime - prevCloudKeyFrame.t) / (nextCloudKeyFrame.t - prevCloudKeyFrame.t)
	cameraLerpCoef = math.min(cameraLerpCoef, 1.0)
	sunLerpCoef = math.min(sunLerpCoef, 1.0)
	cloudLerpCoef = math.min(cloudLerpCoef, 1.0)

	loader.AnimateCamera(
		prevCameraKeyFrame.position[1], prevCameraKeyFrame.position[2], prevCameraKeyFrame.position[3],
		prevCameraKeyFrame.lookAt[1], prevCameraKeyFrame.lookAt[2], prevCameraKeyFrame.lookAt[3],
		-- next key frame
		nextCameraKeyFrame.position[1], nextCameraKeyFrame.position[2], nextCameraKeyFrame.position[3],
		nextCameraKeyFrame.lookAt[1], nextCameraKeyFrame.lookAt[2], nextCameraKeyFrame.lookAt[3],
		-- lerpcoef
		cameraLerpCoef
	)

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
	if currentTime > nextCameraKeyFrame.t then
		cameraKeyIndex = cameraKeyIndex + 1
		if cameraKeyIndex >= nbCameraKeyFrame then
			loader.StopCurrentScript()
		end
	end

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