--[[ Set camera at ground level --]]


loader.SetWeatherTextureDistance(0)

local keyFrame0 = { position = { 0, 6000, 0 }, lookAt = { 100, 6000, 0 } }

loader.SetCameraPosition(keyFrame0.position[1], keyFrame0.position[2], keyFrame0.position[3])
loader.SetCameraLookAt(keyFrame0.lookAt[1], keyFrame0.lookAt[2], keyFrame0.lookAt[3])
loader.ResetFirstFrame()