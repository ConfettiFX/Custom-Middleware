--[[ Move camera along a path --]]

local cameraKeyFrames = {
	{ t = 0.0, position = { 0, 6000, -10 }, lookAt = { 0, 5980, 100 } },
	{ t = 5.52, position = {-122.486, 5571, 267299 }, lookAt = { 0, 6444, 377309 } },
	{ t = 8.5, position = {-700, 201, 418000 }, lookAt = { 6254, -35320, 760000 } },
	{ t = 14.5, position = { 6254, -35320, 760000 }, lookAt = { 2521, -57614, 924257 } },
	{ t = 17.5, position = { 2521, -57614, 924257 }, lookAt = { 2286, -59037, 934507 } }
}

local cloudKeyFrames = {
	{ t = 0.0, distance = 0.0 },
	{ t = 15.0, distance = 0.0 },
	{ t = 24.0, distance = 0.0 }
}

loader.SetCameraPosition(cameraKeyFrames[1].position[1], cameraKeyFrames[1].position[2], cameraKeyFrames[1].position[3])
loader.SetCameraLookAt(cameraKeyFrames[1].lookAt[1], cameraKeyFrames[1].lookAt[2], cameraKeyFrames[1].lookAt[3])
loader.SetLightElevation(210)
loader.ResetFirstFrame()

currentTime = 0.0
nbCameraKeyFrame = 5
nbCloudKeyFrame = 3
cameraKeyIndex = 1
cloudKeyIndex = 1

function Update(dt)
	prevCameraKeyFrame = cameraKeyFrames[cameraKeyIndex]
	nextCameraKeyFrame = cameraKeyFrames[cameraKeyIndex + 1]
	prevCloudKeyFrame = cloudKeyFrames[cloudKeyIndex]
	nextCloudKeyFrame = cloudKeyFrames[cloudKeyIndex + 1]
	currentTime = currentTime + dt

	local cameraLerpCoef = (currentTime - prevCameraKeyFrame.t) / (nextCameraKeyFrame.t - prevCameraKeyFrame.t)
	local cloudLerpCoef = (currentTime - prevCloudKeyFrame.t) / (nextCloudKeyFrame.t - prevCloudKeyFrame.t)
	cameraLerpCoef = math.min(cameraLerpCoef, 1.0)
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